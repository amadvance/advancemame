/*
    Nintendo 64 Video Hardware
*/

#include "driver.h"

#define LOG_RDP_EXECUTION 		0

#if LOG_RDP_EXECUTION
static FILE *rdp_exec;
#endif

/* defined in systems/n64.c */
extern UINT32 *rdram;
extern UINT32 *rsp_imem;
extern UINT32 *rsp_dmem;
extern void dp_full_sync(void);

extern UINT32 vi_origin;
extern UINT32 vi_width;
extern UINT32 vi_control;

extern UINT32 dp_start;
extern UINT32 dp_end;
extern UINT32 dp_current;
extern UINT32 dp_status;

static UINT32 rdp_cmd_data[0x1000];
static int rdp_cmd_ptr = 0;
static int rdp_cmd_cur = 0;

/*****************************************************************************/

#if LSB_FIRST
	#define BYTE_ADDR_XOR		3
	#define WORD_ADDR_XOR		1
#else
	#define BYTE_ADDR_XOR		0
	#define WORD_ADDR_XOR		0
#endif

typedef struct
{
	UINT8 r, g, b, a;
} COLOR;

typedef struct
{
	UINT16 xl, yl, xh, yh;		// 10.2 fixed-point
} RECTANGLE;

typedef struct
{
	int tilenum;
	UINT16 xl, yl, xh, yh;		// 10.2 fixed-point
	INT16 s, t;					// 10.5 fixed-point
	INT16 dsdx, dtdy;			// 5.10 fixed-point
} TEX_RECTANGLE;

typedef struct
{
	int format, size;
	UINT32 line;
	UINT32 tmem;
	int palette;
	// TODO: clamp & mirror parameters

	UINT16 sl, tl, sh, th;		// 10.2 fixed-point
} TILE;

/*****************************************************************************/

#define PIXEL_SIZE_4BIT			0
#define PIXEL_SIZE_8BIT			1
#define PIXEL_SIZE_16BIT		2
#define PIXEL_SIZE_32BIT		3

#define OTHER_MODES_PERSP_TEX			U64(0x0008000000000000)
#define OTHER_MODES_DETAIL_TEX			U64(0x0004000000000000)
#define OTHER_MODES_SHARPEN_TEX			U64(0x0002000000000000)
#define OTHER_MODES_TEX_LOD				U64(0x0001000000000000)
#define OTHER_MODES_TLUT_EN				U64(0x0000800000000000)
#define OTHER_MODES_TLUT_TYPE			U64(0x0000400000000000)
#define OTHER_MODES_SAMPLE_TYPE			U64(0x0000200000000000)
#define OTHER_MODES_MIDTEXEL			U64(0x0000100000000000)
#define OTHER_MODES_BILERP_0			U64(0x0000080000000000)
#define OTHER_MODES_BILERP_1			U64(0x0000040000000000)
#define OTHER_MODES_CONVERT_ONE			U64(0x0000020000000000)
#define OTHER_MODES_CHROMA_KEY_EN		U64(0x0000010000000000)
#define OTHER_MODES_FORCE_BLEND			U64(0x0000000000004000)
#define OTHER_MODES_ALPHA_CVG_SEL		U64(0x0000000000002000)
#define OTHER_MODES_CVG_TIMES_ALPHA		U64(0x0000000000001000)
#define OTHER_MODES_COLOR_ON_CVG		U64(0x0000000000000040)
#define OTHER_MODES_ZUPDATE				U64(0x0000000000000020)
#define OTHER_MODES_ZCOMPARE			U64(0x0000000000000010)
#define OTHER_MODES_ANTIALIAS			U64(0x0000000000000008)
#define OTHER_MODES_ZSOURCE_SEL			U64(0x0000000000000004)
#define OTHER_MODES_DITHER_ALPHA		U64(0x0000000000000002)
#define OTHER_MODES_ALPHA_COMPARE		U64(0x0000000000000001)

static COLOR blend_color;
static COLOR prim_color;
static COLOR env_color;
static COLOR fog_color;

static UINT32 fill_color;		// packed 16-bit or 32-bit, depending on framebuffer format

static UINT16 primitive_z;
static UINT16 primitive_delta_z;

static int fb_format;
static int fb_size;
static int fb_width;
static UINT32 fb_address;

static UINT32 zb_address;

static int ti_format;
static int ti_size;
static int ti_width;
static UINT32 ti_address;

static TILE tile[8];

static RECTANGLE clip;

static UINT64 other_modes;

static UINT8 *texture_cache;
static UINT32 tlut[256];

/*****************************************************************************/



VIDEO_START(n64)
{
#if LOG_RDP_EXECUTION
	rdp_exec = fopen("rdp_execute.txt", "wt");
#endif

	texture_cache = auto_malloc(0x100000);

	return 0;
}

VIDEO_UPDATE(n64)
{
	int i, j;
	int height = (vi_control & 0x40) ? 479 : 239;

	switch (vi_control & 0x3)
	{
		case 0:		// blank/no signal
		{
			break;
		}

		case 2:		// RGBA5551
		{
			UINT16 *frame_buffer = (UINT16*)&rdram[vi_origin / 4];
			if (frame_buffer)
			{
				for (j=0; j <height; j++)
				{
					UINT32 *d = bitmap->line[j];
					for (i=0; i < vi_width; i++)
					{
						UINT16 pix = *frame_buffer++;
						int r = ((pix >> 11) & 0x1f) << 3;
						int g = ((pix >> 6) & 0x1f) << 3;
						int b = ((pix >> 1) & 0x1f) << 3;
						d[i^1] = (r << 16) | (g << 8) | b;
					}
				}
			}
			break;
		}

		case 3:		// RGBA8888
		{
			UINT32 *frame_buffer = (UINT32*)&rdram[vi_origin / 4];
			if (frame_buffer)
			{
				for (j=0; j < height; j++)
				{
					UINT32 *d = bitmap->line[j];
					for (i=0; i < vi_width; i++)
					{
						UINT32 pix = *frame_buffer++;
						*d++ = pix & 0xffffff;
					}
				}
			}
			break;
		}

		default:	fatalerror("Unknown framebuffer format %d\n", vi_control & 0x3);
	}
}


/*****************************************************************************/

static void fill_rectangle_16bit(RECTANGLE *rect)
{
	UINT16 *fb = (UINT16*)&rdram[(fb_address / 4)];
	int index, i, j;
	int x1 = rect->xh / 4;
	int x2 = rect->xl / 4;
	int y1 = rect->yh / 4;
	int y2 = rect->yl / 4;

	// TODO: clip

	for (j=y1; j <= y2; j++)
	{
		index = j * fb_width;
		for (i=x1; i <= x2; i++)
		{
			fb[(index + i) ^ 1] = (i & 1) ? (UINT16)(fill_color >> 16) : (UINT16)(fill_color);
		}
	}
}

static void texture_rectangle_16bit(TEX_RECTANGLE *rect)
{
	UINT16 *fb = (UINT16*)&rdram[(fb_address / 4)];
	int index, i, j, tline;
	int x1, x2, y1, y2;
	int clipx1, clipx2, clipy1, clipy2;

	UINT32 tb = tile[rect->tilenum].tmem;
	UINT32 twidth = tile[rect->tilenum].line;

	INT32 s = 0;
	INT32 t = 0;

	x1 = rect->xh / 4;
	x2 = rect->xl / 4;
	y1 = rect->yh / 4;
	y2 = rect->yl / 4;

	clipx1 = clip.xh / 4;
	clipx2 = clip.xl / 4;
	clipy1 = clip.yh / 4;
	clipy2 = clip.yl / 4;

	// clip
	if (x1 < clipx1)
	{
		s += ((clipx1 - x1) * rect->dsdx) >> 5;
		x1 = clipx1;
	}
	if (y1 < clipy1)
	{
		t += ((clipy1 - y1) * rect->dtdy) >> 5;
		y1 = clipy1;
	}
	if (x2 >= clipx2)
	{
		x2 = clipx2-1;
	}
	if (y2 >= clipy2)
	{
		y2 = clipy2-1;
	}

	if (((other_modes >> 52) & 3) >= 2)
	{
		rect->dsdx >>= 2;
		x2 += 1;
		y2 += 1;
	}

	t = rect->t << 5;	// to .10 fixed


	switch (tile[rect->tilenum].format)
	{
		case 0:		// RGBA
		{
			switch (tile[rect->tilenum].size)
			{
				case PIXEL_SIZE_16BIT:
				{
					UINT16 *tc = (UINT16*)texture_cache;
					for (j=y1; j < y2; j++)
					{
						index = j * fb_width;
						tline = (tb/2) + ((t >> 10) * (twidth/2));

						s = rect->s << 5;		// to .10 fixed

						for (i=x1; i < x2; i++)
						{
							fb[(index + i) ^ WORD_ADDR_XOR] = tc[(tline + (s >> 10)) ^ WORD_ADDR_XOR];
							s += rect->dsdx;

							if (s > tile[rect->tilenum].sh << 8)
								s = tile[rect->tilenum].sl << 8;
						}
						t += rect->dtdy;
						if (t > tile[rect->tilenum].th << 8)
							t = tile[rect->tilenum].tl << 8;
					}
					break;
				}
				case PIXEL_SIZE_32BIT:
				{
					UINT32 *tc = (UINT32*)texture_cache;
					for (j=y1; j < y2; j++)
					{
						index = j * fb_width;
						tline = (tb/4) + ((t >> 10) * (twidth / 4));

						s = rect->s << 5;		// to .10 fixed

						for (i=x1; i < x2; i++)
						{
							UINT32 pix = tc[tline + (s >> 10)];
							UINT32 r = ((pix >> 24) >> 3) & 0x1f;
							UINT32 g = ((pix >> 16) >> 3) & 0x1f;
							UINT32 b = ((pix >>  8) >> 3) & 0x1f;
							UINT32 a = ((pix >>  0) >> 3) & 0x1f;
							if (a > 0)
							{
								fb[(index + i) ^ WORD_ADDR_XOR] = (r << 11) | (g << 6) | (b << 1);
							}
							s += rect->dsdx;

							if (s > tile[rect->tilenum].sh << 8)
								s = tile[rect->tilenum].sl << 8;
						}
						t += rect->dtdy;
						if (t > tile[rect->tilenum].th << 8)
							t = tile[rect->tilenum].tl << 8;
					}
					break;
				}

				//default:  osd_die("RDP: texture_rectangle 16-bit RGBA: tile pixel size = %d\n", tile[rect->tilenum].size); break;
			}
			break;
		}

		case 2:		// Color Index
		{
			switch (tile[rect->tilenum].size)
			{
				case PIXEL_SIZE_4BIT:
				{
					UINT8 *tc = (UINT8*)texture_cache;
					for (j=y1; j < y2; j++)
					{
						index = j * fb_width;
						tline = tb + ((t >> 10) * twidth);

						s = rect->s << 5;		// to .10 fixed
						for (i=x1; i < x2; i++)
						{
							UINT16 color;
							UINT8 pix = tc[(tline + ((s >> 10) / 2)) ^ BYTE_ADDR_XOR];
							pix = ((s >> 10) & 1) ? (pix >> 0) & 0xf : (pix >> 4) & 0xf;
							color = tlut[pix ^ WORD_ADDR_XOR];
							if (color & 1)
							{
								fb[(index + i) ^ WORD_ADDR_XOR] = color;
							}

							s += rect->dsdx;
							if (s > tile[rect->tilenum].sh << 8)
								s = tile[rect->tilenum].sl << 8;
						}

						t += rect->dtdy;
						if (t > tile[rect->tilenum].th << 8)
							t = tile[rect->tilenum].tl << 8;
					}
					break;
				}
				case PIXEL_SIZE_8BIT:
				{
					UINT8 *tc = (UINT8*)texture_cache;
					for (j=y1; j < y2; j++)
					{
						UINT32 xor = 0;
						index = j * fb_width;
						tline = tb + ((t >> 10) * twidth);

						//xor = (t >> 10) & 1 ? 4 : 0;

						s = rect->s << 5;		// to .10 fixed
						for (i=x1; i < x2; i++)
						{
							UINT8 pix = tc[(tline + (s >> 10)) ^ BYTE_ADDR_XOR ^ xor];
							UINT16 color = tlut[pix ^ WORD_ADDR_XOR];
							if (color & 1)
							{
								fb[(index + i) ^ WORD_ADDR_XOR] = color;
							}

							s += rect->dsdx;
							if (s > tile[rect->tilenum].sh << 8)
								s = tile[rect->tilenum].sl << 8;
						}

						t += rect->dtdy;
						if (t > tile[rect->tilenum].th << 8)
							t = tile[rect->tilenum].tl << 8;
					}
					break;
				}

				default:	fatalerror("RDP: texture_rectangle 16-bit CI: tile pixel size = %d\n", tile[rect->tilenum].size); break;
			}
			break;
		}

		case 3:		// IA
		{
			switch (tile[rect->tilenum].size)
			{
				case PIXEL_SIZE_8BIT:
				{
					UINT8 *tc = (UINT8*)texture_cache;
					for (j=y1; j < y2; j++)
					{
						index = j * fb_width;
						tline = tb + ((t >> 10) * twidth);

						s = rect->s << 5;		// to .10 fixed
						for (i=x1; i < x2; i++)
						{
							UINT8 pix = tc[(tline + (s >> 10)) ^ BYTE_ADDR_XOR];
							UINT8 lum = (pix >> 4) << 1;
							UINT8 alpha = (pix & 0xf);
							if (alpha != 0)
							{
								UINT16 oldpix = fb[(index + i) ^ WORD_ADDR_XOR];
								UINT32 r = oldpix & 0xf800;
								UINT32 g = oldpix & 0x07c0;
								UINT32 b = oldpix & 0x003e;

								r = (((lum << 11) * alpha) + (r * (15-alpha))) >> 4;
								g = (((lum <<  6) * alpha) + (g * (15-alpha))) >> 4;
								b = (((lum <<  1) * alpha) + (b * (15-alpha))) >> 4;

								fb[(index + i) ^ WORD_ADDR_XOR] = r | g | b;
							}
							s += rect->dsdx;
						}
						t += rect->dtdy;
					}
					break;
				}

				case PIXEL_SIZE_16BIT:
				{
					UINT16 *tc = (UINT16*)texture_cache;
					for (j=y1; j < y2; j++)
					{
						index = j * fb_width;
						tline = (tb/2) + ((t >> 10) * (twidth/2));

						s = rect->s << 5;		// to .10 fixed
						for (i=x1; i < x2; i++)
						{
							UINT16 pix = tc[(tline + (s >> 10)) ^ WORD_ADDR_XOR];
							UINT8 alpha = (pix >> 8);
							UINT8 lum = (pix & 0xff);
							if (alpha != 0)
							{
								UINT16 oldpix = fb[(index + i) ^ WORD_ADDR_XOR];
								UINT32 r = oldpix & 0xf800;
								UINT32 g = oldpix & 0x07c0;
								UINT32 b = oldpix & 0x003e;

								r = (((lum << 11) * alpha) + (r * (255-alpha))) >> 8;
								g = (((lum <<  6) * alpha) + (g * (255-alpha))) >> 8;
								b = (((lum <<  1) * alpha) + (b * (255-alpha))) >> 8;

								lum >>= 3;

								fb[(index + i) ^ WORD_ADDR_XOR] = lum << 11 | lum << 6 | lum << 1;//r | g | b;
							}
							s += rect->dsdx;
						}
						t += rect->dtdy;
					}
					break;
				}

				default:	fatalerror("RDP: texture_rectangle 16-bit IA: tile pixel size = %d\n", tile[rect->tilenum].size); break;
			}
			break;
		}

		case 4:		// I
		{
			switch (tile[rect->tilenum].size)
			{
				case PIXEL_SIZE_4BIT:
				{
					UINT8 *tc = (UINT8*)texture_cache;
					for (j=y1; j < y2; j++)
					{
						index = j * fb_width;
						tline = tb + ((t >> 10) * twidth);

						s = rect->s << 5;		// to .10 fixed
						for (i=x1; i < x2; i++)
						{
							UINT8 lum = tc[(tline + ((s >> 10) / 2)) ^ BYTE_ADDR_XOR];
							lum = ((s >> 10) & 1) ? ((lum >> 0) & 0xf) : ((lum >> 4) & 0xf);
							lum <<= 1;
							fb[(index + i) ^ WORD_ADDR_XOR] = lum << 11 | lum << 6 | lum << 1;

							s += rect->dsdx;

							if (s > tile[rect->tilenum].sh << 8)
								s = tile[rect->tilenum].sl << 8;
						}
						t += rect->dtdy;

						if (t > tile[rect->tilenum].th << 8)
							t = tile[rect->tilenum].tl << 8;
					}
					break;
				}

				default:	fatalerror("RDP: texture_rectangle 16-bit I: tile pixel size = %d\n", tile[rect->tilenum].size); break;
			}
			break;
		}

		default:	fatalerror("RDP: texture_rectangle 16-bit: tile format = %d\n", tile[rect->tilenum].format); break;
	}
}

static void fill_rectangle_32bit(RECTANGLE *rect)
{
	UINT32 *fb = (UINT32*)&rdram[(fb_address / 4)];
	int index, i, j;
	int x1 = rect->xh / 4;
	int x2 = rect->xl / 4;
	int y1 = rect->yh / 4;
	int y2 = rect->yl / 4;

	// TODO: clip

	for (j=y1; j <= y2; j++)
	{
		index = j * fb_width;
		for (i=x1; i <= x2; i++)
		{
			fb[index + i] = fill_color;
		}
	}
}

static void texture_rectangle_32bit(TEX_RECTANGLE *rect)
{
	UINT32 *fb = (UINT32*)&rdram[(fb_address / 4)];
	int index, i, j, tline;
	int x1, x2, y1, y2;
	int clipx1, clipx2, clipy1, clipy2;

	UINT32 tb = tile[rect->tilenum].tmem;
	UINT32 twidth = tile[rect->tilenum].line;

	UINT32 s = 0;
	UINT32 t = 0;

	x1 = rect->xh / 4;
	x2 = rect->xl / 4;
	y1 = rect->yh / 4;
	y2 = rect->yl / 4;

	clipx1 = clip.xh / 4;
	clipx2 = clip.xl / 4;
	clipy1 = clip.yh / 4;
	clipy2 = clip.yl / 4;

	// clip
	if (x1 < clipx1)
	{
		s += ((clipx1 - x1) * rect->dsdx) >> 5;
		x1 = clipx1;
	}
	if (y1 < clipy1)
	{
		t += ((clipy1 - y1) * rect->dtdy) >> 5;
		y1 = clipy1;
	}
	if (x2 >= clipx2)
	{
		x2 = clipx2-1;
	}
	if (y2 >= clipy2)
	{
		y2 = clipy2-1;
	}

	if (((other_modes >> 52) & 3) >= 2)
	{
		rect->dsdx >>= 2;	// 4 pixels at a time in fill and copy modes
		x2 += 1;
		y2 += 1;
	}

	t = rect->t << 5;	// to .10 fixed


	switch (tile[rect->tilenum].format)
	{
		case 0:		// RGBA
		{
			switch (tile[rect->tilenum].size)
			{
				case PIXEL_SIZE_16BIT:
				{
					UINT16 *tc = (UINT16*)texture_cache;
					for (j=y1; j < y2; j++)
					{
						index = j * fb_width;
						tline = (tb/2) + ((t >> 10) * (twidth / 2));

						s = rect->s << 5;		// to .10 fixed

						for (i=x1; i < x2; i++)
						{
							UINT32 r, g, b;
							UINT16 color = tc[(tline + (s >> 10)) ^ WORD_ADDR_XOR];
							r = ((color >> 11) & 0x1f);
							r = (r << 3) | (r >> 5);
							g = ((color >>  6) & 0x1f);
							g = (g << 3) | (g >> 5);
							b = ((color >>  1) & 0x1f);
							b = (b << 3) | (b >> 5);

							if (color & 0x1)
							{
								fb[index + i] = (r << 16) | (g << 8) | b;
							}

							s += rect->dsdx;

							if (s > tile[rect->tilenum].sh << 8)
								s = tile[rect->tilenum].sl << 8;
						}
						t += rect->dtdy;
						if (t > tile[rect->tilenum].th << 8)
							t = tile[rect->tilenum].tl << 8;
					}
					break;
				}

				case PIXEL_SIZE_32BIT:
				{
					UINT32 *tc = (UINT32*)texture_cache;
					for (j=y1; j < y2; j++)
					{
						index = j * fb_width;
						tline = (tb/4) + ((t >> 10) * (twidth / 4));

						s = rect->s << 5;		// to .10 fixed

						for (i=x1; i < x2; i++)
						{
							UINT32 color = tc[(tline + (s >> 10))];
							fb[index + i] = color;
							s += rect->dsdx;

							if (s > tile[rect->tilenum].sh << 8)
								s = tile[rect->tilenum].sl << 8;
						}
						t += rect->dtdy;
						if (t > tile[rect->tilenum].th << 8)
							t = tile[rect->tilenum].tl << 8;
					}
					break;
				}

				default:	fatalerror("RDP: texture_rectangle 32-bit RGBA: tile pixel size = %d\n", tile[rect->tilenum].size); break;
			}
			break;
		}

		case 2:		// Color Index
		{
			switch (tile[rect->tilenum].size)
			{
				case PIXEL_SIZE_4BIT:
				{
					UINT8 *tc = (UINT8*)texture_cache;
					for (j=y1; j < y2; j++)
					{
						index = j * fb_width;
						tline = tb + ((t >> 10) * twidth);

						s = rect->s << 5;		// to .10 fixed
						for (i=x1; i < x2; i++)
						{
							UINT32 color, r, g, b;
							UINT8 pix = tc[(tline + ((s >> 10) / 2)) ^ BYTE_ADDR_XOR];
							pix = ((s >> 10) & 1) ? (pix >> 0) & 0xf : (pix >> 4) & 0xf;
							color = tlut[pix^1];

							if (other_modes & OTHER_MODES_TLUT_TYPE)
							{
								// 1A (8/8)
								int lum = (color >> 8) & 0xff;
						//      int alpha = (color & 0xff);
								r = g = b = lum;
							}
							else
							{
								// RGBA5551
								r = ((color >> 11) & 0x1f);
								r = (r << 3) | (r >> 5);
								g = ((color >>  6) & 0x1f);
								g = (g << 3) | (g >> 5);
								b = ((color >>  1) & 0x1f);
								b = (b << 3) | (b >> 5);
							}

							//if (color & 1)
							{
								fb[(index + i)] = (r << 16) | (g << 8) | b;
							}
							s += rect->dsdx;
						}
						t += rect->dtdy;
					}
					break;
				}
				case PIXEL_SIZE_8BIT:
				{
					UINT8 *tc = (UINT8*)texture_cache;
					for (j=y1; j < y2; j++)
					{
						UINT32 xor_mask = 0;
						index = j * fb_width;
						tline = tb + ((t >> 10) * twidth);

						if ((t >> 10) & 1)
							xor_mask = BYTE_ADDR_XOR | 4;
						else
							xor_mask = BYTE_ADDR_XOR;

						s = rect->s << 5;		// to .10 fixed
						for (i=x1; i < x2; i++)
						{
							UINT16 color;
							UINT32 r,g,b;
							UINT8 pix = tc[(tline + (s >> 10))];
							color = tlut[pix ^ 1];
							if (other_modes & OTHER_MODES_TLUT_TYPE)
							{
								// 1A (8/8)
								int lum = (color >> 8) & 0xff;
						//      int alpha = (color & 0xff);
								r = g = b = lum;
							}
							else
							{
								// RGBA5551
								r = ((color >> 11) & 0x1f);
								r = (r << 3) | (r >> 5);
								g = ((color >>  6) & 0x1f);
								g = (g << 3) | (g >> 5);
								b = ((color >>  1) & 0x1f);
								b = (b << 3) | (b >> 5);
							}
							//if (color & 1)
							{
								fb[(index + i)] = (r << 16) | (g << 8) | b;
							}
							s += rect->dsdx;
						}
						t += rect->dtdy;
					}
					break;
				}

				default:	fatalerror("RDP: texture_rectangle 32-bit CI: tile pixel size = %d\n", tile[rect->tilenum].size); break;
			}
			break;
		}

		case 3:		// IA
		{
			switch (tile[rect->tilenum].size)
			{
				default:	fatalerror("RDP: texture_rectangle 32-bit IA: tile pixel size = %d\n", tile[rect->tilenum].size); break;
			}
			break;
		}

		default:	fatalerror("RDP: texture_rectangle 32-bit: tile format = %d\n", tile[rect->tilenum].format); break;
	}
}


/*****************************************************************************/

INLINE UINT32 READ_RDP_DATA(UINT32 address)
{
	if (dp_status & 0x1)		// XBUS_DMEM_DMA enabled
	{
		return rsp_dmem[(address & 0xfff) / 4];
	}
	else
	{
		return rdram[(address / 4)];
	}
}

static const char *image_format[] = { "RGBA", "YUV", "CI", "IA", "I", "???", "???", "???" };
static const char *image_size[] = { "4-bit", "8-bit", "16-bit", "32-bit" };

static const int rdp_command_length[64] =
{
	8,			// 0x00, No Op
	8,			// 0x01, ???
	8,			// 0x02, ???
	8,			// 0x03, ???
	8,			// 0x04, ???
	8,			// 0x05, ???
	8,			// 0x06, ???
	8,			// 0x07, ???
	32,			// 0x08, Non-Shaded Triangle
	32+16,		// 0x09, Non-Shaded, Z-Buffered Triangle
	32+64,		// 0x0a, Textured Triangle
	32+64+16,	// 0x0b, Textured, Z-Buffered Triangle
	32+64,		// 0x0c, Shaded Triangle
	32+64+16,	// 0x0d, Shaded, Z-Buffered Triangle
	32+64+64,	// 0x0e, Shaded+Textured Triangle
	32+64+64+16,// 0x0f, Shaded+Textured, Z-Buffered Triangle
	8,			// 0x10, ???
	8,			// 0x11, ???
	8,			// 0x12, ???
	8,			// 0x13, ???
	8,			// 0x14, ???
	8,			// 0x15, ???
	8,			// 0x16, ???
	8,			// 0x17, ???
	8,			// 0x18, ???
	8,			// 0x19, ???
	8,			// 0x1a, ???
	8,			// 0x1b, ???
	8,			// 0x1c, ???
	8,			// 0x1d, ???
	8,			// 0x1e, ???
	8,			// 0x1f, ???
	8,			// 0x20, ???
	8,			// 0x21, ???
	8,			// 0x22, ???
	8,			// 0x23, ???
	16,			// 0x24, Texture_Rectangle
	16,			// 0x25, Texture_Rectangle_Flip
	8,			// 0x26, Sync_Load
	8,			// 0x27, Sync_Pipe
	8,			// 0x28, Sync_Tile
	8,			// 0x29, Sync_Full
	8,			// 0x2a, Set_Key_GB
	8,			// 0x2b, Set_Key_R
	8,			// 0x2c, Set_Convert
	8,			// 0x2d, Set_Scissor
	8,			// 0x2e, Set_Prim_Depth
	8,			// 0x2f, Set_Other_Modes
	8,			// 0x30, Load_TLUT
	8,			// 0x31, ???
	8,			// 0x32, Set_Tile_Size
	8,			// 0x33, Load_Block
	8,			// 0x34, Load_Tile
	8,			// 0x35, Set_Tile
	8,			// 0x36, Fill_Rectangle
	8,			// 0x37, Set_Fill_Color
	8,			// 0x38, Set_Fog_Color
	8,			// 0x39, Set_Blend_Color
	8,			// 0x3a, Set_Prim_Color
	8,			// 0x3b, Set_Env_Color
	8,			// 0x3c, Set_Combine
	8,			// 0x3d, Set_Texture_Image
	8,			// 0x3e, Set_Mask_Image
	8			// 0x3f, Set_Color_Image
};

static int rdp_dasm(char *buffer)
{
	int i;
	int tile;
	const char *format, *size;
	char sl[32], tl[32], sh[32], th[32];
	char s[32], t[32], w[32];
	char dsdx[32], dtdx[32], dwdx[32];
	char dsdy[32], dtdy[32], dwdy[32];
	char dsde[32], dtde[32], dwde[32];
	char yl[32], yh[32], ym[32], xl[32], xh[32], xm[32];
	char dxldy[32], dxhdy[32], dxmdy[32];
	char rt[32], gt[32], bt[32], at[32];
	char drdx[32], dgdx[32], dbdx[32], dadx[32];
	char drdy[32], dgdy[32], dbdy[32], dady[32];
	char drde[32], dgde[32], dbde[32], dade[32];
	UINT32 r,g,b,a;

	UINT32 cmd[64];
	UINT32 length;
	UINT32 command;

	length = rdp_cmd_ptr * 4;
	if (length < 8)
	{
		sprintf(buffer, "ERROR: length = %d\n", length);
		return 0;
	}

	cmd[0] = rdp_cmd_data[rdp_cmd_cur+0];
	cmd[1] = rdp_cmd_data[rdp_cmd_cur+1];

	tile = (w[1] >> 24) & 0x7;
	sprintf(sl, "%4.2f", (float)((cmd[0] >> 12) & 0xfff) / 4.0f);
	sprintf(tl, "%4.2f", (float)((cmd[0] >>  0) & 0xfff) / 4.0f);
	sprintf(sh, "%4.2f", (float)((cmd[1] >> 12) & 0xfff) / 4.0f);
	sprintf(th, "%4.2f", (float)((cmd[1] >>  0) & 0xfff) / 4.0f);

	format = image_format[(cmd[0] >> 21) & 0x7];
	size = image_size[(cmd[0] >> 19) & 0x3];

	r = (cmd[1] >> 24) & 0xff;
	g = (cmd[1] >> 16) & 0xff;
	b = (cmd[1] >>  8) & 0xff;
	a = (cmd[1] >>  0) & 0xff;

	command = (cmd[0] >> 24) & 0x3f;
	switch (command)
	{
		case 0x00:	sprintf(buffer, "No Op"); break;
		case 0x08:		// Tri_NoShade
		{
			int lft = (command >> 23) & 0x1;

			if (length != rdp_command_length[command])
			{
				sprintf(buffer, "ERROR: Tri_NoShade length = %d\n", length);
				return 0;
			}

			cmd[2] = rdp_cmd_data[rdp_cmd_cur+2];
			cmd[3] = rdp_cmd_data[rdp_cmd_cur+3];
			cmd[4] = rdp_cmd_data[rdp_cmd_cur+4];
			cmd[5] = rdp_cmd_data[rdp_cmd_cur+5];
			cmd[6] = rdp_cmd_data[rdp_cmd_cur+6];
			cmd[7] = rdp_cmd_data[rdp_cmd_cur+7];

			sprintf(yl,		"%4.4f", (float)((cmd[0] >>  0) & 0x1fff) / 4.0f);
			sprintf(ym,		"%4.4f", (float)((cmd[1] >> 16) & 0x1fff) / 4.0f);
			sprintf(yh,		"%4.4f", (float)((cmd[1] >>  0) & 0x1fff) / 4.0f);
			sprintf(xl,		"%4.4f", (float)(cmd[2] / 65536.0f));
			sprintf(dxldy,	"%4.4f", (float)(cmd[3] / 65536.0f));
			sprintf(xh,		"%4.4f", (float)(cmd[4] / 65536.0f));
			sprintf(dxhdy,	"%4.4f", (float)(cmd[5] / 65536.0f));
			sprintf(xm,		"%4.4f", (float)(cmd[6] / 65536.0f));
			sprintf(dxmdy,	"%4.4f", (float)(cmd[7] / 65536.0f));

					sprintf(buffer, "Tri_NoShade            %d, XL: %s, XM: %s, XH: %s, YL: %s, YM: %s, YH: %s\n", lft, xl,xm,xh,yl,ym,yh);
			break;
		}
		case 0x0c:		// Tri_Shade
		{
			int lft = (command >> 23) & 0x1;

			if (length != rdp_command_length[command])
			{
				sprintf(buffer, "ERROR: Tri_Shade length = %d\n", length);
				return 0;
			}

			for (i=2; i < 24; i++)
			{
				cmd[i] = rdp_cmd_data[i];
			}

			sprintf(yl,		"%4.4f", (float)((cmd[0] >>  0) & 0x1fff) / 4.0f);
			sprintf(ym,		"%4.4f", (float)((cmd[1] >> 16) & 0x1fff) / 4.0f);
			sprintf(yh,		"%4.4f", (float)((cmd[1] >>  0) & 0x1fff) / 4.0f);
			sprintf(xl,		"%4.4f", (float)((INT32)cmd[2] / 65536.0f));
			sprintf(dxldy,	"%4.4f", (float)((INT32)cmd[3] / 65536.0f));
			sprintf(xh,		"%4.4f", (float)((INT32)cmd[4] / 65536.0f));
			sprintf(dxhdy,	"%4.4f", (float)((INT32)cmd[5] / 65536.0f));
			sprintf(xm,		"%4.4f", (float)((INT32)cmd[6] / 65536.0f));
			sprintf(dxmdy,	"%4.4f", (float)((INT32)cmd[7] / 65536.0f));
			sprintf(rt,		"%4.4f", (float)(INT32)((cmd[8] & 0xffff0000) | ((cmd[12] >> 16) & 0xffff)) / 65536.0f);
			sprintf(gt,		"%4.4f", (float)(INT32)(((cmd[8] & 0xffff) << 16) | (cmd[12] & 0xffff)) / 65536.0f);
			sprintf(bt,		"%4.4f", (float)(INT32)((cmd[9] & 0xffff0000) | ((cmd[13] >> 16) & 0xffff)) / 65536.0f);
			sprintf(at,		"%4.4f", (float)(INT32)(((cmd[9] & 0xffff) << 16) | (cmd[13] & 0xffff)) / 65536.0f);
			sprintf(drdx,	"%4.4f", (float)(INT32)((cmd[10] & 0xffff0000) | ((cmd[14] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dgdx,	"%4.4f", (float)(INT32)(((cmd[10] & 0xffff) << 16) | (cmd[14] & 0xffff)) / 65536.0f);
			sprintf(dbdx,	"%4.4f", (float)(INT32)((cmd[11] & 0xffff0000) | ((cmd[15] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dadx,	"%4.4f", (float)(INT32)(((cmd[11] & 0xffff) << 16) | (cmd[15] & 0xffff)) / 65536.0f);
			sprintf(drde,	"%4.4f", (float)(INT32)((cmd[16] & 0xffff0000) | ((cmd[20] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dgde,	"%4.4f", (float)(INT32)(((cmd[16] & 0xffff) << 16) | (cmd[20] & 0xffff)) / 65536.0f);
			sprintf(dbde,	"%4.4f", (float)(INT32)((cmd[17] & 0xffff0000) | ((cmd[21] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dade,	"%4.4f", (float)(INT32)(((cmd[17] & 0xffff) << 16) | (cmd[21] & 0xffff)) / 65536.0f);
			sprintf(drdy,	"%4.4f", (float)(INT32)((cmd[18] & 0xffff0000) | ((cmd[22] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dgdy,	"%4.4f", (float)(INT32)(((cmd[18] & 0xffff) << 16) | (cmd[22] & 0xffff)) / 65536.0f);
			sprintf(dbdy,	"%4.4f", (float)(INT32)((cmd[19] & 0xffff0000) | ((cmd[23] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dady,	"%4.4f", (float)(INT32)(((cmd[19] & 0xffff) << 16) | (cmd[23] & 0xffff)) / 65536.0f);

			buffer+=sprintf(buffer, "Tri_Shade              %d, XL: %s, XM: %s, XH: %s, YL: %s, YM: %s, YH: %s\n", lft, xl,xm,xh,yl,ym,yh);
			buffer+=sprintf(buffer, "                              ");
			buffer+=sprintf(buffer, "                       R: %s, G: %s, B: %s, A: %s\n", rt, gt, bt, at);
			buffer+=sprintf(buffer, "                              ");
			buffer+=sprintf(buffer, "                       DRDX: %s, DGDX: %s, DBDX: %s, DADX: %s\n", drdx, dgdx, dbdx, dadx);
			buffer+=sprintf(buffer, "                              ");
			buffer+=sprintf(buffer, "                       DRDE: %s, DGDE: %s, DBDE: %s, DADE: %s\n", drde, dgde, dbde, dade);
			buffer+=sprintf(buffer, "                              ");
			buffer+=sprintf(buffer, "                       DRDY: %s, DGDY: %s, DBDY: %s, DADY: %s\n", drdy, dgdy, dbdy, dady);
			break;
		}
		case 0x0e:		// Tri_TexShade
		{
			int lft = (command >> 23) & 0x1;

			if (length < rdp_command_length[command])
			{
				sprintf(buffer, "ERROR: Tri_TexShade length = %d\n", length);
				return 0;
			}

			for (i=2; i < 40; i++)
			{
				cmd[i] = rdp_cmd_data[rdp_cmd_cur+i];
			}

			sprintf(yl,		"%4.4f", (float)((cmd[0] >>  0) & 0x1fff) / 4.0f);
			sprintf(ym,		"%4.4f", (float)((cmd[1] >> 16) & 0x1fff) / 4.0f);
			sprintf(yh,		"%4.4f", (float)((cmd[1] >>  0) & 0x1fff) / 4.0f);
			sprintf(xl,		"%4.4f", (float)((INT32)cmd[2] / 65536.0f));
			sprintf(dxldy,	"%4.4f", (float)((INT32)cmd[3] / 65536.0f));
			sprintf(xh,		"%4.4f", (float)((INT32)cmd[4] / 65536.0f));
			sprintf(dxhdy,	"%4.4f", (float)((INT32)cmd[5] / 65536.0f));
			sprintf(xm,		"%4.4f", (float)((INT32)cmd[6] / 65536.0f));
			sprintf(dxmdy,	"%4.4f", (float)((INT32)cmd[7] / 65536.0f));
			sprintf(rt,		"%4.4f", (float)(INT32)((cmd[8] & 0xffff0000) | ((cmd[12] >> 16) & 0xffff)) / 65536.0f);
			sprintf(gt,		"%4.4f", (float)(INT32)(((cmd[8] & 0xffff) << 16) | (cmd[12] & 0xffff)) / 65536.0f);
			sprintf(bt,		"%4.4f", (float)(INT32)((cmd[9] & 0xffff0000) | ((cmd[13] >> 16) & 0xffff)) / 65536.0f);
			sprintf(at,		"%4.4f", (float)(INT32)(((cmd[9] & 0xffff) << 16) | (cmd[13] & 0xffff)) / 65536.0f);
			sprintf(drdx,	"%4.4f", (float)(INT32)((cmd[10] & 0xffff0000) | ((cmd[14] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dgdx,	"%4.4f", (float)(INT32)(((cmd[10] & 0xffff) << 16) | (cmd[14] & 0xffff)) / 65536.0f);
			sprintf(dbdx,	"%4.4f", (float)(INT32)((cmd[11] & 0xffff0000) | ((cmd[15] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dadx,	"%4.4f", (float)(INT32)(((cmd[11] & 0xffff) << 16) | (cmd[15] & 0xffff)) / 65536.0f);
			sprintf(drde,	"%4.4f", (float)(INT32)((cmd[16] & 0xffff0000) | ((cmd[20] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dgde,	"%4.4f", (float)(INT32)(((cmd[16] & 0xffff) << 16) | (cmd[20] & 0xffff)) / 65536.0f);
			sprintf(dbde,	"%4.4f", (float)(INT32)((cmd[17] & 0xffff0000) | ((cmd[21] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dade,	"%4.4f", (float)(INT32)(((cmd[17] & 0xffff) << 16) | (cmd[21] & 0xffff)) / 65536.0f);
			sprintf(drdy,	"%4.4f", (float)(INT32)((cmd[18] & 0xffff0000) | ((cmd[22] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dgdy,	"%4.4f", (float)(INT32)(((cmd[18] & 0xffff) << 16) | (cmd[22] & 0xffff)) / 65536.0f);
			sprintf(dbdy,	"%4.4f", (float)(INT32)((cmd[19] & 0xffff0000) | ((cmd[23] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dady,	"%4.4f", (float)(INT32)(((cmd[19] & 0xffff) << 16) | (cmd[23] & 0xffff)) / 65536.0f);

			sprintf(s,		"%4.4f", (float)(INT32)((cmd[24] & 0xffff0000) | ((cmd[28] >> 16) & 0xffff)) / 65536.0f);
			sprintf(t,		"%4.4f", (float)(INT32)(((cmd[24] & 0xffff) << 16) | (cmd[28] & 0xffff)) / 65536.0f);
			sprintf(w,		"%4.4f", (float)(INT32)((cmd[25] & 0xffff0000) | ((cmd[29] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dsdx,	"%4.4f", (float)(INT32)((cmd[26] & 0xffff0000) | ((cmd[30] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dtdx,	"%4.4f", (float)(INT32)(((cmd[26] & 0xffff) << 16) | (cmd[30] & 0xffff)) / 65536.0f);
			sprintf(dwdx,	"%4.4f", (float)(INT32)((cmd[27] & 0xffff0000) | ((cmd[31] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dsde,	"%4.4f", (float)(INT32)((cmd[32] & 0xffff0000) | ((cmd[36] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dtde,	"%4.4f", (float)(INT32)(((cmd[32] & 0xffff) << 16) | (cmd[36] & 0xffff)) / 65536.0f);
			sprintf(dwde,	"%4.4f", (float)(INT32)((cmd[33] & 0xffff0000) | ((cmd[37] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dsdy,	"%4.4f", (float)(INT32)((cmd[34] & 0xffff0000) | ((cmd[38] >> 16) & 0xffff)) / 65536.0f);
			sprintf(dtdy,	"%4.4f", (float)(INT32)(((cmd[34] & 0xffff) << 16) | (cmd[38] & 0xffff)) / 65536.0f);
			sprintf(dwdy,	"%4.4f", (float)(INT32)((cmd[35] & 0xffff0000) | ((cmd[39] >> 16) & 0xffff)) / 65536.0f);


			buffer+=sprintf(buffer, "Tri_TexShade           %d, XL: %s, XM: %s, XH: %s, YL: %s, YM: %s, YH: %s\n", lft, xl,xm,xh,yl,ym,yh);
			buffer+=sprintf(buffer, "                              ");
			buffer+=sprintf(buffer, "                       R: %s, G: %s, B: %s, A: %s\n", rt, gt, bt, at);
			buffer+=sprintf(buffer, "                              ");
			buffer+=sprintf(buffer, "                       DRDX: %s, DGDX: %s, DBDX: %s, DADX: %s\n", drdx, dgdx, dbdx, dadx);
			buffer+=sprintf(buffer, "                              ");
			buffer+=sprintf(buffer, "                       DRDE: %s, DGDE: %s, DBDE: %s, DADE: %s\n", drde, dgde, dbde, dade);
			buffer+=sprintf(buffer, "                              ");
			buffer+=sprintf(buffer, "                       DRDY: %s, DGDY: %s, DBDY: %s, DADY: %s\n", drdy, dgdy, dbdy, dady);

			buffer+=sprintf(buffer, "                              ");
			buffer+=sprintf(buffer, "                       S: %s, T: %s, W: %s\n", s, t, w);
			buffer+=sprintf(buffer, "                              ");
			buffer+=sprintf(buffer, "                       DSDX: %s, DTDX: %s, DWDX: %s\n", dsdx, dtdx, dwdx);
			buffer+=sprintf(buffer, "                              ");
			buffer+=sprintf(buffer, "                       DSDE: %s, DTDE: %s, DWDE: %s\n", dsde, dtde, dwde);
			buffer+=sprintf(buffer, "                              ");
			buffer+=sprintf(buffer, "                       DSDY: %s, DTDY: %s, DWDY: %s\n", dsdy, dtdy, dwdy);
			break;
		}
		case 0x24:
		case 0x25:
		{
			if (length != 16)
			{
				sprintf(buffer, "ERROR: Texture_Rectangle length = %d\n", length);
				return 0;
			}
			cmd[2] = rdp_cmd_data[rdp_cmd_cur+2];
			cmd[3] = rdp_cmd_data[rdp_cmd_cur+3];
			sprintf(s,    "%4.4f", (float)(INT16)((cmd[2] >> 16) & 0xffff) / 32.0f);
			sprintf(t,    "%4.4f", (float)(INT16)((cmd[2] >>  0) & 0xffff) / 32.0f);
			sprintf(dsdx, "%4.4f", (float)(INT16)((cmd[3] >> 16) & 0xffff) / 1024.0f);
			sprintf(dtdy, "%4.4f", (float)(INT16)((cmd[3] >> 16) & 0xffff) / 1024.0f);

			if (command == 0x24)
					sprintf(buffer, "Texture_Rectangle      %d, %s, %s, %s, %s,  %s, %s, %s, %s", tile, sh, th, sl, tl, s, t, dsdx, dtdy);
			else
					sprintf(buffer, "Texture_Rectangle_Flip %d, %s, %s, %s, %s,  %s, %s, %s, %s", tile, sh, th, sl, tl, s, t, dsdx, dtdy);

			break;
		}
		case 0x26:	sprintf(buffer, "Sync_Load"); break;
		case 0x27:	sprintf(buffer, "Sync_Pipe"); break;
		case 0x28:	sprintf(buffer, "Sync_Tile"); break;
		case 0x29:	sprintf(buffer, "Sync_Full"); break;
		case 0x2d:	sprintf(buffer, "Set_Scissor            %s, %s, %s, %s", sl, tl, sh, th); break;
		case 0x2e:	sprintf(buffer, "Set_Prim_Depth         %04X, %04X", (cmd[1] >> 16) & 0xffff, cmd[1] & 0xffff); break;
		case 0x2f:	sprintf(buffer, "Set_Other_Modes        %08X %08X", cmd[0], cmd[1]); break;
		case 0x30:	sprintf(buffer, "Load_TLUT              %d, %s, %s, %s, %s", tile, sl, tl, sh, th); break;
		case 0x32:	sprintf(buffer, "Set_Tile_Size          %d, %s, %s, %s, %s", tile, sl, tl, sh, th); break;
		case 0x33:	sprintf(buffer, "Load_Block             %d, %03X, %03X, %03X, %03X", tile, (cmd[0] >> 12) & 0xfff, cmd[0] & 0xfff, (cmd[1] >> 12) & 0xfff, cmd[1] & 0xfff); break;
		case 0x34:	sprintf(buffer, "Load_Tile              %d, %s, %s, %s, %s", tile, sl, tl, sh, th); break;
		case 0x35:	sprintf(buffer, "Set_Tile               %d, %s, %s, %d, %04X", tile, format, size, ((cmd[0] >> 9) & 0x1ff) * 8, (cmd[0] & 0x1ff) * 8); break;
		case 0x36:	sprintf(buffer, "Fill_Rectangle         %s, %s, %s, %s", sh, th, sl, tl); break;
		case 0x37:	sprintf(buffer, "Set_Fill_Color         R: %d, G: %d, B: %d, A: %d", r, g, b, a); break;
		case 0x38:	sprintf(buffer, "Set_Fog_Color          R: %d, G: %d, B: %d, A: %d", r, g, b, a); break;
		case 0x39:	sprintf(buffer, "Set_Blend_Color        R: %d, G: %d, B: %d, A: %d", r, g, b, a); break;
		case 0x3a:	sprintf(buffer, "Set_Prim_Color         %d, %d, R: %d, G: %d, B: %d, A: %d", (cmd[0] >> 8) & 0x1f, cmd[0] & 0xff, r, g, b, a); break;
		case 0x3b:	sprintf(buffer, "Set_Env_Color          R: %d, G: %d, B: %d, A: %d", r, g, b, a); break;
		case 0x3c:	sprintf(buffer, "Set_Combine            %08X %08X", cmd[0], cmd[1]); break;
		case 0x3d:	sprintf(buffer, "Set_Texture_Image      %s, %s, %d, %08X", format, size, (cmd[0] & 0x1ff)+1, cmd[1]); break;
		case 0x3e:	sprintf(buffer, "Set_Mask_Image         %08X", cmd[1]); break;
		case 0x3f:	sprintf(buffer, "Set_Color_Image        %s, %s, %d, %08X", format, size, (cmd[0] & 0x1ff)+1, cmd[1]); break;
		default:	sprintf(buffer, "??? (%08X %08X)", cmd[0], cmd[1]); break;
	}

	return rdp_command_length[command];
}

/*****************************************************************************/

static void triangle(UINT32 w1, UINT32 w2, int shade)
{
	int i, j;
	int xleft, xright, xleft_inc, xright_inc;
	int xstart, xend;
	UINT16 *fb = (UINT16*)&rdram[fb_address / 4];
	UINT16 color;

	INT32 yl, ym, yh;
	INT32 xl, xm, xh;
	INT64 dxldy, dxhdy, dxmdy;
	UINT32 w3, w4, w5, w6, w7, w8;

	w3 = rdp_cmd_data[rdp_cmd_cur+2];
	w4 = rdp_cmd_data[rdp_cmd_cur+3];
	w5 = rdp_cmd_data[rdp_cmd_cur+4];
	w6 = rdp_cmd_data[rdp_cmd_cur+5];
	w7 = rdp_cmd_data[rdp_cmd_cur+6];
	w8 = rdp_cmd_data[rdp_cmd_cur+7];

	yl = (w1 & 0x1fff) >> 2;
	ym = ((w2 >> 16) & 0x1fff) >> 2;
	yh = ((w2 >>  0) & 0x1fff) >> 2;
	xl = (INT32)(w3);
	xh = (INT32)(w5);
	xm = (INT32)(w7);
	dxldy = (INT32)(w4);
	dxhdy = (INT32)(w6);
	dxmdy = (INT32)(w8);

	color = 0xffff;

	if (shade)
	{
		UINT32 shade1 = rdp_cmd_data[rdp_cmd_cur+8];
		UINT32 shade2 = rdp_cmd_data[rdp_cmd_cur+9];
		int r = (shade1 >> 16) & 0xff;
		int g = (shade1 >>  0) & 0xff;
		int b = (shade2 >> 16) & 0xff;
		r >>= 3;
		g >>= 3;
		b >>= 3;
		color = r << 11 | g << 6 | b << 1;
	}

	if ((w1 & 0x800000) == 0)
	{
		xleft = xm;
		xright = xh;
		xleft_inc = dxmdy;
		xright_inc = dxhdy;
	}
	else
	{
		xright = xm;
		xleft = xh;
		xright_inc = dxmdy;
		xleft_inc = dxhdy;
	}



	for (j=yh; j < ym; j++)
	{
		xstart = xleft >> 16;
		if ((xleft & 0xffff) >= 0x8000)
			xstart++;

		xend = xright >> 16;
		if ((xright & 0xffff) >= 0x8000)
			xend++;

		for (i=xstart; i <= xend; i++)
		{
			//if (i >= (clip.xh >> 2) && i < (clip.xl >> 2) && j >= (clip.yh >> 2) && j < (clip.yl >> 2))
			fb[(j * fb_width + i) ^ 1] = color;
		}
		xleft += xleft_inc;
		xright += xright_inc;
	}

	if ((w1 & 0x800000) == 0)
	{
		xleft = xl;
		xleft_inc = dxldy;
		xright_inc = dxhdy;
	}
	else
	{
		xright = xl;
		xright_inc = dxldy;
		xleft_inc = dxhdy;
	}

	for (j=ym; j <= yl; j++)
	{
		xstart = xleft >> 16;
		if ((xleft & 0xffff) >= 0x8000)
			xstart++;

		xend = xright >> 16;
		if ((xright & 0xffff) >= 0x8000)
			xend++;

		for (i=xstart; i <= xend; i++)
		{
			//if (i >= (clip.xh >> 2) && i < (clip.xl >> 2) && j >= (clip.yh >> 2) && j < (clip.yl >> 2))
			fb[(j * fb_width + i) ^ 1] = color;
		}
		xleft += xleft_inc;
		xright += xright_inc;
	}
}

////////////////////////
// RDP COMMANDS
////////////////////////

static void rdp_invalid(UINT32 w1, UINT32 w2)
{
	fatalerror("RDP: invalid command  %d, %08X %08X\n", (w1 >> 24) & 0x3f, w1, w2);
}

static void rdp_noop(UINT32 w1, UINT32 w2)
{

}

static void rdp_tri_noshade(UINT32 w1, UINT32 w2)
{
	//fatalerror("RDP: unhandled command tri_noshade, %08X %08X\n", w1, w2);
	triangle(w1, w2, 0);
}

static void rdp_tri_noshade_z(UINT32 w1, UINT32 w2)
{
	//fatalerror("RDP: unhandled command tri_noshade_z, %08X %08X\n", w1, w2);
	triangle(w1, w2, 0);
}

static void rdp_tri_tex(UINT32 w1, UINT32 w2)
{
	//osd_die("RDP: unhandled command tri_tex, %08X %08X\n", w1, w2);

	triangle(w1, w2, 0);
}

static void rdp_tri_tex_z(UINT32 w1, UINT32 w2)
{
	//osd_die("RDP: unhandled command tri_tex_z, %08X %08X\n", w1, w2);

	triangle(w1, w2, 0);
}

static void rdp_tri_shade(UINT32 w1, UINT32 w2)
{
	//osd_die("RDP: unhandled command tri_shade, %08X %08X\n", w1, w2);

	triangle(w1, w2, 1);
}

static void rdp_tri_shade_z(UINT32 w1, UINT32 w2)
{
	//osd_die("RDP: unhandled command tri_shade_z, %08X %08X\n", w1, w2);

	triangle(w1, w2, 1);
}

static void rdp_tri_texshade(UINT32 w1, UINT32 w2)
{
	//osd_die("RDP: unhandled command tri_texshade, %08X %08X\n", w1, w2);

	triangle(w1, w2, 1);
}

static void rdp_tri_texshade_z(UINT32 w1, UINT32 w2)
{
	//osd_die("RDP: unhandled command tri_texshade_z, %08X %08X\n", w1, w2);

	triangle(w1, w2, 1);
}

static void rdp_tex_rect(UINT32 w1, UINT32 w2)
{
	UINT32 w3, w4;
	TEX_RECTANGLE rect;

	w3 = rdp_cmd_data[rdp_cmd_cur+2];
	w4 = rdp_cmd_data[rdp_cmd_cur+3];

	rect.tilenum	= (w2 >> 24) & 0x7;
	rect.xl			= (w1 >> 12) & 0xfff;
	rect.yl			= (w1 >>  0) & 0xfff;
	rect.xh			= (w2 >> 12) & 0xfff;
	rect.yh			= (w2 >>  0) & 0xfff;
	rect.s			= (w3 >> 16) & 0xffff;
	rect.t			= (w3 >>  0) & 0xffff;
	rect.dsdx		= (w4 >> 16) & 0xffff;
	rect.dtdy		= (w4 >>  0) & 0xffff;

	switch (fb_size)
	{
		case PIXEL_SIZE_16BIT:		texture_rectangle_16bit(&rect); break;
		case PIXEL_SIZE_32BIT:		texture_rectangle_32bit(&rect); break;
	}
}

static void rdp_tex_rect_flip(UINT32 w1, UINT32 w2)
{
	fatalerror("RDP: unhandled command tex_rect_flip, %08X %08X\n", w1, w2);
}

static void rdp_sync_load(UINT32 w1, UINT32 w2)
{
	// Nothing to do?
}

static void rdp_sync_pipe(UINT32 w1, UINT32 w2)
{
	// Nothing to do?
}

static void rdp_sync_tile(UINT32 w1, UINT32 w2)
{
	// Nothing to do?
}

static void rdp_sync_full(UINT32 w1, UINT32 w2)
{
	dp_full_sync();
}

static void rdp_set_key_gb(UINT32 w1, UINT32 w2)
{
	//osd_die("RDP: unhandled command set_key_gb, %08X %08X\n", w1, w2);
}

static void rdp_set_key_r(UINT32 w1, UINT32 w2)
{
	//osd_die("RDP: unhandled command set_key_r, %08X %08X\n", w1, w2);
}

static void rdp_set_convert(UINT32 w1, UINT32 w2)
{
	//osd_die("RDP: unhandled command set_convert, %08X %08X\n", w1, w2);
}

static void rdp_set_scissor(UINT32 w1, UINT32 w2)
{
	clip.xh = (w1 >> 12) & 0xfff;
	clip.yh = (w1 >>  0) & 0xfff;
	clip.xl = (w2 >> 12) & 0xfff;
	clip.yl = (w2 >>  0) & 0xfff;

	// TODO: handle f & o?
}

static void rdp_set_prim_depth(UINT32 w1, UINT32 w2)
{
	primitive_z = (UINT16)(w2 >> 16);
	primitive_delta_z = (UINT16)(w1);
}

static void rdp_set_other_modes(UINT32 w1, UINT32 w2)
{
	other_modes = ((UINT64)(w1) << 32) | w2;
}

static void rdp_load_tlut(UINT32 w1, UINT32 w2)
{
	int i;
	UINT16 sl, sh;
//  int tilenum = (w2 >> 24) & 0x7;

	sl	= ((w1 >> 12) & 0xfff) / 4;
	sh	= ((w2 >> 12) & 0xfff) / 4;

	switch (ti_format)
	{
		case 0:		// RGBA
		{
			switch (ti_size)
			{
				case PIXEL_SIZE_16BIT:
				{
					UINT16 *src = (UINT16*)&rdram[ti_address / 4];
	//              UINT16 *tlut = (UINT16*)&texture_cache[tile[tilenum].tmem/2];

					for (i=sl; i <= sh; i++)
					{
						tlut[i] = src[i];
			//          printf("TLUT %d = %04X\n", i, tlut[i]);
					}
					break;
				}
				default:	fatalerror("RDP: load_tlut: size = %d\n", ti_size);
			}
			break;
		}

		default:	fatalerror("RDP: load_tlut: format = %d\n", ti_format);
	}

}

static void rdp_set_tile_size(UINT32 w1, UINT32 w2)
{
	int tilenum = (w2 >> 24) & 0x7;

	tile[tilenum].sl = (w1 >> 12) & 0xfff;
	tile[tilenum].tl = (w1 >>  0) & 0xfff;
	tile[tilenum].sh = (w2 >> 12) & 0xfff;
	tile[tilenum].th = (w2 >>  0) & 0xfff;
}

static void rdp_load_block(UINT32 w1, UINT32 w2)
{
	int i, c;
	UINT16 sl, sh, tl, dxt;
	int tilenum = (w2 >> 24) & 0x7;

	if (ti_format != tile[tilenum].format || ti_size != tile[tilenum].size)
		fatalerror("RDP: load_block: format conversion required!\n");

	sl	= ((w1 >> 12) & 0xfff);
	tl	= ((w1 >>  0) & 0xfff) << 11;
	sh	= ((w2 >> 12) & 0xfff);
	dxt	= ((w2 >>  0) & 0xfff);

	switch (ti_size)
	{
		case PIXEL_SIZE_4BIT:
		{
			UINT8 *src = (UINT8*)&rdram[ti_address / 4];
			UINT8 *tc = (UINT8*)texture_cache;
			int tb = tile[tilenum].tmem;

			c=0;
			for (i=sl>>1; i <= sh>>1; i++)
			{
				int tline = tb + ((tile[tilenum].line) * (tl >> 11));
				tc[(tline+i) ^ BYTE_ADDR_XOR] = src[i ^ BYTE_ADDR_XOR];
				//tc[tb+i] = src[i^3];
				tl += dxt;

				c++;
				if ((c & 7)	== 7)
				{
					tl += dxt;		// TL is increased every 8 bytes (8 8-bit pixels)
				}
			}
			break;
		}
		case PIXEL_SIZE_8BIT:
		{
			UINT8 *src = (UINT8*)&rdram[ti_address / 4];
			UINT8 *tc = (UINT8*)texture_cache;
			int tb = tile[tilenum].tmem;

			c=0;
			for (i=sl; i <= sh; i++)
			{
				int tline = tb + ((tile[tilenum].line) * (tl >> 11));
				tc[(tline+i) ^ BYTE_ADDR_XOR] = src[i ^ BYTE_ADDR_XOR];
				//tc[tb+i] = src[i^3];

				c++;
				if ((c & 7)	== 7)
				{
					tl += dxt;		// TL is increased every 8 bytes (8 8-bit pixels)
				}
			}
			break;
		}
		case PIXEL_SIZE_16BIT:
		{
			UINT16 *src = (UINT16*)&rdram[ti_address / 4];
			UINT16 *tc = (UINT16*)texture_cache;
			int tb = tile[tilenum].tmem / 2;
			int tline = 0;

			c=0;
			for (i=sl; i <= sh; i++)
			{
				tline = tb + ((tile[tilenum].line/2) * (tl >> 11));
				tc[(tline+i) ^ WORD_ADDR_XOR] = src[i ^ WORD_ADDR_XOR];
				//tc[tb+i] = *src++;

				c++;
				if ((c & 3)	== 3)
				{
					tl += dxt;		// TL is increased every 8 bytes (4 16-bit pixels)
				}
			}

			break;
		}
		case PIXEL_SIZE_32BIT:
		{
			UINT32 *src = (UINT32*)&rdram[ti_address / 4];
			UINT32 *tc = (UINT32*)texture_cache;
			int tb = tile[tilenum].tmem / 4;

			c=0;
			for (i=sl; i <= sh; i++)
			{
				int tline = tb + ((tile[tilenum].line / 4) * (tl >> 11));
				tc[(tline+i)] = src[i];
				//tc[tb+i] = *src++;

				c++;
				if ((c & 1)	== 1)
				{
					tl += dxt;		// TL is increased every 8 bytes (2 32-bit pixels)
				}
			}
			break;
		}

		default:	fatalerror("RDP: load_block: size = %d\n", ti_format);
	}
}

static void rdp_load_tile(UINT32 w1, UINT32 w2)
{
	int i, j;
	UINT16 sl, sh, tl, th;
	int tilenum = (w2 >> 24) & 0x7;

	if (ti_format != tile[tilenum].format || ti_size != tile[tilenum].size)
		fatalerror("RDP: load_block: format conversion required!\n");

	sl	= ((w1 >> 12) & 0xfff) / 4;
	tl	= ((w1 >>  0) & 0xfff) / 4;
	sh	= ((w2 >> 12) & 0xfff) / 4;
	th	= ((w2 >>  0) & 0xfff) / 4;

	switch (ti_size)
	{
		case PIXEL_SIZE_8BIT:
		{
			UINT8 *src = (UINT8*)&rdram[ti_address / 4];
			UINT8 *tc = (UINT8*)texture_cache;
			int tb = tile[tilenum].tmem;

			for (j=tl; j <= th; j++)
			{
				int tline = tb + (tile[tilenum].line * j);
				int sline = ti_width * j;

				for (i=sl; i <= sh; i++)
				{
					tc[(tline+i) ^ BYTE_ADDR_XOR] = src[(sline+i) ^ BYTE_ADDR_XOR];
				}
			}
			break;
		}
		case PIXEL_SIZE_16BIT:
		{
			UINT16 *src = (UINT16*)&rdram[ti_address / 4];
			UINT16 *tc = (UINT16*)texture_cache;
			int tb = (tile[tilenum].tmem / 2);

			for (j=tl; j <= th; j++)
			{
				int tline = tb + ((tile[tilenum].line / 2) * j);
				int sline = ti_width * j;

				for (i=sl; i <= sh; i++)
				{
					tc[(tline+i) ^ WORD_ADDR_XOR] = src[(sline+i) ^ WORD_ADDR_XOR];
				}
			}
			break;
		}
		case PIXEL_SIZE_32BIT:
		{
			UINT32 *src = (UINT32*)&rdram[ti_address / 4];
			UINT32 *tc = (UINT32*)texture_cache;
			int tb = (tile[tilenum].tmem / 4);

			for (j=tl; j <= th; j++)
			{
				int tline = tb + ((tile[tilenum].line / 4) * j);
				int sline = ti_width * j;

				for (i=sl; i <= sh; i++)
				{
					tc[(tline+i)] = src[(sline+i)];
				}
			}
			break;
		}

		default:	fatalerror("RDP: load_tile: size = %d\n", ti_size);
	}
}

static void rdp_set_tile(UINT32 w1, UINT32 w2)
{
	int tilenum = (w2 >> 24) & 0x7;

	tile[tilenum].format	= (w1 >> 21) & 0x7;
	tile[tilenum].size		= (w1 >> 19) & 0x3;
	tile[tilenum].line		= ((w1 >>  9) & 0x1ff) * 8;
	tile[tilenum].tmem		= ((w1 >>  0) & 0x1ff) * 8;

	// TODO: clamp & mirror parameters
}

static void rdp_fill_rect(UINT32 w1, UINT32 w2)
{
	RECTANGLE rect;
	rect.xl = (w1 >> 12) & 0xfff;
	rect.yl = (w1 >>  0) & 0xfff;
	rect.xh = (w2 >> 12) & 0xfff;
	rect.yh = (w2 >>  0) & 0xfff;

	switch (fb_size)
	{
		case PIXEL_SIZE_16BIT:		fill_rectangle_16bit(&rect); break;
		case PIXEL_SIZE_32BIT:		fill_rectangle_32bit(&rect); break;
	}
}

static void rdp_set_fill_color(UINT32 w1, UINT32 w2)
{
	fill_color = w2;
}

static void rdp_set_fog_color(UINT32 w1, UINT32 w2)
{
	fog_color.r = (w2 >> 24) & 0xff;
	fog_color.g = (w2 >> 16) & 0xff;
	fog_color.b = (w2 >>  8) & 0xff;
	fog_color.a = (w2 >>  0) & 0xff;
}

static void rdp_set_blend_color(UINT32 w1, UINT32 w2)
{
	blend_color.r = (w2 >> 24) & 0xff;
	blend_color.g = (w2 >> 16) & 0xff;
	blend_color.b = (w2 >>  8) & 0xff;
	blend_color.a = (w2 >>  0) & 0xff;
}

static void rdp_set_prim_color(UINT32 w1, UINT32 w2)
{
	// TODO: prim min level, prim_level
	prim_color.r = (w2 >> 24) & 0xff;
	prim_color.g = (w2 >> 16) & 0xff;
	prim_color.b = (w2 >>  8) & 0xff;
	prim_color.a = (w2 >>  0) & 0xff;
}

static void rdp_set_env_color(UINT32 w1, UINT32 w2)
{
	env_color.r = (w2 >> 24) & 0xff;
	env_color.g = (w2 >> 16) & 0xff;
	env_color.b = (w2 >>  8) & 0xff;
	env_color.a = (w2 >>  0) & 0xff;
}

static void rdp_set_combine(UINT32 w1, UINT32 w2)
{
	//TODO!

}

static void rdp_set_texture_image(UINT32 w1, UINT32 w2)
{
	ti_format	= (w1 >> 21) & 0x7;
	ti_size		= (w1 >> 19) & 0x3;
	ti_width	= (w1 & 0x3ff) + 1;
	ti_address	= w2 & 0x01ffffff;
}

static void rdp_set_mask_image(UINT32 w1, UINT32 w2)
{
	zb_address	= w2 & 0x01ffffff;
}

static void rdp_set_color_image(UINT32 w1, UINT32 w2)
{
	fb_format 	= (w1 >> 21) & 0x7;
	fb_size		= (w1 >> 19) & 0x3;
	fb_width	= (w1 & 0x3ff) + 1;
	fb_address	= w2 & 0x01ffffff;

	//if (fb_format != 0)   fatalerror("rdp_set_color_image: format = %d\n", fb_format);
	if (fb_format != 0) fb_format = 0;
}

/*****************************************************************************/

static void (* rdp_command_table[64])(UINT32 w1, UINT32 w2) =
{
	/* 0x00 */
	rdp_noop,			rdp_invalid,			rdp_invalid,			rdp_invalid,
	rdp_invalid,		rdp_invalid,			rdp_invalid,			rdp_invalid,
	rdp_tri_noshade,	rdp_tri_noshade_z,		rdp_tri_tex,			rdp_tri_tex_z,
	rdp_tri_shade,		rdp_tri_shade_z,		rdp_tri_texshade,		rdp_tri_texshade_z,
	/* 0x10 */
	rdp_invalid,		rdp_invalid,			rdp_invalid,			rdp_invalid,
	rdp_invalid,		rdp_invalid,			rdp_invalid,			rdp_invalid,
	rdp_invalid,		rdp_invalid,			rdp_invalid,			rdp_invalid,
	rdp_invalid,		rdp_invalid,			rdp_invalid,			rdp_invalid,
	/* 0x20 */
	rdp_invalid,		rdp_invalid,			rdp_invalid,			rdp_invalid,
	rdp_tex_rect,		rdp_tex_rect_flip,		rdp_sync_load,			rdp_sync_pipe,
	rdp_sync_tile,		rdp_sync_full,			rdp_set_key_gb,			rdp_set_key_r,
	rdp_set_convert,	rdp_set_scissor,		rdp_set_prim_depth,		rdp_set_other_modes,
	/* 0x30 */
	rdp_load_tlut,		rdp_invalid,			rdp_set_tile_size,		rdp_load_block,
	rdp_load_tile,		rdp_set_tile,			rdp_fill_rect,			rdp_set_fill_color,
	rdp_set_fog_color,	rdp_set_blend_color,	rdp_set_prim_color,		rdp_set_env_color,
	rdp_set_combine,	rdp_set_texture_image,	rdp_set_mask_image,		rdp_set_color_image
};

void rdp_process_list(void)
{
	int i;
	UINT32 cmd, length, cmd_length;

	length = dp_end - dp_current;

	// load command data
	for (i=0; i < length; i += 4)
	{
		rdp_cmd_data[rdp_cmd_ptr++] = READ_RDP_DATA(dp_current + i);
	}

	dp_current = dp_end;

	cmd = (rdp_cmd_data[0] >> 24) & 0x3f;
	cmd_length = (rdp_cmd_ptr + 1) * 4;

	// check if more data is needed
	if (cmd_length < rdp_command_length[cmd])
	{
		return;
	}

	while (rdp_cmd_cur < rdp_cmd_ptr)
	{
		cmd = (rdp_cmd_data[rdp_cmd_cur] >> 24) & 0x3f;

#if LOG_RDP_EXECUTION
		{
			char string[4000];
			int l = rdp_dasm(string);

			fprintf(rdp_exec, "%08X: %08X %08X   %s\n", dp_start+(rdp_cmd_cur * 4), rdp_cmd_data[rdp_cmd_cur+0], rdp_cmd_data[rdp_cmd_cur+1], string);

			if (l < cmd_length)
			{
				fprintf(rdp_exec, "%08X extra words...\n", cmd_length-l);
			}
		}
#endif

		// execute the command
		rdp_command_table[cmd](rdp_cmd_data[rdp_cmd_cur+0], rdp_cmd_data[rdp_cmd_cur+1]);

		rdp_cmd_cur += rdp_command_length[cmd] / 4;
	};
	rdp_cmd_ptr = 0;
	rdp_cmd_cur = 0;

	dp_start = dp_end;
}

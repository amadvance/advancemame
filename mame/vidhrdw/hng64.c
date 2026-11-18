// license:LGPL-2.1+
// copyright-holders:David Haywood, Angelo Salese, ElSemi, Andrew Gardner

#include "driver.h"
#include "includes/hng64.h"
#include "polylgcy.h"

#include <math.h>
#define BITMAP_ADDR16(bitmap, y, x) (&( (UINT16 *)(bitmap)->line[y])[x])
#define BITMAP_ADDR32(bitmap, y, x) (&( (UINT32 *)(bitmap)->line[y])[x])

UINT32 hng64_packets[HNG64_MAX_PACKETS][8];
unsigned hng64_packet_count;


UINT32* hng64_videoram;

UINT32 *hng64_spriteram, *hng64_spriteregs;
UINT32 *hng64_videoregs;

UINT32 *hng64_tcram;
UINT8 hng64_texture_wrap[32];
UINT32 *hng64_fbregs;
int hng64_roadedge_3d, hng64_samurai_3d;


static void  matmul4( float *product, const float *a, const float *b ) ;
static void  vecmatmul4( float *product, const float *a, const float *b) ;

static float uToF(UINT16 input) ;
static void SetIdentity(float *matrix) ;

static float *depthBuffer;
static UINT16 *hng64_3d_buffer;
static const UINT8 *hng64_texture_rom;
static poly_manager *hng64_poly;
static struct polygon *polys ;

#define WORD_AT(buffer, offset) (((const UINT16 *)(buffer))[(offset) / 2])

static mame_bitmap *hng64_sprite_bitmap, *hng64_sprite_depth;
static unsigned hng64_sprite_groups;

static int hng64_sprite_position(UINT32 raw)
{
	unsigned pos = raw & 0x3ff;
	return (pos & 0x200) ? (int)pos - 0x400 : pos;
}

static void hng64_draw_sprites(const rectangle *clip)
{
	const unsigned entries = 0xc000 / 32;
	const int reverse_z = (hng64_spriteregs[0] & 0x01000000) != 0;
	const int four_bit = (hng64_spriteregs[0] & 0x00800000) != 0;
	const unsigned zoom_shift = hng64_spriteregs[0] & 0x08000000 ? 4 : 8;
	const gfx_element *gfx = Machine->gfx[four_bit ? 4 : 5];
	unsigned entry, next;

	hng64_sprite_groups = 0;
	fillbitmap(hng64_sprite_bitmap, 0, clip);
	fillbitmap(hng64_sprite_depth, reverse_z ? 0x7ff : 0, clip);

	for (entry = 0; entry < entries; entry = next)
	{
		const UINT32 *sprite = hng64_spriteram + entry * 8;
		unsigned flag = sprite[4];
		unsigned z = (sprite[2] >> 16) & 0x7ff;
		unsigned cols = ((sprite[2] >> 4) & 15) + 1;
		unsigned rows = (sprite[2] & 15) + 1;
		int chained = (sprite[2] & 0x100) != 0;
		unsigned dx = (sprite[1] & 0xffff) << zoom_shift;
		unsigned dy = (sprite[1] >> 16) << zoom_shift;
		int x0 = hng64_sprite_position((sprite[0] & 0xffff) + (hng64_spriteregs[1] & 0xffff));
		int y0 = hng64_sprite_position((sprite[0] >> 16) + (hng64_spriteregs[1] >> 16));
		unsigned mosaic = (flag >> 28) + 1;
		unsigned width, height;
		int x, y;

		next = entry + (chained ? cols * rows : 1);
		if (!dx || !dy || (!reverse_z && !z))
			continue;

		width = ((cols << 20) + dx - 1) / dx;
		height = ((rows << 20) + dy - 1) / dy;

		for (y = MAX(y0, clip->min_y); y <= MIN(y0 + (int)height - 1, clip->max_y); ++y)
		{
			UINT16 *pixels = (UINT16 *)hng64_sprite_bitmap->line[y];
			UINT16 *depth = (UINT16 *)hng64_sprite_depth->line[y];
			unsigned sy = (((y - y0) / mosaic) * mosaic * dy) >> 16;

			if (flag & 0x01000000)
				sy = rows * 16 - 1 - sy;

			for (x = MAX(x0, clip->min_x); x <= MIN(x0 + (int)width - 1, clip->max_x); ++x)
			{
				unsigned sx = (((x - x0) / mosaic) * mosaic * dx) >> 16;
				unsigned tile_index, code, color, pen;
				const UINT32 *tile;
				const UINT8 *row;

				if ((flag & 0x04000000) && ((x ^ y) & 1))
					continue;
				if (reverse_z ? z >= depth[x] : z < depth[x])
					continue;

				if (flag & 0x02000000)
					sx = cols * 16 - 1 - sx;
				tile_index = (sy >> 4) * cols + (sx >> 4);
				if (chained && entry + tile_index >= entries)
					continue;
				tile = sprite + (chained ? tile_index * 8 : 0);

				code = tile[4] & 0x7ffff;
				color = (tile[3] >> 16) & 0xff;
				if (!four_bit)
				{
					code >>= 1;
					color &= 0xf;
				}
				if (!chained)
					code += tile_index;
				code %= gfx->total_elements;
				row = gfx->gfxdata + code * gfx->char_modulo + (sy & 15) * gfx->line_modulo;
				if (gfx->flags & GFX_PACKED)
					pen = (row[(sx & 15) / 2] >> ((sx & 1) * 4)) & 15;
				else
					pen = row[sx & 15];
				if (pen)
				{
					pixels[x] = color * gfx->color_granularity + pen;
					pixels[x] |= (flag >> 8) & 0xf000;
					hng64_sprite_groups |= 1 << ((flag >> 20) & 7);
					depth[x] = z;
				}
			}
		}
	}
}

static void hng64_mix_sprites(mame_bitmap *bitmap, const rectangle *clip, unsigned group)
{
	int x, y;
	int alpha = hng64_tcram[0x4c / 4] & 0x10000;

	if (!(hng64_sprite_groups & (1 << (group >> 12))))
		return;

	for (y = clip->min_y; y <= clip->max_y; ++y)
	{
		const UINT16 *src = (const UINT16 *)hng64_sprite_bitmap->line[y];
		UINT32 *dst = (UINT32 *)bitmap->line[y];
		for (x = clip->min_x; x <= clip->max_x; ++x)
		{
			UINT16 pixel = src[x];
			UINT32 color;
			if ((pixel & 0x7000) != group || !(pixel & 0xfff))
				continue;
			color = Machine->pens[pixel & 0xfff];
			if (pixel & 0x8000)
			{
				UINT32 old = dst[x];
				if (alpha)
					color = ((color & 0xfefefe) >> 1) + ((old & 0xfefefe) >> 1);
				else
					color = MAKE_RGB(MIN(255, RGB_RED(color) + RGB_RED(old)),
						MIN(255, RGB_GREEN(color) + RGB_GREEN(old)),
						MIN(255, RGB_BLUE(color) + RGB_BLUE(old)));
			}
			dst[x] = color;
		}
	}
}

static void transition_control(mame_bitmap *bitmap)
{
	int i, j ;

	INT32 finR, finG, finB ;

	INT32 darkR, darkG, darkB ;
	INT32 brigR, brigG, brigB ;

	if (hng64_tcram[0x00000007] != 0x00000000 || hng64_tcram[0x0000000a] != 0x00000000)
	{
		darkR = (INT32)( hng64_tcram[0x00000007]        & 0xff) ;
		darkG = (INT32)((hng64_tcram[0x00000007] >> 8)  & 0xff) ;
		darkB = (INT32)((hng64_tcram[0x00000007] >> 16) & 0xff) ;

		brigR = (INT32)( hng64_tcram[0x0000000a]        & 0xff) ;
		brigG = (INT32)((hng64_tcram[0x0000000a] >> 8)  & 0xff) ;
		brigB = (INT32)((hng64_tcram[0x0000000a] >> 16) & 0xff) ;

		for (i = Machine->visible_area.min_x; i < Machine->visible_area.max_x; i++)
		{
			for (j = Machine->visible_area.min_y; j < Machine->visible_area.max_y; j++)
			{
				UINT32* thePixel = BITMAP_ADDR32(bitmap, j, i);

				finR = (INT32)RGB_RED(*thePixel) ;
				finG = (INT32)RGB_GREEN(*thePixel) ;
				finB = (INT32)RGB_BLUE(*thePixel) ;

				if (hng64_tcram[0x00000007] != 0x00000000)
				{
					finR -= darkR ;
					finG -= darkG ;
					finB -= darkB ;
				}

				if (hng64_tcram[0x0000000a] != 0x00000000)
				{
					finR += brigR ;
					finG += brigG ;
					finB += brigB ;
				}

				if (finR > 255) finR = 255 ;
				if (finG > 255) finG = 255 ;
				if (finB > 255) finB = 255 ;

				if (finR < 0) finR = 0 ;
				if (finG < 0) finG = 0 ;
				if (finB < 0) finB = 0 ;

				*thePixel = MAKE_ARGB(255, (UINT8)finR, (UINT8)finG, (UINT8)finB) ;
			}
		}
	}
}

#define MAX_ONSCREEN_POLYS (10000)

struct polyVert
{
	float worldCoords[4] ;

	float texCoords[4] ;

	float normal[4] ;
	float clipCoords[4] ;

	float light[3] ;
} ;

struct polygon
{
	int n ;
	struct polyVert vert[10] ;

	float faceNormal[4] ;
	int visible ;

	INT8 texIndex;
	UINT16 texture_page;
	UINT16 palette_base, flat_color;
	UINT16 scroll_x, scroll_y;
	UINT16 wrap_x, wrap_y;
	UINT8 four_bit;
	UINT8 blend;
} ;

static void PerformFrustumClip(struct polygon *p) ;

static void DrawShaded(const struct polygon *p);

static float projectionMatrix[16], cameraMatrix[16];
static unsigned paletteState, textureScrollX, textureScrollY;
static float modelScale[3];

void hng64_video_reset(void)
{
	SetIdentity(projectionMatrix);
	SetIdentity(cameraMatrix);
	paletteState = textureScrollX = textureScrollY = 0;
	modelScale[0] = modelScale[1] = modelScale[2] = 1.0f;
	hng64_packet_count = 0;
}

static unsigned hng64_decode_triangle(struct polygon *poly, const struct polygon *last,
	const UINT8 *data, unsigned available, unsigned command_flags)
{
	unsigned words, vertices, stride, i, axis;
	UINT16 format;
	const UINT16 *src;

	if (available < 6)
		return 0;

	format = WORD_AT(data, 0);
	switch (format)
	{
		case 0x04: case 0x0e: case 0x24: case 0x2e:
			words = 24; vertices = 3; stride = 6;
			break;
		case 0x05: case 0x0f:
			words = 33; vertices = 3; stride = 9;
			break;
		case 0x87: case 0x97: case 0xc7: case 0xd7:
			words = 15; vertices = 1; stride = 9;
			break;
		case 0x86: case 0x96: case 0xb6: case 0xc6: case 0xd6:
			words = 12; vertices = 1; stride = 6;
			break;
		default:
			return 0;
	}
	if (available < words * 2)
		return 0;

	src = (const UINT16 *)data + 3;
	poly->n = 3;
	if (vertices == 1)
	{
		poly->vert[1] = last->vert[0];
		poly->vert[2] = last->vert[2];
	}

	for (i = 0; i < vertices; ++i, src += stride)
	{
		struct polyVert *v = &poly->vert[i];
		for (axis = 0; axis < 3; ++axis)
		{
			v->worldCoords[axis] = uToF(src[axis]);
			if (command_flags & 0x0040)
				v->worldCoords[axis] *= modelScale[axis];
		}
		v->worldCoords[3] = 1.0f;
		v->texCoords[0] = uToF(src[4]);
		v->texCoords[1] = uToF(src[5]);
		poly->flat_color = src[4] >> 5;
	}

	for (axis = 0; axis < 3; ++axis)
		poly->faceNormal[axis] = words == 12 ? last->faceNormal[axis] : uToF(src[axis]);
	poly->faceNormal[3] = 0.0f;
	return words * 2;
}

static void draw3d(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int i,k,l,m;
	unsigned packet;

	float modelViewMatrix[16] ;
	float objectMatrix[16] ;


	UINT32 numPolys = 0 ;

	struct polygon lastPoly = { 0 } ;

	SetIdentity(modelViewMatrix) ;
	SetIdentity(objectMatrix) ;

	for (packet = 0; packet < hng64_packet_count; ++packet)
	{
		const UINT32 *workingList = hng64_packets[packet];
		const int i = 0;

			float left, right, top, bottom, near_, far_, screen_z, z_scale;
			UINT8 *threeDRoms ;
			UINT8 *threeDPointer ;
			UINT32 threeDOffset;
			UINT32 geometrySize;
			UINT32 size[4] ;
			UINT32 address[4] ;
			UINT32 megaOffset ;
			float eyeCoords[4] ;

			float ndCoords[4] ;
			float windowCoords[4] ;
			float cullRay[4] ;

			switch((workingList[i+0] & 0xffff0000) >> 16)
			{
			case 0x0012:

				left   = uToF( workingList[i+5] & 0x0000ffff) ;
				right  = uToF((workingList[i+5] & 0xffff0000) >> 16) ;
				top    = uToF((workingList[i+6] & 0xffff0000) >> 16) ;
				bottom = uToF( workingList[i+6] & 0x0000ffff) ;
				z_scale = 1.0f + uToF(workingList[i+2] >> 16);
				screen_z = uToF(workingList[i+3] >> 16) * z_scale;
				near_ = uToF(workingList[i+2] & 0xffff) * z_scale;
				if (right == left || top == bottom || screen_z == 2.0f * near_)
					break;
				far_ = -(screen_z * near_) / (screen_z - 2.0f * near_);
				if (far_ == near_)
					break;

				projectionMatrix[0]  = (2.0f*screen_z)/(right-left) ;
				projectionMatrix[1]  = 0.0f ;
				projectionMatrix[2]  = 0.0f ;
				projectionMatrix[3]  = 0.0f ;

				projectionMatrix[4]  = 0.0f ;
				projectionMatrix[5]  = (2.0f*screen_z)/(top-bottom) ;
				projectionMatrix[6]  = 0.0f ;
				projectionMatrix[7]  = 0.0f ;

				projectionMatrix[8]  = (right+left)/(right-left) ;
				projectionMatrix[9]  = (top+bottom)/(top-bottom) ;
				projectionMatrix[10] = -((far_+near_)/(far_-near_)) ;
				projectionMatrix[11] = -1.0f ;

				projectionMatrix[12] = 0.0f ;
				projectionMatrix[13] = 0.0f ;
				projectionMatrix[14] = -((2.0f*far_*near_)/(far_-near_)) ;
				projectionMatrix[15] = 0.0f ;

				break ;

			case 0x0001:

				cameraMatrix[0]  = uToF( workingList[i+0] & 0x0000ffff) ;
				cameraMatrix[4]  = uToF((workingList[i+1] & 0xffff0000) >> 16) ;
				cameraMatrix[8]  = uToF( workingList[i+1] & 0x0000ffff) ;
				cameraMatrix[3]  = 0.0f ;

				cameraMatrix[1]  = uToF((workingList[i+2] & 0xffff0000) >> 16) ;
				cameraMatrix[5]  = uToF( workingList[i+2] & 0x0000ffff) ;
				cameraMatrix[9]  = uToF((workingList[i+3] & 0xffff0000) >> 16) ;
				cameraMatrix[7]  = 0.0f ;

				cameraMatrix[2]  = uToF( workingList[i+3] & 0x0000ffff) ;
				cameraMatrix[6]  = uToF((workingList[i+4] & 0xffff0000) >> 16) ;
				cameraMatrix[10] = uToF( workingList[i+4] & 0x0000ffff) ;
				cameraMatrix[11] = 0.0f ;

				cameraMatrix[12] = uToF((workingList[i+5] & 0xffff0000) >> 16) ;
				cameraMatrix[13] = uToF( workingList[i+5] & 0x0000ffff) ;
				cameraMatrix[14] = uToF((workingList[i+6] & 0xffff0000) >> 16) ;
				cameraMatrix[15] = 1.0f ;
				break ;

			case 0x0010:

				break ;

			case 0x0011:

				paletteState = (workingList[i+4] >> 24) & 0x3f;
				textureScrollX = (workingList[i] & 0x3fff) >> 5;
				textureScrollY = (workingList[i+1] & 0x3fff0000) >> 21;
				modelScale[0] = (workingList[i+3] & 0xffff) / 256.0f;
				modelScale[1] = (workingList[i+3] >> 16) / 256.0f;
				modelScale[2] = (workingList[i+2] & 0xffff) / 256.0f;

				break ;

			case 0x0100:
			case 0x0101:

				threeDRoms = memory_region(REGION_GFX4);
				geometrySize = memory_region_length(REGION_GFX4);

				if (workingList[i+1] >= geometrySize / 6)
					break;
				threeDOffset = workingList[i+1] * 6;

				if (threeDOffset + 22 > geometrySize)
					break;
				threeDPointer = &threeDRoms[threeDOffset];

				objectMatrix[8 ] = uToF( workingList[i+3] & 0x0000ffff) ;
				objectMatrix[4 ] = uToF((workingList[i+4] & 0xffff0000) >> 16) ;
				objectMatrix[0 ] = uToF( workingList[i+4] & 0x0000ffff) ;
				objectMatrix[3]  = 0.0f ;

				objectMatrix[9 ] = uToF((workingList[i+5] & 0xffff0000) >> 16) ;
				objectMatrix[5 ] = uToF( workingList[i+5] & 0x0000ffff) ;
				objectMatrix[1 ] = uToF((workingList[i+6] & 0xffff0000) >> 16) ;
				objectMatrix[7]  = 0.0f ;

				objectMatrix[10] = uToF( workingList[i+6] & 0x0000ffff) ;
				objectMatrix[6 ] = uToF((workingList[i+7] & 0xffff0000) >> 16) ;
				objectMatrix[2 ] = uToF( workingList[i+7] & 0x0000ffff) ;
				objectMatrix[11] = 0.0f ;

				objectMatrix[12] = uToF((workingList[i+2] & 0xffff0000) >> 16) ;
				objectMatrix[13] = uToF( workingList[i+2] & 0x0000ffff) ;
				objectMatrix[14] = uToF((workingList[i+3] & 0xffff0000) >> 16) ;
				objectMatrix[15] = 1.0f ;

				size[0] = WORD_AT( threeDPointer, ( 6<<1) ) ;
				size[1] = WORD_AT( threeDPointer, ( 7<<1) ) ;
				size[2] = WORD_AT( threeDPointer, ( 9<<1) ) ;
				size[3] = WORD_AT( threeDPointer, (10<<1) ) ;

				megaOffset = ( WORD_AT(threeDRoms, (threeDOffset + 4)) ) << 16 ;
				address[0] = megaOffset | WORD_AT(threeDPointer, (0<<1)) ;
				address[1] = megaOffset | WORD_AT(threeDPointer, (1<<1)) ;
				address[2] = megaOffset | WORD_AT(threeDPointer, (3<<1)) ;
				address[3] = megaOffset | WORD_AT(threeDPointer, (4<<1)) ;

				SetIdentity(modelViewMatrix) ;
				if (!hng64_samurai_3d)
					matmul4(modelViewMatrix, modelViewMatrix, cameraMatrix);
				matmul4(modelViewMatrix, modelViewMatrix, objectMatrix) ;

				for (k = 0; k < 4; k++)
				{
					UINT64 chunkOffset = (UINT64)address[k] * 6;
					if (chunkOffset >= geometrySize)
						continue;
					threeDPointer = &threeDRoms[chunkOffset];

					for (l = 0; l < size[k]; l++)
					{

						unsigned chunkLength;
						struct polygon *poly;

						if (numPolys >= MAX_ONSCREEN_POLYS)
							break;
						poly = &polys[numPolys];
						memset(poly, 0, sizeof(*poly));

						/* Each geometry record has its own length, including triangle continuations. */
						chunkLength = hng64_decode_triangle(poly, &lastPoly, threeDPointer,
							threeDRoms + geometrySize - threeDPointer, workingList[i] & 0xffff);
						if (!chunkLength)
							break;

						{
							UINT16 tex = WORD_AT(threeDPointer, 2);
							unsigned page = tex & 15;
							poly->texIndex = (tex & 0x8000) ? page : -1;
							poly->four_bit = !!(tex & 0x1000);
							poly->blend = !!(tex & 0x4000);
							poly->texture_page = WORD_AT(threeDPointer, 4);
							poly->palette_base = ((tex >> 4) & 0xff) << 3;
							if (workingList[i] & 0x100)
								poly->palette_base |= paletteState << 7;
							poly->wrap_x = 1 << MIN(10, hng64_texture_wrap[page * 2]);
							poly->wrap_y = 1 << MIN(10, hng64_texture_wrap[page * 2 + 1]);
							poly->scroll_x = (workingList[i] & 0x80) ? textureScrollX : 0;
							poly->scroll_y = (workingList[i] & 0x80) ? textureScrollY : 0;
						}

						polys[numPolys].visible = 1;

						memcpy(&lastPoly, &polys[numPolys], sizeof(struct polygon)) ;

						vecmatmul4(cullRay, modelViewMatrix, polys[numPolys].vert[0].worldCoords) ;

						if (workingList[i] & 0x10)
						{
							float normal[4];

							vecmatmul4(normal, modelViewMatrix, poly->faceNormal);
							if (cullRay[0] * normal[0] + cullRay[1] * normal[1] + cullRay[2] * normal[2] >= 0.0f)
								poly->visible = 0;
						}

						if (polys[numPolys].visible)
						{
							for (m = 0; m < polys[numPolys].n; m++)
							{

								vecmatmul4(eyeCoords, modelViewMatrix, polys[numPolys].vert[m].worldCoords) ;
								vecmatmul4(polys[numPolys].vert[m].clipCoords, projectionMatrix, eyeCoords) ;
							}

							if (polys[numPolys].visible)
							{

								PerformFrustumClip(&polys[numPolys]) ;

								for (m = 0; m < polys[numPolys].n; m++)
								{

									if (!(polys[numPolys].vert[m].clipCoords[3] > 0.0f))
									{
										polys[numPolys].visible = 0;
										break;
									}
									ndCoords[0] = polys[numPolys].vert[m].clipCoords[0] / polys[numPolys].vert[m].clipCoords[3] ;
									ndCoords[1] = polys[numPolys].vert[m].clipCoords[1] / polys[numPolys].vert[m].clipCoords[3] ;
									ndCoords[2] = polys[numPolys].vert[m].clipCoords[2] / polys[numPolys].vert[m].clipCoords[3] ;
									ndCoords[3] = polys[numPolys].vert[m].clipCoords[3] ;

									windowCoords[0] = (ndCoords[0]+1.0f) * 256.0f ;
									windowCoords[1] = (ndCoords[1]+1.0f) * 256.0f ;
									windowCoords[2] = (ndCoords[2]+1.0f) * 0.5f ;

									windowCoords[1] = 512.0f - windowCoords[1] ;

									polys[numPolys].vert[m].clipCoords[0] = windowCoords[0] ;
									polys[numPolys].vert[m].clipCoords[1] = windowCoords[1] ;
									polys[numPolys].vert[m].clipCoords[2] = windowCoords[2] ;
									polys[numPolys].vert[m].clipCoords[3] = ndCoords[3] ;
								}
							}
						}

						threeDPointer += chunkLength ;

						numPolys++ ;
					}
				}

				break ;

			default:
				break ;
			}
	}

	memset(hng64_3d_buffer, 0, 512 * 512 * sizeof(*hng64_3d_buffer));
	for (i = 0; i < 512 * 512; ++i)
		depthBuffer[i] = 100.0f;

	for (i = 0; i < numPolys; ++i)
		if (polys[i].visible)
			DrawShaded(&polys[i]);
	poly_wait(hng64_poly, "HNG64 frame");

	if (!(hng64_fbregs[0] & 0x01000000))
	{
		int xscroll = (INT32)hng64_fbregs[2] >> 21;
		unsigned palette_bank = !hng64_roadedge_3d && (hng64_fbregs[0] & 0x2000) ? 0x800 : 0;
		int y, x;
		for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
		{
			const rectangle *vis = &Machine->visible_area;
			int real_y = (y - vis->min_y) * 512 / (vis->max_y - vis->min_y + 1);
			const UINT16 *src = hng64_3d_buffer + (real_y & 511) * 512;
			UINT32 *dst = (UINT32 *)bitmap->line[y];

			for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
			{
				UINT16 pixel = src[(x + xscroll + 256) & 511];
				if (pixel & 0x7ff)
				{
					UINT32 color = Machine->pens[(pixel & 0x7ff) | palette_bank];
					if (pixel & 0x800)
						color = ((color & 0xfefefe) >> 1) + ((dst[x] & 0xfefefe) >> 1);
					dst[x] = color;
				}
			}
		}
	}

	hng64_packet_count = 0;
}

static tilemap *hng64_layers[4][3];
static UINT16 hng64_tileflags[4];
static UINT32 hng64_anim_mask, hng64_anim_bits;

static unsigned hng64_layer_flags(int layer)
{
	return (hng64_videoregs[2 + layer / 2] >> ((layer & 1) ? 0 : 16)) & 0xffff;
}

static void hng64_tile_info(int index, int layer, int large)
{
	UINT32 data = hng64_videoram[index + (layer << 14)];
	int color = data >> 24;
	int flip = TILE_FLIPYX((data >> 22) & 3);
	int eight_bit = (hng64_layer_flags(layer) >> 10) & 1;

	if ((data & 0x200000) && (hng64_videoregs[1] & 0x10000))
		data = (data & hng64_videoregs[0x0b]) | hng64_videoregs[0x0c];
	data &= 0x1fffff;

	if (eight_bit)
		color >>= 4;
	SET_TILE_INFO(large * 2 + eight_bit, data >> (large * 2 + eight_bit), color, flip);

}

#define HNG64_TILE_INFO(layer) \
	static void hng64_tile##layer##_small(int index) { hng64_tile_info(index, layer, 0); } \
	static void hng64_tile##layer##_large(int index) { hng64_tile_info(index, layer, 1); }

HNG64_TILE_INFO(0)
HNG64_TILE_INFO(1)
HNG64_TILE_INFO(2)
HNG64_TILE_INFO(3)

void hng64_tile_dirty(unsigned offset)
{
	unsigned layer = offset >> 14;
	int i;

	if (layer < 4)
		for (i = 0; i < 3; ++i)
			tilemap_mark_tile_dirty(hng64_layers[layer][i], offset & 0x3fff);
}

static void hng64_draw_layer(mame_bitmap *bitmap, const rectangle *cliprect, int layer)
{
	unsigned flags = hng64_layer_flags(layer);
	unsigned global = hng64_videoregs[0];
	unsigned base = (hng64_videoregs[4 + layer / 2] >> ((layer & 1) ? 0 : 16)) & 0x3fff;
	tilemap *map;
	int per_line = !(flags & 0x800);
	int alternative = !!(global & 0x04000000);
	int mosaic = (flags >> 12) + 1;
	int y;

	if (!(flags & 0x40))
		return;

	map = hng64_layers[layer][(flags & 0x200) ? ((global & 0x03000000) ? 2 : 1) : 0];

	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
	{
		rectangle line = *cliprect;
		int source_y = y - y % mosaic;
		UINT32 x = 0, sy = 0;
		int dx = 1 << 16, dy = 1 << 16, dxy = 0, dyx = 0;

		line.min_y = line.max_y = y;
		if (!(global & 0x10000))
		{
			const UINT32 *scroll = hng64_videoram + 0x10000;
			unsigned index = base * 4;
			unsigned row = ((base + source_y) & 0x3fff) * 4;
			UINT32 middle_x = scroll[index + 1], middle_y;

			x = scroll[index];
			sy = scroll[(per_line ? row : index) + 2];
			middle_y = scroll[(per_line ? row : index) + 3];
			if (per_line)
			{
				if (!(scroll[row] & 0xff))
					x = scroll[row];
				if (!(scroll[row + 1] & 0xff))
					middle_x = scroll[row + 1];
			}
			else if (alternative)
			{
				middle_x = scroll[(index + 4) & 0xffff];
				dyx = (INT32)(scroll[index + 1] - x) / 256;
				dxy = (INT32)(scroll[(index + 6) & 0xffff] - sy) / 256;
			}

			dx = (INT32)(middle_x - x) / 256;
			dy = (INT32)(middle_y - sy) / 256;
			if (per_line && alternative)
			{
				dxy = dy;
				dy = 0;
			}
		}

		/* The tilemap core adds the real output Y. Adjust to the mosaic source row. */
		x += (UINT32)(source_y - y) * dyx;
		sy += (UINT32)(source_y - y) * dy;
		if ((global & 1) && !alternative && !per_line && !(global & 0x10000))
		{
			UINT32 right = hng64_videoregs[0x0a];
			UINT32 left = hng64_videoregs[0x09];

			line.min_x = MAX(cliprect->min_x, 256);
			if (line.min_x <= line.max_x)
				tilemap_draw_roz(bitmap, &line, map, x + (right << 16), sy + (right & 0xffff0000),
					dx, dxy, dyx, dy, flags & 0x100, 0, 0);

			line.min_x = cliprect->min_x;
			line.max_x = MIN(cliprect->max_x, 255);
			if (line.min_x <= line.max_x)
				tilemap_draw_roz(bitmap, &line, map, x + (left << 16), sy + (left & 0xffff0000),
					dx, dxy, dyx, dy, flags & 0x100, 0, 0);
		}
		else
			tilemap_draw_roz(bitmap, &line, map, x, sy, dx, dxy, dyx, dy, flags & 0x100, 0, 0);
	}
}

VIDEO_UPDATE( hng64 )
{
	int priority, layer;
	UINT32 anim_mask = (hng64_videoregs[1] & 0x10000) ? hng64_videoregs[0x0b] : 0xffffffff;
	UINT32 anim_bits = (hng64_videoregs[1] & 0x10000) ? hng64_videoregs[0x0c] : 0;
	int anim_changed = anim_mask != hng64_anim_mask || anim_bits != hng64_anim_bits;

	fillbitmap(bitmap, get_black_pen(), cliprect);
	hng64_draw_sprites(cliprect);

	for (layer = 0; layer < 4; ++layer)
	{
		unsigned flags = hng64_layer_flags(layer);
		if (anim_changed || ((flags ^ hng64_tileflags[layer]) & 0x400))
		{
			int i;
			for (i = 0; i < 3; ++i)
				tilemap_mark_all_tiles_dirty(hng64_layers[layer][i]);
			hng64_tileflags[layer] = flags;
		}
	}
	hng64_anim_mask = anim_mask;
	hng64_anim_bits = anim_bits;

	for (priority = 31; priority >= 16; --priority)
	{
		for (layer = 0; layer < 4; ++layer)
			if ((hng64_layer_flags(layer) & 31) == priority)
				hng64_draw_layer(bitmap, cliprect, layer);
		if (!(priority & 3))
			hng64_mix_sprites(bitmap, cliprect, (priority / 4) << 12);
	}

	draw3d(bitmap, cliprect);

	for (priority = 15; priority >= 0; --priority)
	{
		for (layer = 0; layer < 4; ++layer)
			if ((hng64_layer_flags(layer) & 31) == priority)
				hng64_draw_layer(bitmap, cliprect, layer);
		if (!(priority & 3))
			hng64_mix_sprites(bitmap, cliprect, (priority / 4) << 12);
	}

	transition_control(bitmap);
}

VIDEO_START( hng64 )
{
	static void (*const small[4])(int) =
		{ hng64_tile0_small, hng64_tile1_small, hng64_tile2_small, hng64_tile3_small };
	static void (*const large[4])(int) =
		{ hng64_tile0_large, hng64_tile1_large, hng64_tile2_large, hng64_tile3_large };
	int layer, i;

	for (layer = 0; layer < 4; ++layer)
	{
		hng64_layers[layer][0] = tilemap_create(small[layer], tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 128, 128);
		hng64_layers[layer][1] = tilemap_create(large[layer], tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 128, 128);
		hng64_layers[layer][2] = tilemap_create(large[layer], tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 256, 64);
		for (i = 0; i < 3; ++i)
		{
			if (!hng64_layers[layer][i])
				return 1;
			tilemap_set_transparent_pen(hng64_layers[layer][i], 0);
		}
		hng64_tileflags[layer] = 0xffff;
	}
	hng64_sprite_bitmap = auto_bitmap_alloc_depth(768, 528, 16);
	hng64_sprite_depth = auto_bitmap_alloc_depth(768, 528, 16);
	if (!hng64_sprite_bitmap || !hng64_sprite_depth)
		return 1;

	depthBuffer = auto_malloc(512 * 512 * sizeof(*depthBuffer));
	hng64_3d_buffer = auto_malloc(512 * 512 * sizeof(*hng64_3d_buffer));
	hng64_texture_rom = memory_region(REGION_GFX3);
	hng64_poly = poly_alloc(2048, sizeof(struct polygon), 0);
	polys = auto_malloc(MAX_ONSCREEN_POLYS * sizeof(struct polygon));
	return 0;
}

static void matmul4( float *product, const float *a, const float *b )
{
   int i;
   for (i = 0; i < 4; i++)
   {
      const float ai0 = a[0  + i] ;
	  const float ai1 = a[4  + i] ;
	  const float ai2 = a[8  + i] ;
	  const float ai3 = a[12 + i] ;

	  product[0  + i] = ai0 * b[0 ] + ai1 * b[1 ] + ai2 * b[2 ] + ai3 * b[3 ] ;
	  product[4  + i] = ai0 * b[4 ] + ai1 * b[5 ] + ai2 * b[6 ] + ai3 * b[7 ] ;
	  product[8  + i] = ai0 * b[8 ] + ai1 * b[9 ] + ai2 * b[10] + ai3 * b[11] ;
	  product[12 + i] = ai0 * b[12] + ai1 * b[13] + ai2 * b[14] + ai3 * b[15] ;
   }
}

static void vecmatmul4( float *product, const float *a, const float *b)
{
	const float bi0 = b[0] ;
	const float bi1 = b[1] ;
	const float bi2 = b[2] ;
	const float bi3 = b[3] ;

	product[0] = bi0 * a[0] + bi1 * a[4] + bi2 * a[8 ] + bi3 * a[12];
	product[1] = bi0 * a[1] + bi1 * a[5] + bi2 * a[9 ] + bi3 * a[13];
	product[2] = bi0 * a[2] + bi1 * a[6] + bi2 * a[10] + bi3 * a[14];
	product[3] = bi0 * a[3] + bi1 * a[7] + bi2 * a[11] + bi3 * a[15];
}

static void SetIdentity(float *matrix)
{
	int i ;

	for (i = 0; i < 16; i++)
	{
		matrix[i] = 0.0f ;
	}

	matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f ;
}

static float uToF(UINT16 input)
{
	float retVal ;

	retVal = (float)((INT16)input) / 32768.0f ;

	return retVal ;
}

#define HNG64_LEFT   0
#define HNG64_RIGHT  1
#define HNG64_TOP    2
#define HNG64_BOTTOM 3
#define HNG64_NEAR   4
#define HNG64_FAR    5

static int Inside(struct polyVert *v, int plane)
{
	switch(plane)
	{
	case HNG64_LEFT:
		return (v->clipCoords[0] >= -v->clipCoords[3]) ? 1 : 0 ;
	case HNG64_RIGHT:
		return (v->clipCoords[0] <=  v->clipCoords[3]) ? 1 : 0 ;

	case HNG64_TOP:
		return (v->clipCoords[1] <=  v->clipCoords[3]) ? 1 : 0 ;
	case HNG64_BOTTOM:
		return (v->clipCoords[1] >= -v->clipCoords[3]) ? 1 : 0 ;

	case HNG64_NEAR:
		return (v->clipCoords[2] <=  v->clipCoords[3]) ? 1 : 0;
	case HNG64_FAR:
		return (v->clipCoords[2] >= -v->clipCoords[3]) ? 1 : 0;
	}

	return 0 ;
}

static void Intersect(struct polyVert *input0, struct polyVert *input1, struct polyVert *output, int plane)
{
	float t = 0.0f ;

	float *Iv0 = input0->clipCoords ;
	float *Iv1 = input1->clipCoords ;
	float *Ov  = output->clipCoords ;

	float *It0 = input0->texCoords ;
	float *It1 = input1->texCoords ;
	float *Ot  = output->texCoords ;

	float *Il0 = input0->light ;
	float *Il1 = input1->light ;
	float *Ol  = output->light ;

	switch(plane)
	{
	case HNG64_LEFT:
		t = (Iv0[0]+Iv0[3]) / (-Iv1[3]+Iv0[3]-Iv1[0]+Iv0[0]);
		break;
	case HNG64_RIGHT:
		t = (Iv0[0]-Iv0[3]) / (Iv1[3]-Iv0[3]-Iv1[0]+Iv0[0]);
		break;
	case HNG64_TOP:
		t = (Iv0[1]-Iv0[3]) / (Iv1[3]-Iv0[3]-Iv1[1]+Iv0[1]);
		break;
	case HNG64_BOTTOM:
		t = (Iv0[1]+Iv0[3]) / (-Iv1[3]+Iv0[3]-Iv1[1]+Iv0[1]);
		break;
	case HNG64_NEAR:
		t = (Iv0[2]-Iv0[3]) / (Iv1[3]-Iv0[3]-Iv1[2]+Iv0[2]);
		break;
	case HNG64_FAR:
		t = (Iv0[2]+Iv0[3]) / (-Iv1[3]+Iv0[3]-Iv1[2]+Iv0[2]);
		break;
	}

	Ov[0] = Iv0[0] + (Iv1[0] - Iv0[0]) * t ;
	Ov[1] = Iv0[1] + (Iv1[1] - Iv0[1]) * t ;
	Ov[2] = Iv0[2] + (Iv1[2] - Iv0[2]) * t ;
	Ov[3] = Iv0[3] + (Iv1[3] - Iv0[3]) * t ;

	Ot[0] = It0[0] + (It1[0] - It0[0]) * t ;
	Ot[1] = It0[1] + (It1[1] - It0[1]) * t ;
	Ot[2] = It0[2] + (It1[2] - It0[2]) * t ;
	Ot[3] = It0[3] + (It1[3] - It0[3]) * t ;

	Ol[0] = Il0[0] + (Il1[0] - Il0[0]) * t ;
	Ol[1] = Il0[1] + (Il1[1] - Il0[1]) * t ;
	Ol[2] = Il0[2] + (Il1[2] - Il0[2]) * t ;
}

static void PerformFrustumClip(struct polygon *p)
{
	int i, j ;
	int k ;

	struct polygon temp ;

	struct polyVert *v0 ;
	struct polyVert *v1 ;
	struct polyVert *tv ;

	temp.n = 0;

	for (j = 0; j <= HNG64_FAR; j++)
	{
		for (i = 0; i < p->n; i++)
		{
			k = (i+1) % p->n;

			v0 = &p->vert[i] ;
			v1 = &p->vert[k] ;

			tv = &temp.vert[temp.n] ;

			if (temp.n + 2 > ARRAY_LENGTH(temp.vert))
			{
				p->n = 0;
				return;
			}

			if (Inside(v0, j) && Inside(v1, j))
			{
				memcpy(tv, v1, sizeof(struct polyVert)) ;
				temp.n++;
			}

			else if (Inside(v0, j) && !Inside(v1, j))
			{
				Intersect(v0, v1, tv, j) ;
				temp.n++;
			}

			else if (!Inside(v0, j) && Inside(v1, j))
			{
				Intersect(v0, v1, tv, j) ;
				memcpy(&temp.vert[temp.n+1], v1, sizeof(struct polyVert)) ;
				temp.n+=2;
			}
		}

		p->n = temp.n;

		for (i = 0; i < temp.n; i++)
		{
			memcpy(&p->vert[i], &temp.vert[i], sizeof(struct polyVert)) ;
		}

		temp.n = 0 ;
	}
}

static void hng64_scanline(void *dest, INT32 y, const poly_extent *extent, const void *extra, int threadid)
{
	const struct polygon *poly = extra;
	const UINT8 *texture = poly->texIndex < 0 ? NULL : hng64_texture_rom + (poly->texIndex << 20);
	float z = extent->param[0].start;
	float w = extent->param[1].start;
	float s = extent->param[2].start;
	float t = extent->param[3].start;
	float *depth = depthBuffer + y * 512;
	UINT16 *pixels = hng64_3d_buffer + y * 512;
	int x;

	for (x = extent->startx; x < extent->stopx; ++x)
	{
		if (z < depth[x] && w > 0)
		{
			unsigned pen = 0;
			if (texture)
			{
				int u = (int)(t / w * 1024.0f) + poly->scroll_x;
				int v = (int)(s / w * 1024.0f) + poly->scroll_y;
				if (poly->texture_page & 0x8000)
				{
					u = (u & (poly->wrap_x - 1)) + (((poly->texture_page >> 7) & 0x7f) << 3);
					v = (v & (poly->wrap_y - 1)) + ((poly->texture_page & 0x7f) << 3);
				}
				u &= 1023;
				v &= 1023;
				if (poly->four_bit)
					pen = (texture[v * 512 + (u >> 1)] >> ((u & 1) * 4)) & 15;
				else
					pen = texture[v * 1024 + u];
			}
			if (!texture || pen)
			{
				pixels[x] = ((poly->palette_base + (texture ? pen : poly->flat_color)) & 0x7ff) | (poly->blend ? 0x800 : 0);
				depth[x] = z;
			}
		}
		z += extent->param[0].dpdx;
		w += extent->param[1].dpdx;
		s += extent->param[2].dpdx;
		t += extent->param[3].dpdx;
	}
}

static void DrawShaded(const struct polygon *p)
{
	static const rectangle clip = { 0, 511, 0, 511 };
	poly_vertex vertices[ARRAY_LENGTH(p->vert)];
	int i;

	for (i = 0; i < p->n; ++i)
	{
		const struct polyVert *v = &p->vert[i];
		float inverse_w = 1.0f / v->clipCoords[3];
		vertices[i].x = v->clipCoords[0];
		vertices[i].y = v->clipCoords[1];
		vertices[i].p[0] = v->clipCoords[2];
		vertices[i].p[1] = inverse_w;
		vertices[i].p[2] = v->texCoords[0] * inverse_w;
		vertices[i].p[3] = v->texCoords[1] * inverse_w;
	}

	for (i = 1; i + 1 < p->n; ++i)
	{
		struct polygon *extra = poly_get_extra_data(hng64_poly);
		*extra = *p;
		poly_render_triangle(hng64_poly, NULL, &clip, hng64_scanline, 4,
			&vertices[0], &vertices[i], &vertices[i + 1]);
	}
}

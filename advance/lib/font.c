/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

#include "portable.h"

#include "font.h"
#include "log.h"
#include "video.h"
#include "endianrw.h"

static adv_bitmap null_char = { 0, 0, 0, 0, 0, 0 };

/**
 * Get the Y size of the font.
 * The height of the 'A' letter is used.
 */
unsigned adv_font_sizey(adv_font* font)
{
	return adv_font_sizey_char(font, 'A');
}

/**
 * Get the Y size of the font.
 * The height of the 'A' letter is used.
 */
unsigned adv_font_sizey_char(adv_font* font, char c)
{
	if (font->data[(unsigned char)c])
		return font->data[(unsigned char)c]->size_y;
	else
		return 0;
}

/**
 * Get the Y size of the font.
 * The width of the specified string is used.
 */
unsigned adv_font_sizey_string(adv_font* font, const char* begin, const char* end)
{
	unsigned size = 0;
	while (begin != end) {
		size += adv_font_sizey_char(font, *begin);
		++begin;
	}
	return size;
}

/**
 * Get the number of displayed chars.
 */
const char* adv_font_sizey_limit(adv_font* font, const char* begin, const char* end, unsigned limit)
{
	unsigned size = 0;

	while (begin != end) {
		size += adv_font_sizey_char(font, *begin);
		if (size > limit)
			return begin;
		++begin;
	}

	return begin;
}

/**
 * Get the X size of the font.
 * The width of the 'A' letter is used.
 */
unsigned adv_font_sizex(adv_font* font)
{
	return adv_font_sizex_char(font, 'A');
}

/**
 * Get the X size of the font.
 * The width of the specified letter is used.
 */
unsigned adv_font_sizex_char(adv_font* font, char c)
{
	if (font->data[(unsigned char)c])
		return font->data[(unsigned char)c]->size_x;
	else
		return 0;
}

/**
 * Get the X size of the font.
 * The width of the specified string is used.
 */
unsigned adv_font_sizex_string(adv_font* font, const char* begin, const char* end)
{
	unsigned size = 0;
	while (begin != end) {
		size += adv_font_sizex_char(font, *begin);
		++begin;
	}
	return size;
}

/**
 * Get the number of displayed chars.
 */
const char* adv_font_sizex_limit(adv_font* font, const char* begin, const char* end, unsigned limit)
{
	unsigned size = 0;

	while (begin != end) {
		size += adv_font_sizex_char(font, *begin);
		if (size > limit)
			return begin;
		++begin;
	}

	return begin;
}

/**
 * Set a char in the font.
 * The specified bitmap is owned by the font.
 */
void adv_font_set_char(adv_font* font, char c, adv_bitmap* bitmap)
{
	unsigned i = (unsigned char)c;

	if (i>=ADV_FONT_MAX)
		return;

	if (font->data[i] != 0 && font->data[i] != &null_char) {
		adv_bitmap_free(font->data[i]);
	}

	font->data[i] = bitmap;
}

/**
 * Allocate a font.
 */
static adv_font* adv_font_alloc(void)
{
	adv_font* font;
	unsigned i;

	font = malloc(sizeof(adv_font));
	if (!font)
		return 0;

	for(i=0;i<ADV_FONT_MAX;++i)
		font->data[i] = &null_char;

	return font;
}

/**
 * Free a font.
 */
void adv_font_free(adv_font* font)
{
	if (font) {
		int i;
		for(i=0;i<ADV_FONT_MAX;++i) {
			if (font->data[i] != 0 && font->data[i] != &null_char) {
				adv_bitmap_free(font->data[i]);
			}
		}
		free(font);
	}
}

static int adv_font_load_data_fixed(adv_font* font, unsigned char* begin, unsigned start, unsigned count, unsigned width, unsigned height)
{
	unsigned i;

	for(i=start;i<start+count;++i) {
		unsigned x, y;
		adv_bitmap* bitmap;

		bitmap = adv_bitmap_alloc(width, height, 1);
		if (!bitmap)
			return -1;

		for(y=0;y<height;++y) {
			for(x=0;x<width;++x) {
				int set = begin[x/8] & (1 << (7-(x % 8)));
				adv_bitmap_pixel_put(bitmap, x, y, set ? 0xFF : 0);
			}
			begin += (width+7)/8;
		}

		adv_font_set_char(font, i, bitmap);
	}

	return 0;
}

static int adv_font_load_data_size(unsigned count, unsigned* width, unsigned height)
{
	unsigned size = 0;
	unsigned i;

	for(i=0;i<count;++i)
		size += ((width[i]+7)/8)*height;

	return size;
}

static int adv_font_load_data(adv_font* font, unsigned char* begin, unsigned start, unsigned count, unsigned* wtable, unsigned height)
{
	unsigned i;

	for(i=start;i<start+count;++i) {
		unsigned x, y;
		adv_bitmap* bitmap;
		unsigned width = wtable[i-start];

		bitmap = adv_bitmap_alloc(width, height, 1);
		if (!bitmap) {
			return -1;
		}

		for(y=0;y<height;++y) {
			for(x=0;x<width;++x) {
				int set = begin[x/8] & (1 << (7-(x % 8)));
				adv_bitmap_pixel_put(bitmap, x, y, set ? 0xFF : 0);
			}
			begin += (width+7)/8;
		}

		adv_font_set_char(font, i, bitmap);
	}

	return 0;
}

/****************************************************************************/
/* PSF */

/*
			The PSF file-format

		(C) 1997 Yann Dirson <dirson@univ-mlv.fr>


 This file documents the PSF font-file-format, as understood by version 0.94
and above of the Linux console utilities ('bkd'). This file makes obsolete
the old `psffile.doc'.

 This file has revision number 1.0, and is dated 1997/09/02.
 Any useful additionnal information on PSF files would be great.


1. Summary

 A PSF file basically contains one character-font, whose width is 8 pixels,
ie. each scanline in a character occupies 1 byte.

 It may contain characters of any height between 0 and 255, though character
heights lower than 8 or greater than 32 are not attested to exist or even be
useful [more info needed on this].

 Fonts can contain either 256 or 512 characters.
 
 The file can optionnally contain a unicode mapping-table, telling, for each
character in the font, which UCS2 characters it can be used to display.

 The "file mode" byte controls font size (256/512) and whether file contains
a unicode mapping table.


2. History

 Unknown.


3. Known programs understanding this file-format.

 The following program in the Linux console utilities can read and/or write
PSF files:

	setfont (R/W)
	psfaddtable (R/W)
	psfstriptable (R/W)
	psfgettable (R)


4. Technical data

 The file format is described here in sort-of EBNF notation. Upper-case
WORDS represent terminal symbols, ie. C types; lower-case words represent
non-terminal symbols, ie. symbols defined in terms of other symbols.
 [sym] is an optional symbol
 {sym} is a symbol that can be repeated 0 or more times
 {sym}*N is a symbol that must be repeated N times
 Comments are introduced with a # sign.


# The data (U_SHORT's) are stored in LITTLE_ENDIAN byte order.

psf_file =      psf_header
		raw_fontdata
		[unicode_data]
		

psf_header =    CHAR = 0x36  CHAR = 0x04        # magic number
		filemode
		fontheight
		
fontheight =    CHAR            # measured in scan lines
filemode =      CHAR            # IF ([mode] AND 0x01) THEN <fontsize>:=512 ELSE <fontsize>:=256

#

raw_fontdata =  {char_data}*<fontsize>

char_data =     {BYTE}*<fontheight>

#

unicode_data =  { unicode_array psf_separator }*<fontsize>

unicode_array = { unicode }                             # any necessary number of times

unicode =       U_SHORT                                 # UCS2 code
psf_separator = unicode = 0xFFFF

*/

static adv_font* adv_font_load_psf(adv_fz* f)
{
	unsigned char header[2];
	unsigned char c;
	unsigned size;
	unsigned height;
	unsigned width;
	adv_font* font;
	unsigned char* data;
	unsigned data_size;

	width = 8;

	font = adv_font_alloc();
	if (!font) {
		goto out;
	}

	if (fzread(header, 2, 1, f)!=1) {
		goto out_font;
	}

	if (header[0] != 0x36 || header[1] != 0x4) {
		goto out_font;
	}

	if (fzread(&c, 1, 1, f)!=1) {
		goto out_font;
	}

	size = c & 0x1 ? 512 : 256;

	if (fzread(&c, 1, 1, f)!=1) {
		goto out_font;
	}

	height = c;
	if (height < 8 || height > 32) {
		goto out_font;
	}

	data_size = size * height;
	data = malloc(data_size);
	if (!data)
		goto out_font;

	if (fzread(data, data_size, 1, f)!=1)
		goto out_data;

	if (adv_font_load_data_fixed(font, data, 0, size, width, height)!=0) {
		goto out_data;
	}

	free(data);

	return font;

out_data:
	free(data);
out_font:
	free(font);
out:
	return 0;
}

/****************************************************************************/
/* RAW */

/*
			The RAW file-format

		(C) 1997 Yann Dirson <dirson@univ-mlv.fr>


 This file documents the RAW font-file-format, as understood by the Linux
console utilities ('bkd').

 This file has revision number 1.0, and is dated 1997/09/02.


1. Summary

 A RAW file only contains one 8-pixels-wide 256-characters font, ie. each
scanline in a character occupies 1 byte.

 It may contain characters of any height between 0 and 255, though character
heights lower than 8 or greater than 32 are not attested to exist or even be
useful [more info needed on this]; the file's size is used to determine the
font's height when reading it.

 WARNING: no program can reliably ensure a file it reads is in this format;
it can only recognize when the file's size makes it obvious it is not. Thus
some files can be wrongly assumed to be raw font-files. For this reason, you
are strongly encouraged to use other formats, like PSF, which can be
identified by magic-number.


2. History

 Unknown. This file-format probably cannot evolve.


3. Known programs understanding this file-format.

 The following program in the Linux console utilities can read and/or write
RAW files:

	setfont (R/W)


4. Technical data

 The file format is described here in sort-of EBNF notation. Upper-case
WORDS represent terminal symbols, ie. C types; lower-case words represent
non-terminal symbols, ie. symbols defined in terms of other symbols.
 [sym] is an optional symbol
 {sym} is a symbol that can be repeated 0 or more times
 {sym}*N is a symbol that must be repeated N times
 Comments are introduced with a # sign.


# The data (U_SHORT's) are stored in LITTLE_ENDIAN byte order.

raw_file =      raw_fontdata

raw_fontdata =  {char_data}*256

char_data =     {BYTE}*<fontheight>

# This makes the file have a size of 256*<fontheight> bytes; thus only files
# whose size has 0 as less significant byte can be interpreted as a raw font.
# One might even want to extend these lower 8 bits to 10 (resp. 11) to ensure
# that no file is wrongly assumed to be a (quite rare!) less-than-4 (resp. 8)
# scanlines font.
*/

static adv_font* adv_font_load_raw(adv_fz* f)
{
	unsigned height;
	unsigned width;
	unsigned size;
	unsigned file_size;
	adv_font* font;
	unsigned char* data;
	unsigned data_size;

	font = adv_font_alloc();
	if (!font) {
		goto out;
	}

	file_size = fzsize(f);

	if (file_size % 256 != 0) {
		goto out_font;
	}

	size = 256;
	width = 8;
	height = file_size / 256;
	if (height < 8 || height > 32) {
		goto out_font;
	}

	data_size = size * height;
	data = malloc(data_size);
	if (!data) {
		goto out_font;
	}

	if (fzread(data, data_size, 1, f)!=1) {
		goto out_data;
	}

	if (adv_font_load_data_fixed(font, data, 0, size, width, height)!=0) {
		goto out_data;
	}

	free(data);

	return font;
out_data:
	free(data);
out_font:
	free(font);
out:
	return 0;
}

/****************************************************************************/
/* GRX */

#define GRX_FONT_MAGIC 0x19590214L

static adv_font* adv_font_load_grx(adv_fz* f)
{
	unsigned height;
	unsigned width;
	unsigned size, start, stop, isfixed;
	adv_font* font;
	unsigned char* data;
	unsigned data_size;
	unsigned* wtable;
	unsigned char header[56];

	font = adv_font_alloc();
	if (!font)
		goto out;

	if (fzread(header, 56, 1, f)!=1)
		goto out_font;

	if (le_uint32_read(header) != GRX_FONT_MAGIC)
		goto out_font;

	width = le_uint16_read(header+8);
	height = le_uint16_read(header+10);
	start = le_uint16_read(header+12);
	stop = le_uint16_read(header+14);
	isfixed = le_uint16_read(header+16);

	size = stop - start + 1;

	if (!isfixed) {
		unsigned i;
		wtable = malloc(sizeof(unsigned) * size);
		if (!wtable)
			goto out_font;
		for(i=0;i<size;++i) {
			unsigned char wsize[2];
			if (fzread(wsize, 2, 1, f)!=1)
				goto out_font;
			wtable[i] = le_uint16_read(wsize);
		}
		data_size = adv_font_load_data_size(size, wtable, height);
	} else {
		wtable = 0;
		data_size = ((width+7)/8)*height*size;
	}

	data = malloc(data_size);
	if (!data)
		goto out_table;

	if (fzread(data, data_size, 1, f)!=1)
		goto out_data;

	if (!isfixed) {
		if (adv_font_load_data(font, data, start, size, wtable, height)!=0) {
			goto out_data;
		}
	} else {
		if (adv_font_load_data_fixed(font, data, start, size, width, height)!=0) {
			goto out_data;
		}
	}

	free(data);
	free(wtable);

	return font;
out_data:
	free(data);
out_table:
	free(wtable);
out_font:
	free(font);
out:
	return 0;
}

/****************************************************************************/
/* FreeType2 */

#ifdef USE_FREETYPE

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

static adv_font* adv_font_load_freetype2(adv_fz* f, unsigned sizex, unsigned sizey)
{
	FT_Error e;
	FT_Library freetype;
	FT_Face face;
	unsigned char* mem_buf;
	long mem_size;
	unsigned sb;
	unsigned sy;
	adv_font* font;
	unsigned i;

	font = adv_font_alloc();
	if (!font)
		goto err;

	mem_size = fzsize(f);
	if (mem_size < 0)
		goto err_font;

	mem_buf = malloc(mem_size);
	if (!mem_buf)
		goto err_font;

	if (fzread(mem_buf, mem_size, 1, f) != 1)
		goto err_mem;

	log_std(("font: init freetype2 library %d.%d.%d\n", FREETYPE_MAJOR, FREETYPE_MINOR, FREETYPE_PATCH));

	e = FT_Init_FreeType(&freetype);
	if (e != 0)
		goto err_mem;

	e = FT_New_Memory_Face(freetype, mem_buf, mem_size, 0, &face);
	if (e != 0)
		goto err_lib;

	e = FT_Set_Pixel_Sizes(face, sizex, sizey);
	if (e != 0)
		goto err_face;

	sy = (face->size->metrics.height + 63) / 64;
	sb = (face->size->metrics.ascender + 63) / 64;

	for(i=0;i<ADV_FONT_MAX;++i) {
		e = FT_Load_Char(face, i, FT_LOAD_DEFAULT);
		if (e == 0) {
			unsigned x, y;
			FT_Glyph glyph;

			e = FT_Get_Glyph(face->glyph, &glyph);
			if (e == 0) {
				if (e == 0 && glyph->format != FT_GLYPH_FORMAT_BITMAP) {
					e = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);
				}
				if (e == 0) {
					adv_bitmap* bitmap;
					unsigned cx, cy;
					FT_BitmapGlyph glyph_bitmap;

					glyph_bitmap = (FT_BitmapGlyph)glyph;

					cx = glyph_bitmap->root.advance.x >> 16;
					cy = sy;

					bitmap = adv_bitmap_alloc(cx, cy, 1);
					if (!bitmap)
						goto err_face;

					for(y=0;y<cy;++y) {
						int by = y - (sb - glyph_bitmap->top);
						if (by>=0 && by<glyph_bitmap->bitmap.rows) {
							unsigned char* p = ((unsigned char*)glyph_bitmap->bitmap.buffer) + by * glyph_bitmap->bitmap.pitch;
							for(x=0;x<cx;++x) {
								int bx = x - glyph_bitmap->left;
								if (bx>=0 && bx<glyph_bitmap->bitmap.width)
									adv_bitmap_pixel_put(bitmap, x, y, p[bx]);
								else
									adv_bitmap_pixel_put(bitmap, x, y, 0);
							}
						} else {
							for(x=0;x<cx;++x) {
								adv_bitmap_pixel_put(bitmap, x, y, 0);
							}
						}
					}

					adv_font_set_char(font, i, bitmap);
				}

				FT_Done_Glyph(glyph);
			}
		}
	}

	FT_Done_Face(face);

	FT_Done_FreeType(freetype);

	free(mem_buf);

	return font;

err_face:
	FT_Done_Face(face);
err_lib:
	FT_Done_FreeType(freetype);
err_mem:
	free(mem_buf);
err_font:
	adv_font_free(font);
err:
	return 0;
}
#endif

/****************************************************************************/
/* Adjust */

static adv_font* adv_font_adjust(adv_font* font)
{
	adv_bitmap* bitmap;
	char c;

	if (!font)
		return 0;

	/* set the 255 char as space like a number */
	bitmap = adv_bitmap_alloc(adv_font_sizex_char(font, '0'), adv_font_sizey_char(font, '0'), 1);
	adv_bitmap_clear(bitmap, 0, 0, bitmap->size_x, bitmap->size_y, 0);
	adv_font_set_char(font, ADV_FONT_FIXSPACE, bitmap);

	/* ensure that every number is wide at least like '0' */
	for(c='1';c<='9';++c) {
		if (adv_font_sizex_char(font, c) < adv_font_sizex_char(font, '0')) {
			unsigned x, y;
			adv_bitmap* src = font->data[(unsigned char)c];
			bitmap = adv_bitmap_alloc(adv_font_sizex_char(font, '0'), adv_font_sizey_char(font, '0'), 1);
			for(y=0;y<bitmap->size_y;++y)
				for(x=0;x<bitmap->size_x;++x)
					adv_bitmap_pixel_put(bitmap, x, y, adv_bitmap_pixel_get(src, x, y));
			adv_font_set_char(font, c, bitmap);
		}
	}

	/* make a fake space if it's null */
	if (adv_font_sizex_char(font, ' ') == 0) {
		adv_bitmap* bitmap = adv_bitmap_alloc(adv_font_sizex_char(font, 'I'), adv_font_sizey_char(font, 'I'), 1);
		adv_bitmap_clear(bitmap, 0, 0, bitmap->size_x, bitmap->size_y, 0);
		adv_font_set_char(font, ' ', bitmap);
	}

	return font;
}

/****************************************************************************/
/* Load */

static adv_font* font_load_unscaled(adv_fz* f, unsigned sizex, unsigned sizey)
{
	adv_font* font;
	long pos;

	pos = fztell(f);

#ifdef USE_FREETYPE
	font = adv_font_load_freetype2(f, sizex, sizey);
	if (font)
		return adv_font_adjust(font);

	fzseek(f, pos, SEEK_SET); /* ignore error */
#endif

	font = adv_font_load_grx(f);
	if (font)
		return adv_font_adjust(font);

	fzseek(f, pos, SEEK_SET); /* ignore error */

	font = adv_font_load_psf(f);
	if (font)
		return adv_font_adjust(font);

	fzseek(f, pos, SEEK_SET); /* ignore error */

	font = adv_font_load_raw(f);
	if (font)
		return adv_font_adjust(font);

	return 0;
}

/**
 * Load a font from a file.
 * The following formats are supported:
 *  - GRX
 *  - PSF
 *  - RAW
 *  - TTF (with FreeType2)
 */
adv_font* adv_font_load(adv_fz* f, unsigned sizex, unsigned sizey)
{
	adv_font* font;
	unsigned fx;
	unsigned fy;

	font = font_load_unscaled(f, sizex, sizey);

	if (!font)
		return 0;

	fx = 0;
	if (adv_font_sizex_char(font, 'M'))
		fx = sizex / adv_font_sizex_char(font, 'M');
	if (fx == 0)
		fx = 1;

	fy = 0;
	if (adv_font_sizey_char(font, 'M'))
		fy = sizey / adv_font_sizey_char(font, 'M');
	if (fy == 0)
		fy = 1;

	if (fx!=1 || fy!=1)
		adv_font_scale(font, fx, fy);

	return font;
}

#if 0 /* OSDEF Reference code */
int adv_font_set(adv_font* font)
{
	unsigned adv_font_dx, adv_font_dy;
	unsigned i;
	uint8* adv_font_data;

	if (!video_mode_is_active())
		return -1;

	if (!video_is_text())
		return -1;

	adv_font_dx = adv_font_size_x(font);
	adv_font_dy = adv_font_size_y(font);

	if (adv_font_dx!=8 || adv_font_dy>32)
		return -1;

	adv_font_data = (uint8*)malloc(ADV_FONT_MAX * adv_font_dy);
	for(i=0;i<ADV_FONT_MAX;++i) {
		if (font->data[i] != 0) {
			unsigned y;
			for(y=0;y<adv_font_dy;++y) {
				unsigned x;
				uint8 mask = 0;
				uint8* row = adv_bitmap_line(font->data[i], y);
				for(x=0;x<adv_font_dx;++x) {
					mask <<= 1;
					if (row[x])
						mask |= 1;
				}
				adv_font_data[i*adv_font_dy+y] = mask;
			}
		} else {
			unsigned y;
			for(y=0;y<adv_font_dy;++y) {
				adv_font_data[i*adv_font_dy+y] = 0;
			}
		}
	}

	vga_adv_font_set(adv_font_data, adv_font_dy);

	free(adv_font_data);
	return 0;
}
#endif

/**
 * Change the orientation of a font.
 * \param font Font to change.
 * \param orientation_mask Orientation operation to apply.
 */
void adv_font_orientation(adv_font* font, unsigned orientation_mask)
{
	if (orientation_mask) {
		unsigned i;
		for(i=0;i<ADV_FONT_MAX;++i) {
			if (font->data[i] != 0 && font->data[i] != &null_char)
				adv_bitmap_orientation(font->data[i], orientation_mask);
		}
	}
}

/**
 * Draw a char in a bitmap.
 * \param font Font to use.
 * \param dst Destination bitmap.
 * \param x,y Destination position.
 * \param c Char to draw.
 * \param color_front Color to use for foreground.
 * \param color_back Color to use for background.
 */
void adv_font_put_char(adv_font* font, adv_bitmap* dst, int x, int y, char c, unsigned color_front, unsigned color_back)
{
	adv_bitmap* src;
	unsigned cy;

	src = font->data[(unsigned char)c];
	if (!src) {
		return;
	}

	for(cy=0;cy<src->size_y;++cy) {
		unsigned char* src_ptr = adv_bitmap_line(src, cy);
		unsigned char* dst_ptr = adv_bitmap_pixel(dst, x, y);
		unsigned dp = dst->bytes_per_pixel;
		unsigned cx;
		for(cx=0;cx<src->size_x;++cx) {
			unsigned v;
			if (*src_ptr >= 64) {
				v = color_front;
			} else {
				v = color_back;
			}
			cpu_uint_write(dst_ptr, dp, v);
			dst_ptr += dp;
			src_ptr += 1;
		}
		++y;
	}
}

/**
 * Draw a string in a bitmap.
 * \param font Font to use.
 * \param dst Destination bitmap.
 * \param x,y Destination position.
 * \param begin,end String to draw.
 * \param color_front Color to use for foreground.
 * \param color_back Color to use for background.
 */
void adv_font_put_string(adv_font* font, adv_bitmap* dst, int x, int y, const char* begin, const char* end, unsigned color_front, unsigned color_back)
{
	while (begin != end) {
		adv_font_put_char(font, dst, x, y, *begin, color_front, color_back);
		x += adv_font_sizex_char(font, *begin);
		++begin;
	}
}

/**
 * Draw an oriented string in a bitmap.
 * The font is supposed to be already oriented.
 */
void adv_font_put_string_oriented(adv_font* font, adv_bitmap* dst, int x, int y, const char* begin, const char* end, unsigned color_front, unsigned color_back, unsigned orientation)
{
	if ((orientation & ADV_ORIENTATION_FLIP_XY) != 0) {
		if ((orientation & ADV_ORIENTATION_FLIP_X) != 0)
			x -= adv_font_sizex_char(font, *begin) - 1;
	} else {
		if ((orientation & ADV_ORIENTATION_FLIP_Y) != 0)
			y -= adv_font_sizey_char(font, *begin) - 1;
	}

	while (begin != end) {
		if ((orientation & ADV_ORIENTATION_FLIP_XY) != 0) {
			if ((orientation & ADV_ORIENTATION_FLIP_Y) != 0)
				y -= adv_font_sizey_char(font, *begin) - 1;
		} else {
			if ((orientation & ADV_ORIENTATION_FLIP_X) != 0)
				x -= adv_font_sizex_char(font, *begin) - 1;
		}
		adv_font_put_char(font, dst, x, y, *begin, color_front, color_back);
		if ((orientation & ADV_ORIENTATION_FLIP_XY) != 0) {
			if ((orientation & ADV_ORIENTATION_FLIP_Y) != 0)
				y -= 1;
			else
				y += adv_font_sizey_char(font, *begin);
		} else {
			if ((orientation & ADV_ORIENTATION_FLIP_X) != 0)
				x -= 1;
			else
				x += adv_font_sizex_char(font, *begin);
		}
		++begin;
	}
}

/**
 * Draw an alpha char in a bitmap.
 * \param font Font to use.
 * \param dst Destination bitmap.
 * \param x,y Destination position.
 * \param c Char to draw.
 * \param map Map of 256 colors to use for the different alpha values.
 */
void adv_font_put_char_map(adv_font* font, adv_bitmap* dst, int x, int y, char c, const adv_pixel* map)
{
	adv_bitmap* src;
	unsigned cy;
	unsigned dp;

	src = font->data[(unsigned char)c];
	if (!src)
		return;

	dp = dst->bytes_per_pixel;

	for(cy=0;cy<src->size_y;++cy) {
		unsigned char* src_ptr = adv_bitmap_line(src, cy);
		unsigned char* dst_ptr = adv_bitmap_pixel(dst, x, y);
		unsigned cx;
		for(cx=0;cx<src->size_x;++cx) {
			cpu_uint_write(dst_ptr, dp, map[*src_ptr]);
			dst_ptr += dp;
			src_ptr += 1;
		}
		++y;
	}
}

/**
 * Draw an alpha string in a bitmap.
 * \param font Font to use.
 * \param dst Destination bitmap.
 * \param x,y Destination position.
 * \param begin,end String to draw.
 * \param map Map of 256 colors to use for the different alpha values.
 */
void adv_font_put_string_map(adv_font* font, adv_bitmap* dst, int x, int y, const char* begin, const char* end, const adv_pixel* map)
{
	while (begin != end) {
		adv_font_put_char_map(font, dst, x, y, *begin, map);
		x += adv_font_sizex_char(font, *begin);
		++begin;
	}
}

/**
 * Draw a transparent char in a bitmap.
 * \param font Font to use.
 * \param dst Destination bitmap.
 * \param x,y Destination position.
 * \param c Char to draw.
 * \param color_front Color to use for foreground.
 */
void adv_font_put_char_trasp(adv_font* font, adv_bitmap* dst, int x, int y, char c, unsigned color_front)
{
	adv_bitmap* src;
	unsigned cy;
	unsigned dp;

	src = font->data[(unsigned char)c];
	if (!src)
		return;

	dp = dst->bytes_per_pixel;

	for(cy=0;cy<src->size_y;++cy) {
		unsigned char* src_ptr = adv_bitmap_line(src, cy);
		unsigned char* dst_ptr = adv_bitmap_pixel(dst, x, y);
		unsigned cx;
		for(cx=0;cx<src->size_x;++cx) {
			if (*src_ptr >= 64) {
				cpu_uint_write(dst_ptr, dp, color_front);
			}
			dst_ptr += dp;
			src_ptr += 1;
		}
		++y;
	}
}

/**
 * Draw a transparent string in a bitmap.
 * \param font Font to use.
 * \param dst Destination bitmap.
 * \param x,y Destination position.
 * \param begin,end String to draw.
 * \param color_front Color to use for foreground.
 */
void adv_font_put_string_trasp(adv_font* font, adv_bitmap* dst, int x, int y, const char* begin, const char* end, unsigned color_front)
{
	while (begin != end) {
		adv_font_put_char_trasp(font, dst, x, y, *begin, color_front);
		x += adv_font_sizex_char(font, *begin);
		++begin;
	}
}

/**
 * Scale a font by an integer factor.
 */
void adv_font_scale(adv_font* font, unsigned fx, unsigned fy)
{
	unsigned i;

	for(i=0;i<ADV_FONT_MAX;++i) {
		if (font->data[i] != 0 && font->data[i] != &null_char) {
			adv_bitmap* bitmap;

			bitmap = adv_bitmap_resize(font->data[i], 0, 0, font->data[i]->size_x, font->data[i]->size_y, font->data[i]->size_x*fx, font->data[i]->size_y*fy, 0);

			adv_font_set_char(font, i, bitmap);
		}
	}
}


/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
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
 */

#include "font.h"
#include "video.h"
#include "endianrw.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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
 * Free a font.
 */
void adv_font_free(adv_font* adv_font)
{
	if (adv_font) {
		int i;
		for(i=0;i<BITMAP_FONT_MAX;++i) {
			if (adv_font->data[i] && adv_font->data[i]!=&null_char) {
				adv_bitmap_free(adv_font->data[i]);
			}
		}
		free(adv_font);
	}
}

static int load_adv_font_data_fixed(adv_font* load_font, unsigned char* begin, unsigned start, unsigned count, unsigned width, unsigned height)
{
	unsigned i;

	for(i=0;i<start;++i)
		load_font->data[i] = &null_char;

	for(;i<start+count;++i) {
		unsigned x, y;
		adv_bitmap* bitmap;

		bitmap = adv_bitmap_alloc(width, height, 8);
		if (!bitmap) {
			return -1;
		}
		load_font->data[i] = bitmap;

		for(y=0;y<height;++y) {
			for(x=0;x<width;++x) {
				int set = begin[x/8] & (1 << (7-(x % 8)));
				adv_bitmap_pixel_put(bitmap, x, y, set ? 0xFF : 0);
			}
			begin += (width+7)/8;
		}
	}

	for(;i<BITMAP_FONT_MAX;++i)
		load_font->data[i] = &null_char;

	return 0;
}

static int load_adv_font_data_size(unsigned count, unsigned* width, unsigned height)
{
	unsigned size = 0;
	unsigned i;
	for(i=0;i<count;++i)
		size += ((width[i]+7)/8)*height;
	return size;
}

static int load_adv_font_data(adv_font* load_font, unsigned char* begin, unsigned start, unsigned count, unsigned* wtable, unsigned height)
{
	unsigned i;

	for(i=0;i<start;++i)
		load_font->data[i] = &null_char;

	for(;i<start+count;++i) {
		unsigned x, y;
		adv_bitmap* bitmap;
		unsigned width = wtable[i-start];

		bitmap = adv_bitmap_alloc(width, height, 8);
		if (!bitmap) {
			return -1;
		}
		load_font->data[i] = bitmap;

		for(y=0;y<height;++y) {
			for(x=0;x<width;++x) {
				int set = begin[x/8] & (1 << (7-(x % 8)));
				adv_bitmap_pixel_put(bitmap, x, y, set ? 0xFF : 0);
			}
			begin += (width+7)/8;
		}
	}

	for(;i<BITMAP_FONT_MAX;++i)
		load_font->data[i] = &null_char;

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

static adv_font* load_adv_font_psf(adv_fz* f)
{
	unsigned char header[2];
	unsigned char c;
	unsigned size;
	unsigned height;
	unsigned width;
	adv_font* load_font;
	unsigned char* data;
	unsigned data_size;

	width = 8;

	load_font = malloc(sizeof(adv_font));
	if (!load_font) {
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

	if (load_adv_font_data_fixed(load_font, data, 0, size, width, height)!=0) {
		goto out_data;
	}

	free(data);

	return load_font;

out_data:
	free(data);
out_font:
	free(load_font);
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

static adv_font* load_adv_font_raw(adv_fz* f)
{
	unsigned height;
	unsigned width;
	unsigned size;
	unsigned file_size;
	adv_font* load_font;
	unsigned char* data;
	unsigned data_size;

	load_font = malloc(sizeof(adv_font));
	if (!load_font) {
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

	if (load_adv_font_data_fixed(load_font, data, 0, size, width, height)!=0) {
		goto out_data;
	}

	free(data);

	return load_font;
out_data:
	free(data);
out_font:
	free(load_font);
out:
	return 0;
}

/****************************************************************************/
/* GRX */

#define GRX_FONT_MAGIC 0x19590214L

static adv_font* load_adv_font_grx(adv_fz* f)
{
	unsigned height;
	unsigned width;
	unsigned size, start, stop, isfixed;
	adv_font* load_font;
	unsigned char* data;
	unsigned data_size;
	unsigned* wtable;
	unsigned char header[56];

	load_font = malloc(sizeof(adv_font));
	if (!load_font)
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
		data_size = load_adv_font_data_size(size, wtable, height);
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
		if (load_adv_font_data(load_font, data, start, size, wtable, height)!=0) {
			goto out_data;
		}
	} else {
		if (load_adv_font_data_fixed(load_font, data, start, size, width, height)!=0) {
			goto out_data;
		}
	}

	free(data);
	free(wtable);

	return load_font;
out_data:
	free(data);
out_table:
	free(wtable);
out_font:
	free(load_font);
out:
	return 0;
}

/****************************************************************************/
/* Load */

/**
 * Load a font from a file.
 * The following formats are supported:
 *  - GRX
 *  - PSF
 *  - RAW
 */
adv_font* adv_font_load(adv_fz* f)
{
	adv_font* load_font;
	long pos;

	pos = fztell(f);

	load_font = load_adv_font_grx(f);
	if (load_font)
		return load_font;

	fzseek(f, pos, SEEK_SET); /* ignore error */

	load_font = load_adv_font_psf(f);
	if (load_font)
		return load_font;

	fzseek(f, pos, SEEK_SET); /* ignore error */

	load_font = load_adv_font_raw(f);
	if (load_font)
		return load_font;

	return 0;
}

#if 0
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

	adv_font_data = (uint8*)malloc(BITMAP_FONT_MAX * adv_font_dy);
	for(i=0;i<BITMAP_FONT_MAX;++i) {
		if (font->data[i]) {
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
		for(i=0;i<BITMAP_FONT_MAX;++i) {
			if (font->data[i])
				adv_bitmap_orientation(font->data[i], orientation_mask);
		}
	}
}

void adv_font_put_char(adv_font* font, adv_bitmap* dst, int x, int y, char c, unsigned color_front, unsigned color_back)
{
	adv_bitmap* src;
	unsigned cy;

	src = font->data[(unsigned char)c];
	if (!src)
		return;

	for(cy=0;cy<src->size_y;++cy) {
		unsigned char* src_ptr = adv_bitmap_line(src, cy);
		unsigned char* dst_ptr = adv_bitmap_pixel(dst, x, y);
		unsigned dp = dst->bytes_per_pixel;
		unsigned cx;
		for(cx=0;cx<src->size_x;++cx) {
			unsigned v = *src_ptr ? color_front : color_back;
			cpu_uint_write(dst_ptr, dp, v);
			dst_ptr += dp;
			src_ptr += 1;
		}
		++y;
	}
}

void adv_font_put_string(adv_font* font, adv_bitmap* dst, int x, int y, const char* begin, const char* end, unsigned color_front, unsigned color_back)
{
	while (begin != end) {
		adv_font_put_char(font, dst, x, y, *begin, color_front, color_back);
		x += adv_font_sizex_char(font, *begin);
		++begin;
	}
}

void adv_font_puttrasp_char(adv_font* font, adv_bitmap* dst, int x, int y, char c, unsigned color_front)
{
	adv_bitmap* src;
	unsigned cy;

	src = font->data[(unsigned char)c];
	if (!src)
		return;

	for(cy=0;cy<src->size_y;++cy) {
		unsigned char* src_ptr = adv_bitmap_line(src, cy);
		unsigned char* dst_ptr = adv_bitmap_pixel(dst, x, y);
		unsigned dp = dst->bytes_per_pixel;
		unsigned cx;
		for(cx=0;cx<src->size_x;++cx) {
			if (*src_ptr) {
				cpu_uint_write(dst_ptr, dp, color_front);
			}
			dst_ptr += dp;
			src_ptr += 1;
		}
		++y;
	}
}

void adv_font_puttrasp_string(adv_font* font, adv_bitmap* dst, int x, int y, const char* begin, const char* end, unsigned color_front)
{
	while (begin != end) {
		adv_font_puttrasp_char(font, dst, x, y, *begin, color_front);
		x += adv_font_sizex_char(font, *begin);
		++begin;
	}
}

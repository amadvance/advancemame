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

#include "rgb.h"
#include "png.h"
#include "mode.h"
#include "endianrw.h"

static char color_name_buffer[64];

/* YUV Conversion

   The following 2 sets of formulae are taken from information from Keith Jack's
   excellent book "Video Demystified" (ISBN 1-878707-09-4).

RGB to YUV Conversion

Y  =      (0.257 * R) + (0.504 * G) + (0.098 * B) + 16
Cr = V =  (0.439 * R) - (0.368 * G) - (0.071 * B) + 128
Cb = U = -(0.148 * R) - (0.291 * G) + (0.439 * B) + 128

YUV to RGB Conversion

B = 1.164(Y - 16)                   + 2.018(U - 128)
G = 1.164(Y - 16) - 0.813(V - 128) - 0.391(U - 128)
R = 1.164(Y - 16) + 1.596(V - 128)

   In both these cases, you have to clamp the output values to keep them in the
   [0-255] range. Rumour has it that the valid range is actually a subset of
   [0-255] (I've seen an RGB range of [16-235] mentioned) but clamping the values
   into [0-255] seems to produce acceptable results to me.

Further Information

   Julien (surname unknown) suggests that there are problems with the above
   formulae and suggests the following instead:

Y = 0.299R + 0.587G + 0.114B
U'= (B-Y)*0.565
V'= (R-Y)*0.713

   with reciprocal versions:

R = Y + 1.403V'
G = Y - 0.344U' - 0.714V'
B = Y + 1.770U'

http://www.funducode.com/freec/Fileformats/format3/format3c.htm

      Y =  0.299  R + 0.587  G + 0.114  B
      U = -0.1687 R - 0.3313 G + 0.5    B + 128
      V =  0.5    R - 0.4187 G - 0.0813 B + 128

      R = Y                   + 1.402   (V-128)
      G = Y - 0.34414 (U-128) - 0.71414 (V-128)
      B  =  Y   +   1. 772   (U-128)
               
http://www.cs.sfu.ca/undergrad/CourseMaterials/CMPT479/material/notes/Chap3/Chap3.3/Chap3.3.html

       Y = 0.299R + 0.587G + 0.114B 
       U = 0.492 (B - Y)
       V = 0.877 (R - Y)

http://www.cse.msu.edu/%7Ecbowen/docs/yuvtorgb.html

       Y = R *  .299 + G *  .587 + B *  .114;
       U = R * -.169 + G * -.332 + B *  .500 + 128.;
       V = R *  .500 + G * -.419 + B * -.0813 + 128.;

       R = Y + (1.4075 * (V - 128));
       G = Y - (0.3455 * (U - 128) - (0.7169 * (V - 128));
       B = Y + (1.7790 * (U - 128);

http://www.northpoleengineering.com/rgb2yuv.htm

The block diagram of the basic RGB to YUV color converter is shown below.
The color converter accepts 24-bits of RGB data and converts it to 24-bits
of YUV data using only combinational logic. The conversions are based on the
CCIR601 recommendations which implement an approximation of the color space
conversion formulas. The CCIR recommended formulas are shown below:

Y = (0.257 * R) + (0.504 * G) + (0.098 * B) + 16
U = (0.439 * R) - (0.368 * G) - (0.071 * B) + 128
V =-(0.148 * R) - (0.291 * G) + (0.439 * B) + 128

http://www.matrix-vision.com/faq_englisch/a1e.htm

RGB -> YUV:

Y = 0.30 R + 0.59 G + 0.11 B
U = 0.62 R - 0.52 G - 0.10 B + 128
V = -0.15 R - 0.29 G +0.44 B + 128

YUV - > RGB:

R = Y + 1.370 (U - 128)
G = Y - 0.698 (U - 128) - 0.336 (V - 128)
B = Y + 1.730 (V - 128)

*/

/**
 * Compute a RGB value with a specific format
 * \param r, g, b RGB values [0-255].
 * \param def_ordinal RGB format definition.
 * \return RGB nibble as ordinal value.
 */
adv_pixel pixel_make_from_def(unsigned r, unsigned g, unsigned b, adv_color_def def_ordinal)
{
	union adv_color_def_union def;
	def.ordinal = def_ordinal;

	switch (def.nibble.type) {
	case adv_color_type_rgb :
		return rgb_nibble_insert(r, rgb_shift_make_from_def(def.nibble.red_len, def.nibble.red_pos), rgb_mask_make_from_def(def.nibble.red_len, def.nibble.red_pos))
			| rgb_nibble_insert(g, rgb_shift_make_from_def(def.nibble.green_len, def.nibble.green_pos), rgb_mask_make_from_def(def.nibble.green_len, def.nibble.green_pos))
			| rgb_nibble_insert(b, rgb_shift_make_from_def(def.nibble.blue_len, def.nibble.blue_pos), rgb_mask_make_from_def(def.nibble.blue_len, def.nibble.blue_pos));
	case adv_color_type_yuy2 : {
/*
      Y =  0.299  R + 0.587  G + 0.114  B
      U = -0.1687 R - 0.3313 G + 0.5    B + 128
      V =  0.5    R - 0.4187 G - 0.0813 B + 128
*/
		unsigned y = ((19595*r + 38469*g + 7471*b) >> 16) & 0xFF;
		unsigned u = ((-11055*r - 21712*g + 32768*b + 8388608) >> 16) & 0xFF;
		unsigned v = ((32768*r - 27439*g - 5328*b + 8388608) >> 16) & 0xFF;

		return cpu_uint32_make_uint8(y, u, y, v);
		}
	default :
		/* return always 0 if the pixel is not computable, */
		/* for example in palettized modes */
		return 0;
	}
}

/**
 * Compute a RGBA value with a specific format
 * \param r, g, b, a RGBA values [0-255].
 * \param def_ordinal RGBA format definition.
 * \return RGBA nibble as ordinal value.
 */
adv_pixel alpha_make_from_def(unsigned r, unsigned g, unsigned b, unsigned a, adv_color_def def_ordinal)
{
	int alpha_shift;
	adv_pixel alpha_mask;
	adv_pixel p;

	alpha_shiftmask_get(&alpha_shift, &alpha_mask, def_ordinal);

	return pixel_make_from_def(r, g, b, def_ordinal)
		| rgb_nibble_insert(a, alpha_shift, alpha_mask);
}

/**
 * Compute a RGB value with a specific format
 * \param fr, fg, fb Foreground RGB values [0-255].
 * \param br, bg, bb Background RGB values [0-255].
 * \param a Alpha value [0-255].
 * \param def_ordinal RGB format definition.
 * \return RGB nibble as ordinal value.
 */
adv_pixel pixel_merge_from_def(unsigned fr, unsigned fg, unsigned fb, unsigned br, unsigned bg, unsigned bb, unsigned char a, adv_color_def def_ordinal)
{
	return pixel_make_from_def(
		(fr*a + (255-a)*br) / 255,
		(fg*a + (255-a)*bg) / 255,
		(fb*a + (255-a)*bb) / 255,
		def_ordinal
	);
}

/**
 * Get a textual description of a RGB format definition.
 * Return a string description in the format red_len/red_pos, green_len/green_pos, blue_len/blue_pos.
 * \return Pointer at a static buffer.
 */
const char* color_def_name_get(adv_color_def def_ordinal)
{
	union adv_color_def_union def;
	def.ordinal = def_ordinal;

	switch (def.nibble.type) {
	case adv_color_type_palette :
		snprintf(color_name_buffer, sizeof(color_name_buffer), "palette %d", color_def_bytes_per_pixel_get(def_ordinal) * 8);
		return color_name_buffer;
	case adv_color_type_rgb :
		snprintf(color_name_buffer, sizeof(color_name_buffer), "rgb %d-%d/%d,%d/%d,%d/%d",
			color_def_bytes_per_pixel_get(def_ordinal) * 8,
			def.nibble.red_len, def.nibble.red_pos,
			def.nibble.green_len, def.nibble.green_pos,
			def.nibble.blue_len, def.nibble.blue_pos
		);
		return color_name_buffer;
	case adv_color_type_yuy2 :
		snprintf(color_name_buffer, sizeof(color_name_buffer), "%s", "yuy2");
		return color_name_buffer;
	case adv_color_type_text :
		snprintf(color_name_buffer, sizeof(color_name_buffer), "text %d", color_def_bytes_per_pixel_get(def_ordinal) * 8);
		return color_name_buffer;
	default:
		snprintf(color_name_buffer, sizeof(color_name_buffer), "%s", "unknown");
		return color_name_buffer;
	}
}

/**
 * Get the bytes per pixel of a color definition.
 */
unsigned color_def_bytes_per_pixel_get(adv_color_def def_ordinal)
{
	union adv_color_def_union def;
	def.ordinal = def_ordinal;

	switch (def.nibble.type) {
	case adv_color_type_palette :
		return 1 + def.nibble.alpha_size;
	case adv_color_type_rgb :
		return (def.nibble.red_len + def.nibble.green_len + def.nibble.blue_len + 7) / 8 + def.nibble.alpha_size;
	case adv_color_type_yuy2 :
		return 4;
	case adv_color_type_text :
		return 2;
	default:
		return 0;
	}
}

/**
 * Get the type of a color definition.
 */
adv_color_type color_def_type_get(adv_color_def def_ordinal)
{
	union adv_color_def_union def;
	def.ordinal = def_ordinal;
	return def.nibble.type;
}

/**
 * Compute the size in bit of the mask.
 */
static unsigned rgb_len_get_from_mask(unsigned mask)
{
	unsigned len = 0;
	if (!mask)
		return len;
	while ((mask & 1) == 0)
		mask >>= 1;
	while (mask) {
		++len;
		mask >>= 1;
	}
	return len;
}

/**
 * Make an arbitrary color format definition from the specified color type.
 */
adv_color_def color_def_make(adv_color_type type)
{
	union adv_color_def_union def;
	def.ordinal = 0;

	def.nibble.type = type;

	return def.ordinal;
}

/**
 * Make an arbitrary color format definition from the specified index type.
 */
adv_color_def color_def_make_from_index(unsigned index)
{
	switch (index) {
	case MODE_FLAGS_INDEX_PALETTE8 :
		return color_def_make_palette_from_size(1);
	case MODE_FLAGS_INDEX_BGR8 :
		return color_def_make_rgb_from_sizelenpos(1, 3, 5, 3, 2, 2, 0);
	case MODE_FLAGS_INDEX_BGR15 :
		return color_def_make_rgb_from_sizelenpos(2, 5, 10, 5, 5, 5, 0);
	case MODE_FLAGS_INDEX_BGR16 :
		return color_def_make_rgb_from_sizelenpos(2, 5, 11, 6, 5, 5, 0);
	case MODE_FLAGS_INDEX_BGR24 :
		return color_def_make_rgb_from_sizelenpos(3, 8, 16, 8, 8, 8, 0);
	case MODE_FLAGS_INDEX_BGR32 :
		return color_def_make_rgb_from_sizelenpos(4, 8, 16, 8, 8, 8, 0);
	case MODE_FLAGS_INDEX_YUY2 :
		return color_def_make(adv_color_type_yuy2);
	default :
		return color_def_make(adv_color_type_unknown);
	}
}

/**
 * Make an arbitrary palette format definition.
 * \param bytes_per_pixel Bytes per pixel.
 * \return Color definition format.
 */
adv_color_def color_def_make_palette_from_size(unsigned bytes_per_pixel)
{
	union adv_color_def_union def;

	def.ordinal = 0;

	def.nibble.type = adv_color_type_palette;
	def.nibble.alpha_size = bytes_per_pixel - 1;

	return def.ordinal;
}

/**
 * Make an arbitrary RGB format definition.
 * \param bytes_per_pixel Bytes per pixel.
 * \param red_len Bits of the red channel.
 * \param red_pos Bit position red channel.
 * \param green_len Bits of the green channel.
 * \param green_pos Bit position green channel.
 * \param blue_len Bits of the blue channel.
 * \param blue_pos Bit position blue channel.
 * \return Color definition format.
 */
adv_color_def color_def_make_rgb_from_sizelenpos(unsigned bytes_per_pixel, unsigned red_len, unsigned red_pos, unsigned green_len, unsigned green_pos, unsigned blue_len, unsigned blue_pos)
{
	union adv_color_def_union def;

	def.ordinal = 0;

	def.nibble.type = adv_color_type_rgb;
	def.nibble.red_len = red_len;
	def.nibble.red_pos = red_pos;
	def.nibble.green_len = green_len;
	def.nibble.green_pos = green_pos;
	def.nibble.blue_len = blue_len;
	def.nibble.blue_pos = blue_pos;
	def.nibble.alpha_size = bytes_per_pixel - (red_len + green_len + blue_len + 7) / 8;

	return def.ordinal;
}

/**
 * Make an arbitrary RGB format definition from a shiftmask specification.
 * \param bytes_per_pixel Bytes per pixel.
 * \param red_mask, green_mask, blue_mask Bit mask.
 * \param red_shift, green_shift, blue_shift Shift.
 */
adv_color_def color_def_make_rgb_from_sizeshiftmask(unsigned bytes_per_pixel, int red_shift, unsigned red_mask, int green_shift, unsigned green_mask, int blue_shift, unsigned blue_mask)
{
	unsigned red_len = rgb_len_get_from_mask(red_mask);
	unsigned green_len = rgb_len_get_from_mask(green_mask);
	unsigned blue_len = rgb_len_get_from_mask(blue_mask);
	unsigned red_pos = 8 + red_shift - red_len;
	unsigned green_pos = 8 + green_shift - green_len;
	unsigned blue_pos = 8 + blue_shift - blue_len;

	return color_def_make_rgb_from_sizelenpos(bytes_per_pixel, red_len, red_pos, green_len, green_pos, blue_len, blue_pos);
}

/**
 * Compute the distance of two colors.
 */
unsigned video_color_dist(const adv_color_rgb* A, const adv_color_rgb* B)
{
	int r, g, b;
	r = (int)A->red - B->red;
	g = (int)A->green - B->green;
	b = (int)A->blue - B->blue;
	return r*r + g*g + b*b;
}

/**
 * Find the nearest color in a palette.
 */
unsigned video_color_find(unsigned r, unsigned g, unsigned b, const adv_color_rgb* palette_map, unsigned palette_mac)
{
	unsigned dist;
	unsigned index;
	unsigned i;
	adv_color_rgb c;

	c.red = r;
	c.green = g;
	c.blue = b;

	dist = video_color_dist(&c, palette_map);
	index = 0;

	for(i=1;i<palette_mac;++i) {
		unsigned new_dist = video_color_dist(&c, palette_map + i);

		if (new_dist == 0)
			return i;

		if (new_dist < dist) {
			index = i;
			dist = new_dist;
		}
	}

	return index;
}

/**
 * Adjust a 8 bit channel computed from a n bit value.
 * The value is adjusted to save the black and white colors.
 * \param value 8 bit value with lower bits at 0.
 * \param len Original number of bit of the channel value before the 8 bit expansion.
 * \return Adjusted 8 bit channel value.
 */
unsigned rgb_approx(unsigned value, unsigned len)
{
	unsigned fill = len;
	while (fill < 8) {
		value |= value >> fill;
		fill *= 2;
	}
	return value;
}

/**
 * Compute the shift required for a color conversion.
 * The conversion must be done with DST = rgb_shift(SRC, shift) & mask;
 */
int rgb_conv_shift_get(unsigned s_len, unsigned s_pos, unsigned d_len, unsigned d_pos)
{
	return rgb_shift_make_from_def(s_len, s_pos) - rgb_shift_make_from_def(d_len, d_pos);
}

/**
 * Compute the mask required for a color conversion.
 * The conversion must be done with DST = rgb_shift(SRC, shift) & mask;
 */
adv_pixel rgb_conv_mask_get(unsigned s_len, unsigned s_pos, unsigned d_len, unsigned d_pos)
{
	return rgb_mask_make_from_def(d_len, d_pos) & rgb_shift(rgb_mask_make_from_def(s_len, s_pos), rgb_conv_shift_get(s_len, s_pos, d_len, d_pos));
}

/**
 * Compute the shift and mask values for the alpha channel.
 */
void alpha_shiftmask_get(int* shift, unsigned* mask, adv_color_def def_ordinal)
{
	union adv_color_def_union def;
	unsigned pos;
	unsigned len;

	def.ordinal = def_ordinal;

	assert(def.nibble.type == adv_color_type_rgb);

	len = def.nibble.alpha_size * 8;
	if (def.nibble.red_pos == 0 || def.nibble.green_pos == 0 || def.nibble.blue_pos == 0) {
		pos = color_def_bytes_per_pixel_get(def_ordinal) * 8 - len;
	} else {
		pos = 0;
	}

	rgb_shiftmask_get(shift, mask, len, pos);
}


/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999-2002 Andrea Mazzoleni
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

#include "mng.h"
#include "png.h"
#include "endianrw.h"

#include <stdlib.h>
#include <string.h>

/**************************************************************************************/
/* MNG */

static unsigned char MNG_Signature[] = "\x8A\x4D\x4E\x47\x0D\x0A\x1A\x0A";

int mng_read_signature(FZ* f)
{
	unsigned char signature[8];

	if (fzread(signature,8,1,f) != 1) {
		png_error("Error reading the signature");
		return -1;
	}

	if (memcmp(signature,MNG_Signature,8)!=0) {
		png_error("Invalid MNG signature");
		return -1;
	}

	return 0;
}

int mng_write_signature(FZ* f, unsigned* count)
{
	if (fzwrite(MNG_Signature, 8, 1, f) != 1) {
		png_error("Error writing the signature");
		return -1;
	}

	if (count)
		*count += 8;

	return 0;
}

struct mng_context {
	int end_flag; /**< End flag */
	unsigned pixel; /**< Bytes per pixel */
	unsigned char* dat_ptr; /**< Current image buffer */
	unsigned dat_size; /**< Size of the buffer image */
	unsigned dat_line; /**< Bytes per scanline */
	int dat_x;
	int dat_y;
	unsigned dat_width;
	unsigned dat_height;

	unsigned char* dlt_ptr; /**< Delta buffer */
	unsigned dlt_size;
	unsigned dlt_line; /**< Bytes per scanline */

	unsigned char pal_ptr[256*3];
	unsigned pal_size;

	unsigned frame_frequency; /**< Base frame rate */
	unsigned frame_tick; /**< Ticks for a generic frame */
	unsigned frame_width; /**< Frame width */
	unsigned frame_height; /**< Frame height */
};

static int mng_read_ihdr(struct mng_context* mng, FZ* f, const unsigned char* ihdr, unsigned ihdr_size)
{
	unsigned type;
	unsigned char* data;
	unsigned size;
	unsigned long dat_size;

	if (ihdr_size != 13) {
		png_error("Invalid IHDR size");
		goto err;
	}

	mng->dat_width = be_uint32_read(ihdr + 0);
	mng->dat_height = be_uint32_read(ihdr + 4);
	if (mng->dat_x + mng->frame_width > mng->dat_width) {
		png_error("Frame not complete");
		goto err;
	}
	if (mng->dat_y + mng->frame_height > mng->dat_height) {
		png_error("Frame not complete");
		goto err;
	}

	if (ihdr[8] != 8) { /* bit depth */
		png_error_unsupported("Unsupported bit depth");
		goto err;
	}
	if (ihdr[9] == 3) /* color type */
		mng->pixel = 1;
	else if (ihdr[9] == 2)
		mng->pixel = 3;
	else if (ihdr[9] == 6)
		mng->pixel = 4;
	else {
		png_error_unsupported("Unsupported color type");
		goto err;
	}
	if (ihdr[10] != 0) { /* compression */
		png_error_unsupported("Unsupported compression type");
		goto err;
	}
	if (ihdr[11] != 0) { /* filter */
		png_error_unsupported("unsupported Filter type");
		goto err;
	}
	if (ihdr[12] != 0) { /* interlace */
		png_error_unsupported("Unsupported interlace type");
		goto err;
	}

	if (!mng->dat_ptr) {
		mng->dat_line = mng->dat_width * mng->pixel + 1; /* +1 for the filter byte */
		mng->dat_size = mng->dat_height * mng->dat_line;
		mng->dat_ptr = malloc(mng->dat_size);
	} else {
		if (mng->dat_line != mng->dat_width * mng->pixel + 1
			|| mng->dat_size != mng->dat_height * mng->dat_line) {
			png_error_unsupported("Unsupported size change");
			goto err;
		}
	}
	if (!mng->dlt_ptr) {
		mng->dlt_line = mng->frame_width * mng->pixel + 1; /* +1 for the filter byte */
		mng->dlt_size = mng->frame_height * mng->dlt_line;
		mng->dlt_ptr = malloc(mng->dlt_size);
	}

	if (mng->pixel == 1) {
		if (png_read_chunk(f, &data, &size, &type) != 0)
			goto err;

		if (type != PNG_CN_PLTE) {
			png_error("Missing PLTE chunk");
			goto err_data;
		}

		if (size % 3 != 0 || size / 3 > 256) {
			png_error("Invalid palette size in PLTE chunk");
			goto err_data;
		}

		mng->pal_size = size;
		memcpy(mng->pal_ptr, data, size);

		free(data);
	}

	if (png_read_chunk(f, &data, &size, &type) != 0)
		goto err;

	if (type != PNG_CN_IDAT) {
		png_error("Missing IDAT chunk");
		goto err_data;
	}

	dat_size = mng->dat_size;
	if (uncompress(mng->dat_ptr, &dat_size, data, size) != Z_OK) {
		png_error("Corrupt compressed data");
		goto err_data;
	}

	free(data);

	if (dat_size != mng->dat_size) {
		png_error("Corrupt compressed data");
		goto err;
	}

	if (mng->pixel == 1)
		png_unfilter_8(mng->dat_width * mng->pixel, mng->dat_height, mng->dat_ptr, mng->dat_line);
	else if (mng->pixel == 3)
		png_unfilter_24(mng->dat_width * mng->pixel, mng->dat_height, mng->dat_ptr, mng->dat_line);
	else if (mng->pixel == 4)
		png_unfilter_32(mng->dat_width * mng->pixel, mng->dat_height, mng->dat_ptr, mng->dat_line);

	if (png_read_chunk(f, &data, &size, &type) != 0)
		goto err;

	if (png_read_iend(f, data, size, type) != 0)
		goto err_data;

	free(data);

	return 0;

err_data:
	free(data);
err:
	return -1;
}

static int mng_read_defi(struct mng_context* mng, unsigned char* defi, unsigned defi_size)
{
	unsigned id;

	if (defi_size != 4 && defi_size != 12) {
		png_error_unsupported("Unsupported DEFI size");
		return -1;
	}

	id = be_uint16_read(defi + 0);
	if (id != 1) {
		png_error_unsupported("Unsupported id number in DEFI chunk");
		return -1;
	}
	if (defi[2] != 0) { /* visible */
		png_error_unsupported("Unsupported visible type in DEFI chunk");
		return -1;
	}
	if (defi[3] != 1) { /* concrete */
		png_error_unsupported("Unsupported concrete type in DEFI chunk");
		return -1;
	}

	if (defi_size >= 12) {
		mng->dat_x = - (int)be_uint32_read(defi + 4);
		mng->dat_y = - (int)be_uint32_read(defi + 8);
	} else {
		mng->dat_x = 0;
		mng->dat_y = 0;
	}

	return 0;
}

static int mng_read_move(struct mng_context* mng, FZ* f, unsigned char* move, unsigned move_size)
{
	unsigned id;

	if (move_size != 13) {
		png_error_unsupported("Unsupported MOVE size in MOVE chunk");
		return -1;
	}

	id = be_uint16_read(move + 0);
	if (id != 1) {
		png_error_unsupported("Unsupported id number in MOVE chunk");
		return -1;
	}

	id = be_uint16_read(move + 2);
	if (id != 1) {
		png_error_unsupported("Unsupported id number in MOVE chunk");
		return -1;
	}

	if (move[4] == 0) { /* replace */
		mng->dat_x = - (int)be_uint32_read(move + 5);
		mng->dat_y = - (int)be_uint32_read(move + 9);
	} else if (move[4] == 1) { /* adding */
		mng->dat_x += - (int)be_uint32_read(move + 5);
		mng->dat_y += - (int)be_uint32_read(move + 9);
	} else {
		png_error_unsupported("Unsupported move type in MOVE chunk");
		return -1;
	}

	return 0;
}

static void mng_delta_replacement(struct mng_context* mng, unsigned pos_x, unsigned pos_y, unsigned width, unsigned height)
{
	unsigned i;
	unsigned bytes_per_run = width * mng->pixel;
	unsigned delta_bytes_per_scanline = bytes_per_run + 1;
	unsigned char* p0 = mng->dat_ptr + pos_y * mng->dat_line + pos_x * mng->pixel + 1;
	unsigned char* p1 = mng->dlt_ptr + 1;

	for(i=0;i<height;++i) {
		memcpy(p0, p1, bytes_per_run);
		p0 += mng->dat_line;
		p1 += delta_bytes_per_scanline;
	}
}

static void mng_delta_addition(struct mng_context* mng, unsigned pos_x, unsigned pos_y, unsigned width, unsigned height)
{
	unsigned i,j;
	unsigned bytes_per_run = width * mng->pixel;
	unsigned delta_bytes_per_scanline = bytes_per_run + 1;
	unsigned char* p0 = mng->dat_ptr + pos_y * mng->dat_line + pos_x * mng->pixel + 1;
	unsigned char* p1 = mng->dlt_ptr + 1;

	for(i=0;i<height;++i) {
		for(j=0;j<bytes_per_run;++j) {
			*p0++ += *p1++;
		}
		p0 += mng->dat_line - bytes_per_run;
		p1 += delta_bytes_per_scanline - bytes_per_run;
	}
}

static int mng_read_delta(struct mng_context* mng, FZ* f, unsigned char* dhdr, unsigned dhdr_size)
{
	unsigned type;
	unsigned char* data;
	unsigned size;
	unsigned width;
	unsigned height;
	unsigned pos_x;
	unsigned pos_y;
	unsigned id;
	unsigned ope;

	if (dhdr_size != 4 && dhdr_size != 12 && dhdr_size != 20) {
		png_error_unsupported("Unsupported DHDR size");
		goto err;
	}

	id = be_uint16_read(dhdr + 0);
	if (id != 1) /* object id 1 */ {
		png_error_unsupported("Unsupported id number in DHDR chunk");
		goto err;
	}

	if (dhdr[2] != 1) /* PNG stream without IHDR header */ {
		png_error_unsupported("Unsupported delta type in DHDR chunk");
		goto err;
	}

	ope = dhdr[3];
	if (ope != 0 && ope != 1 && ope != 4 && ope != 7) {
		png_error_unsupported("Unsupported delta operation in DHDR chunk");
		goto err;
	}

	if (!mng->dat_ptr || !mng->dlt_ptr) {
		png_error("Invalid delta context in DHDR chunk");
		goto err;
	}

	if (dhdr_size >= 12) {
		width = be_uint32_read(dhdr + 4);
		height = be_uint32_read(dhdr + 8);
	} else {
		width = mng->frame_width;
		height = mng->frame_height;
	}

	if (dhdr_size >= 20) {
		pos_x = be_uint32_read(dhdr + 12);
		pos_y = be_uint32_read(dhdr + 16);
	} else {
		pos_x = 0;
		pos_y = 0;
	}

	if (png_read_chunk(f, &data, &size, &type) != 0)
		goto err;

	if (type == PNG_CN_PLTE) {
		if (mng->pixel != 1) {
			png_error("Unexpected PLTE chunk");
			goto err_data;
		}

		if (size % 3 != 0 || size / 3 > 256) {
			png_error("Invalid palette size");
			goto err_data;
		}

		mng->pal_size = size;
		memcpy(mng->pal_ptr, data, size);

		free(data);

		if (png_read_chunk(f, &data, &size, &type) != 0)
			goto err;
	}

	if (type == MNG_CN_PPLT) {
		unsigned i;
		if (mng->pixel != 1) {
			png_error("Unexpected PPLT chunk");
			goto err_data;
		}

		if (data[0] != 0) { /* RGB replacement */
			png_error("Unsupported palette operation in PPLT chunk");
			goto err_data;
		}

		i = 1;
		while (i < size) {
			unsigned v0,v1,delta_size;
			if (i + 2 > size) {
				png_error("Invalid palette size in PPLT chunk");
				goto err_data;
			}
			v0 = data[i++];
			v1 = data[i++];
			delta_size = (v1 - v0 + 1) * 3;
			if (i + delta_size > size) {
				png_error("Invalid palette format in PPLT chunk");
				goto err_data;
			}
			memcpy(mng->pal_ptr + v0 * 3, data + i, delta_size);
			i += delta_size;
		}

		free(data);

		if (png_read_chunk(f, &data, &size, &type) != 0)
			goto err;
	}

	if (type == PNG_CN_IDAT) {
		unsigned long dlt_size;

		if (pos_x + width > mng->dat_width || pos_y + height > mng->dat_height) {
			png_error("Frame not complete in IDAT chunk");
			goto err_data;
		}

		dlt_size = mng->dat_size;
		if (uncompress(mng->dlt_ptr, &dlt_size, data, size) != Z_OK) {
			png_error("Corrupt compressed data in IDAT chunk");
			goto err_data;
		}

		if (ope == 0 || ope == 4) {
			mng_delta_replacement(mng, pos_x, pos_y, width, height);
		} else if (ope == 1) {
			mng_delta_addition(mng, pos_x, pos_y, width, height);
		} else {
			png_error_unsupported("Unsupported delta operation");
			goto err_data;
		}

		free(data);

		if (png_read_chunk(f, &data, &size, &type) != 0)
			goto err;
	} else {
		if (ope != 7) {
			png_error_unsupported("Unsupported delta operation");
			goto err_data;
		}
	}

	if (png_read_iend(f, data, size, type) != 0)
		goto err_data;

	free(data);

	return 0;

err_data:
	free(data);
err:
	return -1;
}

static void mng_import(
	struct mng_context* mng,
	unsigned* pix_width, unsigned* pix_height, unsigned* pix_pixel,
	unsigned char** dat_ptr, unsigned* dat_size,
	unsigned char** pix_ptr, unsigned* pix_scanline,
	unsigned char** pal_ptr, unsigned* pal_size
) {
	unsigned char* current_ptr = mng->dat_ptr + mng->dat_x * mng->pixel + mng->dat_y * mng->dat_line + 1;

	*pix_width = mng->frame_width;
	*pix_height = mng->frame_height;
	*pix_pixel = mng->pixel;

	if (mng->pixel == 1) {
		*pal_ptr = malloc(mng->pal_size);
		memcpy(*pal_ptr, mng->pal_ptr, mng->pal_size);
		*pal_size = mng->pal_size;
	} else {
		*pal_ptr = 0;
		*pal_size = 0;
	}

	*dat_ptr = 0;
	*dat_size = 0;

	*pix_ptr = current_ptr;
	*pix_scanline = mng->dat_line;
}

/**
 * Read a MNG image.
 * \return
 *   - == 0 ok
 *   - == 1 end of the mng stream
 *   - < 0 error
 */
int mng_read(
	void* void_mng,
	unsigned* pix_width, unsigned* pix_height, unsigned* pix_pixel,
	unsigned char** dat_ptr, unsigned* dat_size,
	unsigned char** pix_ptr, unsigned* pix_scanline,
	unsigned char** pal_ptr, unsigned* pal_size,
	unsigned* tick,
	FZ* f
) {
	struct mng_context* mng = (struct mng_context*)void_mng;
	unsigned type;
	unsigned char* data;
	unsigned size;

	if (mng->end_flag)
		return -1;

	*tick = mng->frame_tick;

	while (1) {
		if (png_read_chunk(f, &data, &size, &type) != 0)
			goto err;

		switch (type) {
			case MNG_CN_DEFI :
				if (mng_read_defi(mng,data,size) != 0)
					goto err_data;
				free(data);
				break;
			case MNG_CN_MOVE :
				if (mng_read_move(mng,f,data,size) != 0)
					goto err_data;
				free(data);
				break;
			case PNG_CN_IHDR :
				if (mng_read_ihdr(mng,f,data,size) != 0)
					goto err_data;
				free(data);
				mng_import(mng, pix_width, pix_height, pix_pixel, dat_ptr, dat_size, pix_ptr, pix_scanline, pal_ptr, pal_size);
				return 0;
			case MNG_CN_DHDR :
				if (mng_read_delta(mng, f,data,size) != 0)
					goto err_data;
				free(data);
				mng_import(mng, pix_width, pix_height, pix_pixel, dat_ptr, dat_size, pix_ptr, pix_scanline, pal_ptr, pal_size);
				return 0;
			case MNG_CN_MEND :
				mng->end_flag = 1;
				free(data);
				return 1;
			case MNG_CN_FRAM :
				if (size > 1) {
					unsigned i = 1;
					while (i < size && data[i])
						++i;
					if (size >= i+9) {
						unsigned v = be_uint32_read(data + i+5);
						if (v < 1)
							v = 1;
						if (data[i+1] == 1 || data[i+1] == 2)
							*tick = v;
						if (data[i+1] == 2)
							mng->frame_tick = v;
					}
				}
				free(data);
				break;
			case MNG_CN_BACK :
				/* ignored OUTOFSPEC */
				free(data);
				break;
			case MNG_CN_LOOP :
			case MNG_CN_ENDL :
			case MNG_CN_SAVE :
			case MNG_CN_SEEK :
			case MNG_CN_TERM :
				/* ignored */
				free(data);
				break;
			default :
				/* ancillary bit. bit 5 of first byte. 0 (uppercase) = critical, 1 (lowercase) = ancillary. */
				if ((type & 0x20000000) == 0) {
					char buf[4];
					be_uint32_write(buf, type);
					png_error_unsupported("Unsupported critical chunk '%c%c%c%c'", buf[0], buf[1], buf[2], buf[3]);
					goto err_data;
				}
				/* ignored */
				free(data);
				break;
		}
	}

	free(data);

	return 1;

err_data:
	free(data);
err:
	return 0;
}

void* mng_init(FZ* f)
{
	struct mng_context* mng;

	unsigned type;
	unsigned char* data;
	unsigned size;
	unsigned simplicity;

	mng = malloc(sizeof(struct mng_context));
	if (!mng)
		goto err;

	mng->end_flag = 0;
	mng->pixel = 0;
	mng->dat_ptr = 0;
	mng->dat_size = 0;
	mng->dat_line = 0;
	mng->dat_x = 0;
	mng->dat_y = 0;
	mng->dat_width = 0;
	mng->dat_height = 0;
	mng->dlt_ptr = 0;
	mng->dlt_size = 0;
	mng->dlt_line = 0;

	if (mng_read_signature(f) != 0)
		goto err_mng;

	if (png_read_chunk(f, &data, &size, &type) != 0)
		goto err_mng;

	if (type != MNG_CN_MHDR) {
		png_error("Missing MHDR chunk\n");
		goto err_data;
	}

	if (size != 28) {
		png_error("Invalid MHDR size\n");
		goto err_data;
	}

	mng->frame_width = be_uint32_read(data + 0);
	mng->frame_height = be_uint32_read(data + 4);
	mng->frame_frequency = be_uint32_read(data + 8);
	if (mng->frame_frequency < 1)
		mng->frame_frequency = 1;
	mng->frame_tick = 1;
	simplicity = be_uint32_read(data + 24);

	free(data);

	return mng;

err_data:
	free(data);
err_mng:
	free(mng);
err:
	return 0;
}

void mng_done(void* void_mng)
{
	struct mng_context* mng = (struct mng_context*)void_mng;

	free(mng->dat_ptr);
	free(mng->dlt_ptr);
	free(mng);
}

unsigned mng_frequency_get(void* void_mng) {
	struct mng_context* mng = (struct mng_context*)void_mng;

	return mng->frame_frequency;
}

unsigned mng_width_get(void* void_mng) {
	struct mng_context* mng = (struct mng_context*)void_mng;

	return mng->frame_width;
}

unsigned mng_height_get(void* void_mng) {
	struct mng_context* mng = (struct mng_context*)void_mng;

	return mng->frame_height;
}

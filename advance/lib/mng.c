/*
 * This file is part of the AdvanceMAME project.
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

#include "os.h"

#include <stdlib.h>
#include <string.h>

/**************************************************************************************/
/* MNG */

#define PNG_CN_IHDR 0x49484452
#define PNG_CN_PLTE 0x504C5445
#define PNG_CN_IDAT 0x49444154
#define PNG_CN_IEND 0x49454E44
#define MNG_CN_DHDR 0x44484452
#define MNG_CN_MHDR 0x4D484452
#define MNG_CN_MEND 0x4D454E44
#define MNG_CN_DEFI 0x44454649
#define MNG_CN_PPLT 0x50504c54
#define MNG_CN_MOVE 0x4d4f5645
#define MNG_CN_TERM 0x5445524d
#define MNG_CN_SAVE 0x53415645
#define MNG_CN_SEEK 0x5345454b
#define MNG_CN_LOOP 0x4c4f4f50
#define MNG_CN_ENDL 0x454e444c
#define MNG_CN_BACK 0x4241434b
#define MNG_CN_FRAM 0x4652414d

static unsigned char MNG_Signature[] = "\x8A\x4D\x4E\x47\x0D\x0A\x1A\x0A";

static unsigned read_be_u32(unsigned char* v) {
	return v[0] << 24 | v[1] << 16 | v[2] << 8 | v[3];
}

static unsigned read_be_u16(unsigned char* v) {
	return v[0] << 8 | v[1];
}

static int mng_read_signature(FZ* f) {
	unsigned char signature[8];

	if (fzread(signature,8,1,f) != 1) {
		return -1;
	}

	if (memcmp(signature,MNG_Signature,8)!=0) {
		return -1;
	}

	return 0;
}

//---------------------------------------------------------------------------
// Stream

struct mng_context {
	int end_flag; /** End flag */
	unsigned pixel; /** Bytes per pixel */
	unsigned char* dat_ptr; /** Current image buffer */
	unsigned dat_size; /** Size of the buffer image */
	unsigned dat_line; /** Bytes per scanline */
	int dat_x;
	int dat_y;
	unsigned dat_width;
	unsigned dat_height;

	unsigned char* dlt_ptr; /** Delta buffer */
	unsigned dlt_size;
	unsigned dlt_line; /** Bytes per scanline */

	unsigned char pal_ptr[256*3];
	unsigned pal_size;

	unsigned frame_frequency; /** Base frame rate */
	unsigned frame_tick; /** Ticks for a generic frame */
	unsigned frame_width; /** Frame width */
	unsigned frame_height; /** Frame height */
};

static int mng_load_png(struct mng_context* mng, FZ* f, unsigned char* ihdr, unsigned ihdr_size) {
	unsigned type;
	unsigned char* data;
	unsigned size;
	unsigned i;
	unsigned long dat_size;

	if (ihdr_size != 13) {
		log_std(("mng: invalid IHDR size\n"));
		goto err;
	}

	mng->dat_width = read_be_u32(ihdr + 0);
	mng->dat_height = read_be_u32(ihdr + 4);
	if (mng->dat_x + mng->frame_width > mng->dat_width) {
		log_std(("mng: frame not complete\n"));
		goto err;
	}
	if (mng->dat_y + mng->frame_height > mng->dat_height) {
		log_std(("mng: frame not complete\n"));
		goto err;
	}

	if (ihdr[8] != 8) { /* bit depth */
		log_std(("mng: unsupported bit depth\n"));
		goto err;
	}
	if (ihdr[9] == 3) /* color type */
		mng->pixel = 1;
	else if (ihdr[9] == 2)
		mng->pixel = 3;
	else if (ihdr[9] == 6)
		mng->pixel = 4;
	else {
		log_std(("mng: unsupported color type\n"));
		goto err;
	}
	if (ihdr[10] != 0) { /* compression */
		log_std(("mng: unsupported compression type\n"));
		goto err;
	}
	if (ihdr[11] != 0) { /* filter */
		log_std(("mng: unsupported filter type\n"));
		goto err;
	}
	if (ihdr[12] != 0) { /* interlace */
		log_std(("mng: interlace not supported\n"));
		goto err;
	}

	if (!mng->dat_ptr) {
		mng->dat_line = mng->dat_width * mng->pixel + 1; /* +1 for the filter byte */
		mng->dat_size = mng->dat_height * mng->dat_line;
		mng->dat_ptr = malloc(mng->dat_size);
	} else {
		if (mng->dat_line != mng->dat_width * mng->pixel + 1
			|| mng->dat_size != mng->dat_height * mng->dat_line) {
			log_std(("mng: unsupported size change\n"));
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
			log_std(("mng: missing PLTE chunk\n"));
			goto err_data;
		}

		if (size % 3 != 0 || size / 3 > 256) {
			log_std(("mng: invalid palette size\n"));
			goto err_data;
		}

		mng->pal_size = size;
		memcpy(mng->pal_ptr, data, size);

		free(data);
	}

	if (png_read_chunk(f, &data, &size, &type) != 0)
		goto err;

	if (type != PNG_CN_IDAT) {
		log_std(("mng: missing IDAT chunk\n"));
		goto err_data;
	}

	dat_size = mng->dat_size;
	if (uncompress(mng->dat_ptr, &dat_size, data, size) != Z_OK) {
		log_std(("mng: corrupt compressed data\n"));
		goto err_data;
	}

	free(data);

	if (dat_size != mng->dat_size) {
		log_std(("mng: corrupt compressed data\n"));
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

	if (type != PNG_CN_IEND) {
		log_std(("mng: missing IEND chunk\n"));
		goto err_data;
	}

	free(data);

	return 0;

err_data:
	free(data);
err:
	return -1;
}

static int mng_load_defi(struct mng_context* mng, unsigned char* defi, unsigned defi_size) {
	unsigned id;

	if (defi_size != 4 && defi_size != 12) {
		log_std(("mng: unsupported DEFI size\n"));
		return -1;
	}

	id = read_be_u16(defi + 0);
	if (id != 1) {
		log_std(("mng: unsupported id number\n"));
		return -1;
	}
	if (defi[2] != 0) { /* visible */
		log_std(("mng: unsupported visible type\n"));
		return -1;
	}
	if (defi[3] != 1) { /* concrete */
		log_std(("mng: unsupported concrete type\n"));
		return -1;
	}

	if (defi_size >= 12) {
		mng->dat_x = - (int)read_be_u32(defi + 4);
		mng->dat_y = - (int)read_be_u32(defi + 8);
	} else {
		mng->dat_x = 0;
		mng->dat_y = 0;
	}

	return 0;

err:
	return -1;
}

static int mng_load_move(struct mng_context* mng, FZ* f, unsigned char* move, unsigned move_size) {
	unsigned id;

	if (move_size != 13) {
		log_std(("mng: unsupported MOVE size\n"));
		return -1;
	}

	id = read_be_u16(move + 0);
	if (id != 1) {
		log_std(("mng: unsupported id number\n"));
		return -1;
	}

	id = read_be_u16(move + 2);
	if (id != 1) {
		log_std(("mng: unsupported id number\n"));
		return -1;
	}

	if (move[4] == 0) { /* replace */
		mng->dat_x = - (int)read_be_u32(move + 5);
		mng->dat_y = - (int)read_be_u32(move + 9);
	} else if (move[4] == 1) { /* adding */
		mng->dat_x += - (int)read_be_u32(move + 5);
		mng->dat_y += - (int)read_be_u32(move + 9);
	} else {
		log_std(("mng: unsupported move type\n"));
		return -1;
	}

	return 0;

err:
	return -1;
}

static void mng_delta_replacement(struct mng_context* mng, unsigned pos_x, unsigned pos_y, unsigned width, unsigned height) {
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

static void mng_delta_addition(struct mng_context* mng, unsigned pos_x, unsigned pos_y, unsigned width, unsigned height) {
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

static int mng_load_delta(struct mng_context* mng, FZ* f, unsigned char* dhdr, unsigned dhdr_size) {
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
		log_std(("mng: unsupported DHDR size\n"));
		goto err;
	}

	id = read_be_u16(dhdr + 0);
	if (id != 1) /* object id 1 */ {
		log_std(("mng: unsupported id number\n"));
		goto err;
	}

	if (dhdr[2] != 1) /* PNG stream without IHDR header */ {
		log_std(("mng: unsupported delta type\n"));
		goto err;
	}

	ope = dhdr[3];
	if (ope != 0 && ope != 1 && ope != 4 && ope != 7) {
		log_std(("mng: unsupported delta operation\n"));
		goto err;
	}

	if (!mng->dat_ptr || !mng->dlt_ptr) {
		log_std(("mng: invalid delta context\n"));
		goto err;
	}

	if (dhdr_size >= 12) {
		width = read_be_u32(dhdr + 4);
		height = read_be_u32(dhdr + 8);
	} else {
		width = mng->frame_width;
		height = mng->frame_height;
	}

	if (dhdr_size >= 20) {
		pos_x = read_be_u32(dhdr + 12);
		pos_y = read_be_u32(dhdr + 16);
	} else {
		pos_x = 0;
		pos_y = 0;
	}

	if (png_read_chunk(f, &data, &size, &type) != 0)
		goto err;

	if (type == PNG_CN_PLTE) {
		if (mng->pixel != 1) {
			log_std(("mng: unexpected PLTE chunk\n"));
			goto err_data;
		}

		if (size % 3 != 0 || size / 3 > 256) {
			log_std(("mng: invalid palette size\n"));
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
			log_std(("mng: unexpected PPLT chunk\n"));
			goto err_data;
		}

		if (data[0] != 0) { /* RGB replacement */
			log_std(("mng: unsupported palette operation\n"));
			goto err_data;
		}

		i = 1;
		while (i < size) {
			unsigned v0,v1,delta_size;
			if (i + 2 > size)
				goto err_data;
			v0 = data[i++];
			v1 = data[i++];
			delta_size = (v1 - v0 + 1) * 3;
			if (i + delta_size > size) {
				log_std(("mng: invalid palette format\n"));
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
			log_std(("mng: frame not complete\n"));
			goto err_data;
		}

		dlt_size = mng->dat_size;
		if (uncompress(mng->dlt_ptr, &dlt_size, data, size) != Z_OK) {
			log_std(("mng: corrupt compressed data\n"));
			goto err_data;
		}

		if (ope == 0 || ope == 4) {
			mng_delta_replacement(mng, pos_x, pos_y, width, height);
		} else if (ope == 1) {
			mng_delta_addition(mng, pos_x, pos_y, width, height);
		} else {
			log_std(("mng: unsupported delta operation\n"));
			goto err_data;
		}

		free(data);

		if (png_read_chunk(f, &data, &size, &type) != 0)
			goto err;
	} else {
		if (ope != 7) {
			log_std(("mng: unsupported delta operation\n"));
			goto err_data;
		}
	}

	if (type != PNG_CN_IEND) {
		log_std(("mng: missing IEND chunk\n"));
		goto err_data;
	}

	free(data);

	return 0;

err_data:
	free(data);
err:
	return -1;
}

static struct bitmap* mng_import(struct mng_context* mng, video_color* rgb, unsigned* rgb_max) {
	unsigned char* current_ptr = mng->dat_ptr + mng->dat_x * mng->pixel + mng->dat_y * mng->dat_line + 1;

	if (mng->pixel == 1) {
		unsigned char* p = mng->pal_ptr;
		unsigned n = mng->pal_size / 3;
		unsigned i;
		for(i=0;i<n;++i) {
			rgb[i].red = *p++;
			rgb[i].green = *p++;
			rgb[i].blue = *p++;
			rgb[i].alpha = 0;
		}
		*rgb_max = n;

		return bitmap_import(mng->frame_width, mng->frame_height, 8, mng->dat_line, current_ptr, 0);
	} else if (mng->pixel == 3) {
		*rgb_max = 0;

		return bitmap_import(mng->frame_width, mng->frame_height, 24, mng->dat_line, current_ptr, 0);
	} else if (mng->pixel == 4) {
		*rgb_max = 0;

		return bitmap_import(mng->frame_width, mng->frame_height, 32, mng->dat_line, current_ptr, 0);
	}

	return 0;
}

struct bitmap* mng_load(void* void_mng, FZ* f, video_color* rgb, unsigned* rgb_max, double* delay) {
	struct mng_context* mng = (struct mng_context*)void_mng;
	unsigned type;
	unsigned char* data;
	unsigned size;

	if (mng->end_flag)
		return 0;

	*delay = mng->frame_tick / (double)mng->frame_frequency;

	while (1) {
		if (png_read_chunk(f, &data, &size, &type) != 0)
			goto err;

		switch (type) {
			case MNG_CN_DEFI :
				if (mng_load_defi(mng,data,size) != 0)
					goto err_data;
				free(data);
				break;
			case MNG_CN_MOVE :
				if (mng_load_move(mng,f,data,size) != 0)
					goto err_data;
				free(data);
				break;
			case PNG_CN_IHDR :
				if (mng_load_png(mng,f,data,size) != 0)
					goto err_data;
				free(data);
				return mng_import(mng, rgb, rgb_max);
			case MNG_CN_DHDR :
				if (mng_load_delta(mng, f,data,size) != 0)
					goto err_data;
				free(data);
				return mng_import(mng, rgb, rgb_max);
			case MNG_CN_MEND :
				mng->end_flag = 1;
				free(data);
				return 0; /* end */
			case MNG_CN_FRAM :
				if (size > 1) {
					unsigned i = 1;
					while (i < size && data[i])
						++i;
					if (size >= i+9) {
						unsigned tick = read_be_u32(data + i+5);
						if (tick < 1)
							tick = 1;
						if (data[i+1] == 1 || data[i+1] == 2)
							*delay = tick / (double)mng->frame_frequency;
						if (data[i+1] == 2)
							mng->frame_tick = tick;
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
					log_std(("mng: unsupported critical chunk 0x%08x\n", type));
					goto err_data;
				}
		}
	}

err_data:
	free(data);
err:
	return 0;
}

void* mng_init(FZ* f) {
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

	if (mng_read_signature(f) != 0) {
		log_std(("mng: invalid signature\n"));
		goto err_mng;
	}

	if (png_read_chunk(f, &data, &size, &type) != 0)
		goto err_mng;

	if (type != MNG_CN_MHDR) {
		log_std(("mng: missing MHDR chunk\n"));
		goto err_data;
	}

	if (size != 28) {
		log_std(("mng: invalid MHDR size\n"));
		goto err_data;
	}

	mng->frame_width = read_be_u32(data + 0);
	mng->frame_height = read_be_u32(data + 4);
	mng->frame_frequency = read_be_u32(data + 8);
	if (mng->frame_frequency < 1)
		mng->frame_frequency = 1;
	mng->frame_tick = 1;
	simplicity = read_be_u32(data + 24);

	if (simplicity != 0x41 /* MNG-VLC without transparency */
		&& simplicity != 0x43 /* MNG-VLC without transparency + FRAME */
		&& simplicity != 0x267 /* generated by zmng */) {
		log_std(("mng: unsupported simplicity %d, continue anyway\n", simplicity));
	}

	free(data);

	return mng;

err_data:
	free(data);
err_mng:
	free(mng);
err:
	return 0;
}

void mng_done(void* void_mng) {
	struct mng_context* mng = (struct mng_context*)void_mng;

	free(mng->dat_ptr);
	free(mng->dlt_ptr);
	free(mng);
}


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

#include "png.h"

#include "os.h"

#include <stdlib.h>
#include <string.h>

/**************************************************************************************/
/* PNG */

#define PNG_CN_IHDR 0x49484452
#define PNG_CN_PLTE 0x504C5445
#define PNG_CN_IDAT 0x49444154
#define PNG_CN_IEND 0x49454E44

static unsigned char PNG_Signature[] = "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A";

static unsigned read_be_u32(unsigned char* v) {
	return v[0] << 24 | v[1] << 16 | v[2] << 8 | v[3];
}

int png_read_chunk(FZ* f, unsigned char** data, unsigned* size, unsigned* type) {
	unsigned char cl[4];
	unsigned char ct[4];
	unsigned char cc[4];

	if (fzread(cl, 4, 1, f) != 1) {
		os_log(("png: io error\n"));
		goto err;
	}

	*size = read_be_u32(cl);

	if (fzread(ct, 4, 1, f) != 1) {
		os_log(("png: io error\n"));
		goto err;
	}

	*type = read_be_u32(ct);

	if (*size) {
		*data = malloc(*size);
		if (!*data) {
			os_log(("png: low memory\n"));
			goto err;
		}

		if (fzread(*data, *size, 1, f) != 1) {
			os_log(("png: io error\n"));
			goto err_data;
		}
	} else {
		*data = 0;
	}

	if (fzread(cc, 4, 1, f) != 1) {
		os_log(("png: io error\n"));
		goto err_data;
	}

	return 0;
err_data:
	free(*data);
err:
	return -1;
}

void png_expand_4(unsigned width, unsigned height, unsigned char* ptr) {
	unsigned i,j;
	unsigned char* p8 = ptr + height * (width + 1) - 1;
	unsigned char* p4 = ptr + height * (width / 2 + 1) - 1;

	width /= 2;
	for(i=0;i<height;++i) {
		for(j=0;j<width;++j) {
			unsigned char v = *p4;
			*p8-- = v & 0xF;
			*p8-- = v >> 4;
			--p4;
		}
		--p8;
		--p4;
	}
}

void png_expand_2(unsigned width, unsigned height, unsigned char* ptr) {
	unsigned i,j;
	unsigned char* p8 = ptr + height * (width + 1) - 1;
	unsigned char* p2 = ptr + height * (width / 4 + 1) - 1;

	width /= 4;
	for(i=0;i<height;++i) {
		for(j=0;j<width;++j) {
			unsigned char v = *p2;
			*p8-- = v & 0x3;
			*p8-- = (v >> 2) & 0x3;
			*p8-- = (v >> 4) & 0x3;
			*p8-- = v >> 6;
			--p2;
		}
		--p8;
		--p2;
	}
}

void png_expand_1(unsigned width, unsigned height, unsigned char* ptr) {
	unsigned i,j;
	unsigned char* p8 = ptr + height * (width + 1) - 1;
	unsigned char* p1 = ptr + height * (width / 8 + 1) - 1;

	width /= 8;
	for(i=0;i<height;++i) {
		for(j=0;j<width;++j) {
			unsigned char v = *p1;
			*p8-- = v & 0x1;
			*p8-- = (v >> 1) & 0x1;
			*p8-- = (v >> 2) & 0x1;
			*p8-- = (v >> 3) & 0x1;
			*p8-- = (v >> 4) & 0x1;
			*p8-- = (v >> 5) & 0x1;
			*p8-- = (v >> 6) & 0x1;
			*p8-- = v >> 7;
			--p1;
		}
		--p8;
		--p1;
	}
}

void png_unfilter_8(unsigned width, unsigned height, unsigned char* p, unsigned line) {
	unsigned i,j;

	for(i=0;i<height;++i) {
		unsigned char f = *p++;

		if (f == 0) { /* none */
			p += width;
		} else if (f == 1) { /* sub */
			++p;
			for(j=1;j<width;++j) {
				p[0] += p[-1];
				++p;
			}
		} else if (f == 2) { /* up */
			if (i) {
				unsigned char* u = p - line;
				for(j=0;j<width;++j) {
					*p += *u;
					++p;
					++u;
				}
			} else {
				p += width;
			}
		} else if (f == 3) { /* average */
			if (i) {
				unsigned char* u = p - line;
				p[0] += u[0] / 2;
				++p;
				++u;
				for(j=1;j<width;++j) {
					unsigned a = (unsigned)u[0] + (unsigned)p[-1];
					p[0] += a >> 1;
					++p;
					++u;
				}
			} else {
				++p;
				for(j=1;j<width;++j) {
					p[0] += p[-1] / 2;
					++p;
				}
			}
		} else if (f == 4) { /* paeth */
			unsigned char* u = p - line;
			for(j=0;j<width;++j) {
				unsigned a,b,c;
				int v;
				int da, db, dc;
				a = j<1 ? 0 : p[-1];
				b = i<1 ? 0 : u[0];
				c = (j<1 || i<1) ? 0 : u[-1];
				v = a + b - c;
				da = v - a;
				if (da < 0)
					da = -da;
				db = v - b;
				if (db < 0)
					db = -db;
				dc = v - c;
				if (dc < 0)
					dc = -dc;
				if (da <= db && da <= dc)
					p[0] += a;
				else if (db <= dc)
					p[0] += b;
				else
					p[0] += c;
				++p;
				++u;
			}
		}

		p += line - width - 1;
	}
}

void png_unfilter_24(unsigned width, unsigned height, unsigned char* p, unsigned line) {
	unsigned i,j;

	for(i=0;i<height;++i) {
		unsigned char f = *p++;

		if (f == 0) { /* none */
			p += width;
		} else if (f == 1) { /* sub */
			p += 3;
			for(j=3;j<width;++j) {
				p[0] += p[-3];
				++p;
			}
		} else if (f == 2) { /* up */
			if (i) {
				unsigned char* u = p - line;
				for(j=0;j<width;++j) {
					*p += *u;
					++p;
					++u;
				}
			} else {
				p += width;
			}
		} else if (f == 3) { /* average */
			if (i) {
				unsigned char* u = p - line;
				p[0] += u[0] / 2;
				p[1] += u[1] / 2;
				p[2] += u[2] / 2;
				p += 3;
				u += 3;
				for(j=3;j<width;++j) {
					unsigned a = (unsigned)u[0] + (unsigned)p[-3];
					p[0] += a >> 1;
					++p;
					++u;
				}
			} else {
				p += 3;
				for(j=3;j<width;++j) {
					p[0] += p[-3] / 2;
					++p;
				}
			}
		} else if (f == 4) { /* paeth */
			unsigned char* u = p - line;
			for(j=0;j<width;++j) {
				unsigned a,b,c;
				int v;
				int da, db, dc;
				a = j<3 ? 0 : p[-3];
				b = i<1 ? 0 : u[0];
				c = (j<3 || i<1) ? 0 : u[-3];
				v = a + b - c;
				da = v - a;
				if (da < 0)
					da = -da;
				db = v - b;
				if (db < 0)
					db = -db;
				dc = v - c;
				if (dc < 0)
					dc = -dc;
				if (da <= db && da <= dc)
					p[0] += a;
				else if (db <= dc)
					p[0] += b;
				else
					p[0] += c;
				++p;
				++u;
			}
		}

		p += line - width - 1;
	}
}

void png_unfilter_32(unsigned width, unsigned height, unsigned char* p, unsigned line) {
	unsigned i,j;

	for(i=0;i<height;++i) {
		unsigned char f = *p++;

		if (f == 0) { /* none */
			p += width;
		} else if (f == 1) { /* sub */
			p += 4;
			for(j=4;j<width;++j) {
				p[0] += p[-4];
				++p;
			}
		} else if (f == 2) { /* up */
			if (i) {
				unsigned char* u = p - line;
				for(j=0;j<width;++j) {
					*p += *u;
					++p;
					++u;
				}
			} else {
				p += width;
			}
		} else if (f == 3) { /* average */
			if (i) {
				unsigned char* u = p - line;
				p[0] += u[0] / 2;
				p[1] += u[1] / 2;
				p[2] += u[2] / 2;
				p[3] += u[3] / 2;
				p += 4;
				u += 4;
				for(j=4;j<width;++j) {
					unsigned a = (unsigned)u[0] + (unsigned)p[-4];
					p[0] += a >> 1;
					++p;
					++u;
				}
			} else {
				p += 4;
				for(j=4;j<width;++j) {
					p[0] += p[-4] / 2;
					++p;
				}
			}
		} else if (f == 4) { /* paeth */
			unsigned char* u = p - line;
			for(j=0;j<width;++j) {
				unsigned a,b,c;
				int v;
				int da, db, dc;
				a = j<4 ? 0 : p[-4];
				b = i<1 ? 0 : u[0];
				c = (j<4 || i<1) ? 0 : u[-4];
				v = a + b - c;
				da = v - a;
				if (da < 0)
					da = -da;
				db = v - b;
				if (db < 0)
					db = -db;
				dc = v - c;
				if (dc < 0)
					dc = -dc;
				if (da <= db && da <= dc)
					p[0] += a;
				else if (db <= dc)
					p[0] += b;
				else
					p[0] += c;
				++p;
				++u;
			}
		}

		p += line - width - 1;
	}
}

static int png_read_signature(FZ* f) {
	unsigned char signature[8];

	if (fzread(signature,8,1,f) != 1) {
		return -1;
	}

	if (memcmp(signature,PNG_Signature,8)!=0) {
		return -1;
	}

	return 0;
}

struct bitmap* png_load(FZ* f, video_color* rgb, unsigned* rgb_max) {
	unsigned type;
	unsigned char* data;
	unsigned size;
	unsigned char* dat_ptr;
	unsigned dat_size;
	unsigned long res_size;
	unsigned bytes_per_pixel;
	unsigned width;
	unsigned width_align;
	unsigned height;
	unsigned depth;
	unsigned i;
	int r;
	z_stream z;

	if (png_read_signature(f) != 0)
		goto err;

	if (png_read_chunk(f, &data, &size, &type) != 0)
		goto err;

	if (size != 13)
		goto err_data;

	width = read_be_u32(data + 0);
	height = read_be_u32(data + 4);

	depth = data[8];
	if (data[9] == 3 && depth == 8) {
		bytes_per_pixel = 1;
		width_align = width;
	} else if (data[9] == 3 && depth == 4) {
		bytes_per_pixel = 1;
		width_align = (width + 1) & ~1;
	} else if (data[9] == 3 && depth == 2) {
		bytes_per_pixel = 1;
		width_align = (width + 3) & ~3;
	} else if (data[9] == 3 && depth == 1) {
		bytes_per_pixel = 1;
		width_align = (width + 7) & ~7;
	} else if (data[9] == 2 && depth == 8) {
		bytes_per_pixel = 3;
		width_align = width;
	} else
		goto err_data;
	if (data[10] != 0) /* compression */
		goto err_data;
	if (data[11] != 0) /* filter */
		goto err_data;
	if (data[12] != 0) /* interlace */
		goto err_data;

	free(data);

	if (png_read_chunk(f, &data, &size, &type) != 0)
		goto err;

	while (type != PNG_CN_PLTE && type != PNG_CN_IDAT) {
		free(data);

		if (png_read_chunk(f, &data, &size, &type) != 0)
			goto err;
	}

	if (type == PNG_CN_PLTE) {
		unsigned char* p;
		unsigned n;

		if (bytes_per_pixel != 1)
			goto err_data;

		if (size > 256*3)
			goto err_data;

		p = data;
		n = size / 3;
		for(i=0;i<n;++i) {
			rgb[i].red = *p++;
			rgb[i].green = *p++;
			rgb[i].blue = *p++;
			rgb[i].alpha = 0;
		}
		*rgb_max = n;

		free(data);

		if (png_read_chunk(f, &data, &size, &type) != 0)
			goto err;
	} else {
		if (bytes_per_pixel != 3)
			goto err_data;

		*rgb_max = 0;
	}

	while (type != PNG_CN_IDAT) {
		free(data);

		if (png_read_chunk(f, &data, &size, &type) != 0)
			goto err;
	}

	dat_size = height * (width_align * bytes_per_pixel + 1);
	dat_ptr = malloc(dat_size);

	z.zalloc = 0;
	z.zfree = 0;
	z.next_out = dat_ptr;
	z.avail_out = dat_size;
	z.next_in = 0;
	z.avail_in = 0;

	r = inflateInit(&z);

	while (r == Z_OK && type == PNG_CN_IDAT) {
		z.next_in = data;
		z.avail_in = size;

		r = inflate(&z, Z_NO_FLUSH);

		free(data);

		if (png_read_chunk(f, &data, &size, &type) != 0) {
			inflateEnd(&z);
			free(dat_ptr);
			goto err;
		}
	}

	res_size = z.total_out;

	inflateEnd(&z);

	if (r != Z_STREAM_END)
		goto err_data_ptr;

	if (type == PNG_CN_IDAT)
		goto err_data_ptr;

	if (depth == 8) {
		if (res_size != dat_size)
			goto err_data_ptr;

		if (bytes_per_pixel == 1)
			png_unfilter_8(width * bytes_per_pixel, height, dat_ptr, width_align * bytes_per_pixel + 1);
		else
			png_unfilter_24(width * bytes_per_pixel, height, dat_ptr, width_align * bytes_per_pixel + 1);

	} else if (depth == 4) {
		if (res_size != height * (width_align / 2 + 1))
			goto err_data_ptr;

		png_unfilter_8(width_align / 2, height, dat_ptr, width_align / 2 + 1);

		png_expand_4(width_align, height, dat_ptr);
	} else if (depth == 2) {
		if (res_size != height * (width_align / 4 + 1))
			goto err_data_ptr;

		png_unfilter_8(width_align / 4, height, dat_ptr, width_align / 4 + 1);

		png_expand_2(width_align, height, dat_ptr);
	} else if (depth == 1) {
		if (res_size != height * (width_align / 8 + 1))
			goto err_data_ptr;

		png_unfilter_8(width_align / 8, height, dat_ptr, width_align / 8 + 1);

		png_expand_1(width_align, height, dat_ptr);
	}

	free(data);

	return bitmap_import(width, height, bytes_per_pixel * 8, width_align * bytes_per_pixel + 1, dat_ptr + 1, dat_ptr);

err_data_ptr:
	free(dat_ptr);
err_data:
	free(data);
err:
	return 0;
}



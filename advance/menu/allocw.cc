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

#include <assert.h>
#include <stdlib.h>
#include <pc.h>
#include <dos.h>

/**************************************************************************/
/* malloc/new wrapper */

#ifndef NDEBUG

#ifdef __cplusplus
extern "C" {
#endif

static unsigned malloc_counter = 1;
static unsigned malloc_total = 0;

static void fill(void* _p, unsigned size) {
	unsigned char* p = (unsigned char*)_p;
	unsigned i;

	for(i=0;i<size;++i)
		p[i] = malloc_counter ^ (i*i) ^ 0x4A;
}

typedef struct {
	unsigned wsize;
	unsigned counter;
} header;

typedef struct {
	unsigned counter;
} footer;

static void* wrap2real(void* _w) {
	unsigned char* w = (unsigned char*)_w;
	return w - sizeof(header);
}

static void* real2wrap(void* _r) {
	unsigned char* r = (unsigned char*)_r;
	return r + sizeof(header);
}

static header* wrap2header(void* w) {
	return (header*)wrap2real(w);
}

static footer* wrap2footer(void* _w, unsigned wsize) {
	unsigned char* w = (unsigned char*)_w;
	return (footer*)(w + wsize);
}

static unsigned wrapsize2realsize(unsigned wsize) {
	return wsize + sizeof(header) + sizeof(footer);
}

static void wrapcheck(void* w) {
	header* h = wrap2header(w);
	footer* f = wrap2footer(w,h->wsize);
	if (h->counter != f->counter) {
		sound(200);
		delay(1000);
		nosound();

		abort();
	}
}

extern void* __real_malloc(size_t size);

void* __wrap_malloc(size_t wsize) {
	unsigned rsize = wrapsize2realsize(wsize);
	void* r = __real_malloc(rsize);

	if (!r) {
		return 0;
	} else {
		void* w = real2wrap(r);
		header* h = wrap2header(w);
		footer* f = wrap2footer(w,wsize);
		h->wsize = wsize;
		h->counter = malloc_counter;
		f->counter = malloc_counter;
		wrapcheck(w);
		++malloc_counter;
		++malloc_total;
		return w;
	}
}

extern void __real_free(void* p);

void __wrap_free(void* w) {
	if (!w) {
		return;
	} else {
		void* r = wrap2real(w);
		unsigned rsize = wrapsize2realsize(wrap2header(w)->wsize);
		wrapcheck(w);
		fill(r,rsize);
		__real_free(r);
		--malloc_total;
	}
}

extern void* __real_realloc(void* p, size_t size);

void* __wrap_realloc(void* w, size_t wsize) {
	if (!w) {
		return __wrap_malloc(wsize);
	} else {
		void* r = wrap2real(w);
		unsigned wsizeold = wrap2header(w)->wsize;
		unsigned rsize = wrapsize2realsize(wsize);
		wrapcheck(w);
		r = __real_realloc(r,rsize);
		if (!r) {
			return 0;
		} else {
			header* h;
			footer* f;
			w = real2wrap(r);
			if (wsizeold < wsize)
				fill(((unsigned char*)w) + wsizeold, wsize - wsizeold);
			h = wrap2header(w);
			f = wrap2footer(w,wsize);
			h->wsize = wsize;
			h->counter = malloc_counter;
			f->counter = malloc_counter;
			wrapcheck(w);
			++malloc_counter;
			return w;
		}
	}
}

#ifdef __cplusplus
}

void* operator new(size_t size) /* throw(std::bad_alloc) */ {
	return __wrap_malloc(size);
}

void operator delete(void* p) /* throw() */ {
	__wrap_free(p);
}

void* operator new[](size_t size) /* throw(std::bad_alloc) */ {
	return __wrap_malloc(size);
}

void operator delete[](void* p) /* throw() */ {
	__wrap_free(p);
}

#endif

#endif

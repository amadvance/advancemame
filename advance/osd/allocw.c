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

#include "alloc.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**************************************************************************/
/* malloc wrapper */

struct malloc_header {
	struct malloc_header* pred;
	struct malloc_header* next;
	unsigned counter;
	unsigned size;
	unsigned mask;
};

struct malloc_footer {
	unsigned mask;
};

static struct malloc_header* base;

unsigned malloc_abort_counter;
unsigned malloc_start_counter;
unsigned malloc_last_check;
static unsigned malloc_counter;

static int malloc_check(void* _d) {
	unsigned char* d;
	struct malloc_header* h;
	struct malloc_footer* f;

	d = _d;
	h = (struct malloc_header*)(d - sizeof(struct malloc_header));

	malloc_last_check = h->counter;

	if (h->mask != (h->size ^ h->counter)) {
		return 0;
	}

	f = (struct malloc_footer*)(d + h->size);
	if (h->mask != f->mask)
		return 0;

	return 1;
}

void malloc_init(void) {
	malloc_start_counter = malloc_counter;
}

void malloc_done(void) {
	struct malloc_header* i;
	int bad;

	if (!base) {
		printf("Heap OK\n");
		return;
	}

	printf("Heap unallocated\n");
	i = base;
	bad = 0;
	do {
		if (i->counter > malloc_start_counter) {
			printf(" %d/%d", i->counter, i->size);
			if (!malloc_check( ((unsigned char*)i) + sizeof(struct malloc_header) ) ) {
				printf("/BAD");
				bad = 1;
			}
		}
		i = i->next;
	} while (i != base);
	printf("\nEnd, %d bad blocks\n",bad);
}

static void malloc_insert(struct malloc_header* h) {
	if (!base) {
		base = h;
		h->pred = base;
		h->next = base;
	} else {
		h->pred = base;
		h->next = base->next;
		base->next->pred = h;
		base->next = h;
	}
}

static void malloc_remove(struct malloc_header* h) {
	if (base == h) {
		base = base->next;
	}
	if (base == h) {
		base = 0;
	} else {
		h->pred->next = h->next;
		h->next->pred = h->pred;
	}
}

extern void* __real_malloc(size_t size);

void* __wrap_malloc(size_t size) {
	unsigned char* p;
	unsigned dsize;

	++malloc_counter;
	if (malloc_counter == malloc_abort_counter) {
		fprintf(stderr,"malloc[%d]: abort\n", malloc_counter);
		abort();
	}

	dsize = (size + 3) & ~3;

	p = (unsigned char*)__real_malloc(dsize + sizeof(struct malloc_header) + sizeof(struct malloc_footer));
	if (!p) {
		return 0;
	} else {
		struct malloc_header* h;
		unsigned char* d;
		struct malloc_footer* f;

		h = (struct malloc_header*)p;
		d = p + sizeof(struct malloc_header);
		f = (struct malloc_footer*)(d + dsize);

		memset(d,0x5A,dsize);

		h->counter = malloc_counter;
		h->size = dsize;
		h->mask = h->counter ^ h->size;
		f->mask = h->mask;

		malloc_insert(h);

		if (!malloc_check(d)) {
			fprintf(stderr,"malloc[%d]: bad alloc\n", malloc_counter);
			/* abort(); */
			return 0;
		}

		return d;
	}
}

extern void __real_free(void* p);

void __wrap_free(void* _d) {
	unsigned char* d = (unsigned char*)_d;

	++malloc_counter;
	if (malloc_counter == malloc_abort_counter) {
		fprintf(stderr,"free[%d]: abort\n", malloc_counter);
		abort();
	}

	if (!d) {
		return;
	} else {
		struct malloc_header* h;

		h = (struct malloc_header*)(d - sizeof(struct malloc_header));

		if (!malloc_check(d)) {
			fprintf(stderr,"free[%d]: bad block counter %d, size %d, mask %x, expected mask %x\n", malloc_counter, h->counter, h->size, h->mask, h->counter ^ h->size);
			/* abort(); */
			return;
		}

		malloc_remove(h);

		__real_free(h);
	}
}

extern void* __real_realloc(void* p, size_t size);

void* __wrap_realloc(void* _d, size_t size) {
	unsigned char* d = (unsigned char*)_d;

	++malloc_counter;
	if (malloc_counter == malloc_abort_counter) {
		fprintf(stderr,"realloc[%d]: abort\n", malloc_counter);
		abort();
	}

	if (!d) {
		return __wrap_malloc(size);
	} else {
		struct malloc_header* h;
		unsigned char* p;
		unsigned dsize;

		if (!malloc_check(d)) {
			fprintf(stderr,"realloc[%d]: bad block\n", malloc_counter);
			/* abort(); */
			return 0;
		}

		p = d - sizeof(struct malloc_header);
		h = (struct malloc_header*)p;

		malloc_remove(h);

		dsize = (size + 3) & ~3;

		p = (unsigned char*)__real_realloc( p, dsize + sizeof(struct malloc_header) + sizeof(struct malloc_footer));
		if (!p) {
			return 0;
		} else {
			struct malloc_footer* f;

			h = (struct malloc_header*)p;
			d = p + sizeof(struct malloc_header);
			f = (struct malloc_footer*)(d + dsize);

			h->counter = malloc_counter;
			h->size = dsize;
			h->mask = h->counter ^ h->size;
			f->mask = h->mask;

			malloc_insert(h);

			if (!malloc_check(d)) {
				fprintf(stderr,"realloc[%d]: bad alloc\n", malloc_counter);
				/* abort(); */
				return 0;
			}

			return d;
		}
	}
}

extern char* __real_strdup(char* s);

char* __wrap_strdup(char* s) {
	unsigned len;
	char* ptr;

	assert(s);

	len = strlen(s);
	ptr = malloc(len + 1);
	if (!ptr)
		return 0;

	memcpy(ptr,s,len+1);

	return ptr;
}

#ifdef __cplusplus

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



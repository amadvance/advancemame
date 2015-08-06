/****************************************************************************

	library.c

	Code relevant to the Imgtool library; analgous to the MESS/MAME driver
	list.

****************************************************************************/

#include <assert.h>
#include <string.h>

#include "osdepend.h"
#include "library.h"
#include "pool.h"

struct _imgtool_library
{
	memory_pool pool;
	struct ImageModule *first;
	struct ImageModule *last;
};



imgtool_library *imgtool_library_create(void)
{
	imgtool_library *library;

	library = malloc(sizeof(struct _imgtool_library));
	if (!library)
		return NULL;
	memset(library, 0, sizeof(*library));

	pool_init(&library->pool);
	return library;
}



void imgtool_library_close(imgtool_library *library)
{
	pool_exit(&library->pool);
	free(library);
}



const struct ImageModule *imgtool_library_unlink(imgtool_library *library,
	const char *module)
{
	struct ImageModule *m;
	struct ImageModule **previous;
	struct ImageModule **next;

	for (m = library->first; m; m = m->next)
	{
		if (!mame_stricmp(m->name, module))
		{
			previous = m->previous ? &m->previous->next : &library->first;
			next = m->next ? &m->next->previous : &library->last;
			*previous = m->next;
			*next = m->previous;
			m->previous = NULL;
			m->next = NULL;
			return m;
		}
	}
	return NULL;
}



static int module_compare(const struct ImageModule *m1,
	const struct ImageModule *m2, imgtool_libsort_t sort)
{
	int rc = 0;
	switch(sort)
	{
		case ITLS_NAME:
			rc = strcmp(m1->name, m2->name);
			break;
		case ITLS_DESCRIPTION:
			rc = mame_stricmp(m1->name, m2->name);
			break;
	}
	return rc;
}



void imgtool_library_sort(imgtool_library *library, imgtool_libsort_t sort)
{
	struct ImageModule *m1;
	struct ImageModule *m2;
	struct ImageModule *target;
	struct ImageModule **before;
	struct ImageModule **after;

	for (m1 = library->first; m1; m1 = m1->next)
	{
		target = m1;
		for (m2 = m1->next; m2; m2 = m2->next)
		{
			while(module_compare(target, m2, sort) > 0)
				target = m2;
		}

		if (target != m1)
		{
			/* unlink the target */
			before = target->previous ? &target->previous->next : &library->first;
			after = target->next ? &target->next->previous : &library->last;
			*before = target->next;
			*after = target->previous;

			/* now place the target before m1 */
			target->previous = m1->previous;
			target->next = m1;
			before = m1->previous ? &m1->previous->next : &library->first;
			*before = target;
			m1->previous = target;

			/* since we changed the order, we have to replace ourselves */
			m1 = target;
		}
	}
}



imgtoolerr_t imgtool_library_createmodule(imgtool_library *library,
	const char *module_name, struct ImageModule **module)
{
	struct ImageModule *newmodule;
	char *alloc_module_name;

	newmodule = pool_malloc(&library->pool, sizeof(struct ImageModule));
	if (!newmodule)
		goto outofmemory;

	alloc_module_name = pool_strdup(&library->pool, module_name);
	if (!alloc_module_name)
		goto outofmemory;

	memset(newmodule, 0, sizeof(*newmodule));
	newmodule->previous = library->last;
	newmodule->name = alloc_module_name;

	if (library->last)
		library->last->next = newmodule;
	else
		library->first = newmodule;
	library->last = newmodule;

	*module = newmodule;
	return IMGTOOLERR_SUCCESS;

outofmemory:
	*module = NULL;
	return IMGTOOLERR_OUTOFMEMORY;
}



const struct ImageModule *imgtool_library_findmodule(
	imgtool_library *library, const char *module_name)
{
	const struct ImageModule *module;

	assert(library);
	module = library->first;
	while(module && module_name && strcmp(module->name, module_name))
		module = module->next;
	return module;
}



struct ImageModule *imgtool_library_iterate(imgtool_library *library, const struct ImageModule *module)
{
	return module ? module->next : library->first;
}



struct ImageModule *imgtool_library_index(imgtool_library *library, int i)
{
	struct ImageModule *module;
	module = library->first;
	while(module && i--)
		module = module->next;
	return module;
}



void *imgtool_library_alloc(imgtool_library *library, size_t mem)
{
	return pool_malloc(&library->pool, mem);
}



char *imgtool_library_strdup(imgtool_library *library, const char *s)
{
	return s ? pool_strdup(&library->pool, s) : NULL;
}



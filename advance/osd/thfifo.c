/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003 Andrea Mazzoleni
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

/** \file
 * A pthread implementation of the osd_parallelize function.
 *
 * This implementation supports a N processor system and
 * reentrant calls.
 *
 * A set of companion threads is created at the startup like a
 * workpile implementation.
 */

#include "portable.h"

#include "thread.h"

#include <pthread.h>

/**
 * Number of extra threads for reentrant calls.
 * To support reentrant calls some extra threads are required.
 * It happen because for any reentrant call, the calling thread is
 * stopped until all the others threads terminate.
 */
#define THREAD_EXTRA 2

/** Group. */
struct group_t {
	pthread_cond_t isempty; /**< End condition. */
	pthread_mutex_t mutex; /**< Access mutex. */
	unsigned count; /**< Number of works in the group not completed. */
};

/** Work item. */
struct work_t {
	struct group_t* group; /**< Part of this group. */

	void (*func)(void*, int, int); /**< Function to call. */
	void* arg; /**< Argument of the function. */
	unsigned num; /**< Argument of the function. */
	unsigned max; /**< Argument of the function. */

	struct work_t* next; /**< Next element in the fifo. */
	struct work_t* pred; /**< Pred element in the fifo. */
};

static int thread_exit; /**< Thread exit requested. */
static pthread_mutex_t work_mutex; /**< Mutex for all the work fifo access. */
static pthread_cond_t work_notempty; /**< Condition of not empty work fifo. */
static struct work_t* work_fifo; /**< List of work items. */
static unsigned work_running; /**< Number of threads running. */
static unsigned work_ready; /**< Number of threads ready. */
static unsigned work_limit; /**< Max number of suggested threads running. */
static pthread_t* work_map; /**< Vector of thread id. */
static unsigned work_max; /**< Number of thread created. */

/** Push an element in the work fifo. */
static void work_fifo_push(struct work_t* work)
{
	if (work_fifo == 0) {
		work_fifo = work;
		work_fifo->next = work_fifo;
		work_fifo->pred = work_fifo;
	} else {
		work->next = work_fifo;
		work->pred = work_fifo->pred;
		work_fifo->pred->next = work;
		work_fifo->pred = work;
		work_fifo = work;
	}
}

/** Check if the work fifo is empty. */
static int work_fifo_isempty(void)
{
	return work_fifo == 0;
}

/** Pop an element from the work fifo. */
static struct work_t* work_fifo_pop(void)
{
	struct work_t* work;
	if (work_fifo == work_fifo->next) {
		work = work_fifo;
		work_fifo = 0;
	} else {
		work = work_fifo->pred;
		work_fifo->pred = work->pred;
		work_fifo->pred->next = work_fifo;
	}
	return work;
}

/** Main thread function. */
static void* group_func(void* arg)
{
	pthread_mutex_lock(&work_mutex);

	while (1) {
		struct work_t* work;

		/* wait until a work item is available */
		--work_running;
		++work_ready;
		while (work_fifo_isempty() && !thread_exit) {
			pthread_cond_wait(&work_notempty, &work_mutex);
		}
		--work_ready;
		++work_running;

		if (thread_exit) {
			pthread_mutex_unlock(&work_mutex);
			pthread_exit(0);
		}

		/* remove a work item from the fifo */
		work = work_fifo_pop();

		pthread_mutex_unlock(&work_mutex);

		/* call the function */
		work->func(work->arg, work->num, work->max);

		/* signal the stop condition */
		pthread_mutex_lock(&work->group->mutex);
		if (--work->group->count == 0)
			pthread_cond_signal(&work->group->isempty);
		pthread_mutex_unlock(&work->group->mutex);

		pthread_mutex_lock(&work_mutex);
	}
}

/** Return the number of threads to start for an optimal load. */
unsigned work_free_get(void)
{
	unsigned running;
	unsigned ready;

	pthread_mutex_lock(&work_mutex);
	running = work_running;
	ready = work_ready;
	pthread_mutex_unlock(&work_mutex);

	if (running >= work_limit)
		return 1;

	if (ready + running > work_limit)
		ready = work_limit - running;

	return ready + 1;
}

/** Initialize a group of work items. */
void group_init(struct group_t* group)
{
	pthread_mutex_init(&group->mutex, NULL);
	pthread_cond_init(&group->isempty, NULL);
	group->count = 0;
}

/** Destroy a group of work items. */
void group_destroy(struct group_t* group)
{
	pthread_mutex_destroy(&group->mutex);
	pthread_cond_destroy(&group->isempty);
}

/** Insert a work item in the group and start it. */
void group_put(struct group_t* group, struct work_t* work)
{
	pthread_mutex_lock(&group->mutex);
	++group->count;
	pthread_mutex_unlock(&group->mutex);

	pthread_mutex_lock(&work_mutex);
	work->group = group;
	work_fifo_push(work);
	pthread_cond_signal(&work_notempty);
	pthread_mutex_unlock(&work_mutex);
}

/** Wait the conclusion of all the work items of the group. */
void group_wait(struct group_t* group)
{
	/* we will probably wait */
	pthread_mutex_lock(&work_mutex);
	--work_running;
	pthread_mutex_unlock(&work_mutex);

	/* waits until the the work items are done */
	pthread_mutex_lock(&group->mutex);
	while (group->count != 0)
		pthread_cond_wait(&group->isempty, &group->mutex);
	pthread_mutex_unlock(&group->mutex);

	pthread_mutex_lock(&work_mutex);
	++work_running;
	pthread_mutex_unlock(&work_mutex);
}

int thread_init(void)
{
	unsigned i;

	thread_exit = 0;

	work_fifo = 0;
	work_limit = sysconf(_SC_NPROCESSORS_ONLN) + 1;
	work_max = work_limit + THREAD_EXTRA;
	work_running = 1 + work_max;
	work_ready = 0;

	work_map = (pthread_t*)malloc(work_max * sizeof(pthread_t));
	if (!work_map)
		return -1;

	if (pthread_mutex_init(&work_mutex, NULL) != 0)
		return -1;
	if (pthread_cond_init(&work_notempty, NULL) != 0)
		return -1;

	for(i=0;i<work_max;++i) {
		if (pthread_create(&work_map[i], NULL, group_func, 0) != 0)
			return -1;
	}

	return 0;
}

void thread_done(void)
{
	unsigned i;

	thread_exit = 1;

	pthread_mutex_lock(&work_mutex);
	pthread_cond_broadcast(&work_notempty);
	pthread_mutex_unlock(&work_mutex);

	for(i=0;i<work_max;++i)
		pthread_join(work_map[i], NULL);

	pthread_mutex_destroy(&work_mutex);
	pthread_cond_destroy(&work_notempty);

	free(work_map);
}

/** Max number of thread for osd_parallelize(). */
#define THREAD_MAX 16

void osd_parallelize(void (*func)(void* arg, int num, int max), void* arg, int max)
{
	struct work_t work[THREAD_MAX];
	struct group_t group;
	unsigned limit;
	unsigned i;

	if (!thread_is_active()) {
		func(arg, 0, 1);
		return;
	}

	/* get the number of suggested work items */
	limit = work_free_get();

	/* limit the number of threads */
	if (max > THREAD_MAX)
		max = THREAD_MAX;
	if (max > limit)
		max = limit;

	if (max <= 1) {
		func(arg, 0, 1);
		return;
	}

	group_init(&group);

	for(i=1;i<max;++i) {
		work[i].func = func;
		work[i].arg = arg;
		work[i].num = i;
		work[i].max = max;
		group_put(&group, &work[i]);
	}

	func(arg, 0, max);

	group_wait(&group);

	group_destroy(&group);
}


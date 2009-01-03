
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

#include "portable.h"

#include "thread.h"

/** \file
 * A pthread implementation of the osd_parallelize function.
 *
 * This implementation is optimized for a biprocessor system.
 * It doesn't support reentrant calls.
 *
 * A companion thread is created at the startup. This thread is used
 * to run an additional function when requested. 
 * This approach is faster than creating a new thread every time. 
 * For example on my system (Dual Pentium II 350 - Linux 2.4.19) calling 
 * 100000 osd_parallelize with pthread_create takes 20 sec, with this approach 
 * only 4.5 sec.
 */

#include <pthread.h>

static int thread_exit; /**< Thread exit requested. */
static pthread_t thread_id; /**< ID of the companion thread. */
static int thread_inuse; /**< Reentrant check. */
static pthread_cond_t thread_cond; /**< Start/Stop condition. */
static pthread_mutex_t thread_mutex; /**< Access mutex. */
static void (*thread_func)(void*, int, int); /**< Function to call. */
static void* thread_arg; /**< Argument of the function to call. */

static void* thread_proc(void* arg) 
{
	pthread_mutex_lock(&thread_mutex);

	while (1) {
		void (*func)(void*, int, int);

		/* wait for the start signal */
		while (!thread_func && !thread_exit)
			pthread_cond_wait(&thread_cond, &thread_mutex);

		if (thread_exit) {
			pthread_mutex_unlock(&thread_mutex);
			break;
		}

		func = thread_func;
		pthread_mutex_unlock(&thread_mutex);

		/* call the secondary function */
		func(thread_arg, 1, 2);

		/* signal the end */
		pthread_mutex_lock(&thread_mutex);

		thread_func = 0;
		pthread_cond_signal(&thread_cond);
	}

	pthread_exit(0);
	return 0;
}


/** Initialize the thread support. */
int thread_init(void)
{
	thread_exit = 0;

	if (pthread_mutex_init(&thread_mutex, NULL) != 0)
		return -1;
	if (pthread_cond_init(&thread_cond, NULL) != 0)
		return -1;
	if (pthread_create(&thread_id, NULL, thread_proc, 0) != 0)
		return -1;

	return 0;
}

/** Deinitialize the thread system. */
void thread_done(void)
{
	pthread_mutex_lock(&thread_mutex);
	thread_exit = 1;
	pthread_cond_signal(&thread_cond);
	pthread_mutex_unlock(&thread_mutex);

	pthread_join(thread_id, NULL);

	pthread_mutex_destroy(&thread_mutex);
	pthread_cond_destroy(&thread_cond);
}

void osd_parallelize(void (*func)(void* arg, int num, int max), void* arg, int max) 
{
	if (!thread_is_active()) {
		func(arg, 0, 1);
		return;
	}

	if (max <= 1) {
		func(arg, 0, 1);
		return;
	}

	if (thread_inuse) {
		func(arg, 0, 1);
		return;
	}

	thread_inuse = 1;

	pthread_mutex_lock(&thread_mutex);
	thread_func = func;
	thread_arg = arg;
	pthread_cond_signal(&thread_cond);
	pthread_mutex_unlock(&thread_mutex);
  
	/* call the primary function */
	func(arg, 0, 2);

	pthread_mutex_lock(&thread_mutex);
	while (thread_func)
		pthread_cond_wait(&thread_cond, &thread_mutex);
	pthread_mutex_unlock(&thread_mutex);

	thread_inuse = 0;
}



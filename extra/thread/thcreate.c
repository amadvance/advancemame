#include <pthread.h>

struct thread_helper_data {
	void (*func)(void* arg, int num, int max);
	void* arg;
};

static void* thread_helper(void* arg) {
	struct thread_helper_data* p = (struct thread_helper_data*)arg;
	p->func(p->arg,1,2);
	return 0;
}

void osd_parallelize(void (*func)(void* arg, int num, int max), void* arg, int max)
{
	pthread_t thread_id;
	struct thread_helper_data thread_data;

	thread_data.func = func;
	thread_data.arg = arg;

	if (max <= 1) {
		func(arg,0,1);
		return;
	}

	if (pthread_create(&thread_id, NULL, thread_helper, &thread_data) == 0) {
		func(arg,0,2);
		pthread_join(thread_id, NULL);
	} else {
		func(arg,0,1);
	}
}

void thread_init(void) {
}

void thread_done(void) {
}

#include "test.h"

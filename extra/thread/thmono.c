void osd_parallelize(void (*func)(void* arg, int num, int max), void* arg, int max)
{
	func(arg,0,1);
}

void thread_init(void) {
}

void thread_done(void) {
}

#include "test.h"

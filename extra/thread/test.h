/********************************************************************************/
/* test */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

#if 0
#define PERIOD (1 << 27)
#define SIZE (1 << 16)
#define COUNT (PERIOD / SIZE)
#else
#define SIZE (1 << 16)
#define COUNT (1 << 13)
#endif

struct arg1 {
	unsigned char buf0[SIZE];
	unsigned char buf1[SIZE];
	unsigned char buf2[SIZE];
	unsigned char buf3[SIZE];
};

void set(unsigned char* buf0) {
	unsigned i;
	for(i=0;i<SIZE;++i)
		buf0[i] = rand();
}

void mask(unsigned char* buf0, unsigned char* buf1) {
	unsigned i;
#if 1
	unsigned size = (SIZE / 2) + rand() % (SIZE / 2);
#else
	unsigned size = SIZE;
#endif
	for(i=0;i<size;++i)
		if (buf0[i] != 0)
			buf1[i] = buf0[i];
}

void process1(void* _arg, int num, int max) {
	struct arg1* arg = _arg;
	if (max == 1) {
		mask(arg->buf0, arg->buf1);
		mask(arg->buf2, arg->buf3);
	} else {
		if (num == 0) {
			mask(arg->buf0, arg->buf1);
		} else {
			mask(arg->buf2, arg->buf3);
		}
	}
}

struct arg2 {
	struct arg1 p0;
	struct arg1 p1;
};

void process2(void* _arg, int num, int max) {
	struct arg2* arg = _arg;
	if (max == 1) {
		osd_parallelize(process1, &arg->p0, 2);
		osd_parallelize(process1, &arg->p1, 2);
	} else {
		if (num == 0) {
			osd_parallelize(process1, &arg->p0, 2);
		} else {
			osd_parallelize(process1, &arg->p1, 2);
		}
	}
}

#define TICKS_PER_SEC 1000000LL

long long tick(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000LL + tv.tv_usec;
}

int main() {
	struct arg2 p;
	long long start;
	long long stop;
	unsigned i;
	double period;

	srand(0);
	set(p.p0.buf0);
	set(p.p0.buf1);
	set(p.p0.buf2);
	set(p.p0.buf3);
	set(p.p1.buf0);
	set(p.p1.buf1);
	set(p.p1.buf2);
	set(p.p1.buf3);

	printf("size %d\n", SIZE);
	printf("count %d\n", COUNT);

	thread_init();
	sched_yield();

	start = tick();
	for(i=0;i<COUNT;++i)
		osd_parallelize(process2,&p,2);
	stop = tick();

	thread_done();

	period = (double)(stop - start) / TICKS_PER_SEC;

	printf("%g [sec]\n", period );
	printf("%g [thread/sec]\n", COUNT * 4 / period );
	printf("%g [thread/frame]\n", COUNT * 4 / (period*60) );

	return 0;
}

/* Run-length slice test program */

#include <stdio.h>
#include <stdlib.h>

struct run_slice {
	unsigned whole;
	int up;
	int down;
	int error_t;
	unsigned count;
};

void run_slice_init(struct run_slice* r, int S, int D)
{
	if (S > D) {
		/* reduction */
		--S;
		--D;
		r->whole = S / D;
		r->up = (S % D) * 2;
		r->down = D * 2;
		r->error_t = 0;
		r->count = D + 1;
	} else {
		/* expansion */
		r->whole = D / S;
		r->up = (D % S) * 2;
		r->down = S * 2;
		r->error_t = 0;
		r->count = S;
	}
}

void test_red(int S, int D)
{
	int total = 0;
	int step = 0;
	struct run_slice r;
	int error_t;
	int count;

	if (S <= D)
		return;

	run_slice_init(&r, S, D);

	error_t = r.error_t;
	count = r.count;

	while (count) {
		unsigned run = r.whole;

		if ((error_t += r.up) > 0) {
			++run;
			error_t -= r.down;
		}
		if (count == 1)
			total += 1;
		else
			total += run;
		++step;
		--count;
	}

	if (total != S || step != D) {
		printf("error: tot %d, stp %d, src %d, dst %d\n", total, step, S, D);
		exit(1);
	}
}

void test_exp(int S, int D)
{
	int total = 0;
	int step = 0;
	struct run_slice r;
	int error_t;
	int count;

	if (S >= D)
		return;

	run_slice_init(&r, S, D);

	error_t = r.error_t;
	count = r.count;

	while (count) {
		unsigned run = r.whole;

		if ((error_t += r.up) > 0) {
			++run;
			error_t -= r.down;
		}
		total += run;
		++step;
		--count;
	}

	if (total != D || step != S) {
		printf("error: tot %d, stp %d, src %d, dst %d\n", total, step, S, D);
		exit(1);
	}
}

int main()
{
	int i, j;

	printf("Testing...\n");

	for(i=2;i<3000;++i) {
		for(j=2;j<3000;++j) {
			if (i > j)
				test_red(i, j);
			if (i < j)
				test_exp(i, j);
		}
	}

	printf("Ok\n");

	return 0;
}


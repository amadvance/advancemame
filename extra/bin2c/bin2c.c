#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	int c;
	int n;
	int s;
	FILE* f;

	f = fopen(argv[1], "rb");
	if (!f) {
		exit(EXIT_FAILURE);
	}

	n = 0;
	s = 0;
	c = fgetc(f);
	printf("unsigned char DATA[] = {\n\t");

	while (c!=EOF) {
		++s;
		printf("0x%02x",(unsigned)c);
		c = fgetc(f);

		if (c == EOF) {
			printf("\n};\n\n");
		} else {
			if (++n == 8) {
				printf(",\n\t");
				n = 0;
			} else {
				printf(", ");
			}
		}
	}

	printf("#define DATA_SIZE %d\n",s);

	return EXIT_SUCCESS;
}


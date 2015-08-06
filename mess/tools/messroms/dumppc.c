#include <stdio.h>
#include <dos.h>

/*
simple dos utility to dump the
the rom area of pc's

(may have troubles in virtuel 8086
of the 80386 processors)

memory modell large required
*/

/* if you are familiar with
debug or other such utilities
its better to use the utility */

char buffer[0x4000];

int main(void)
{
	char far *dest;
	unsigned long seg;
	int i;
	FILE *file;

	file=fopen("dump.bin","wb");

	for (seg=0xc000; seg<0x10000; seg+=0x400) {

		dest=MK_FP(seg,0);

		for (i=0;i<0x4000;i++) buffer[i]=dest[i];

		fwrite(buffer,1,0x4000,file);
	}

	fclose(file);
	return 0;
}

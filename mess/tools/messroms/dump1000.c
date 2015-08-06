#include <stdio.h>
#include <dos.h>
#include <mem.h>

/*
simple dos utility to dump the
bankswitched roms of several
tandy modells (1000rl, 1000rlx, 2500xl, ...)

usage of dumppc is still recommended, to dump
ega/vga bios roms, network card bootroms, ...
usage of dumpat might be usefull

memory modell large required
*/

/*
  dma hardware might/will have problems with
  rom chips
  graphics card memory

  --> copy it always to normal ram, before fwrite
 */
char buffer[0x4000];

int main(void)
{
	char far *source;
	unsigned long adr, adr2;
	int oldpage, page, i, j;
	FILE *file;
	union REGS regs, oregs;

	regs.x.ax=0x7002;
	int86(0x15, &regs, &oregs);
	oldpage=oregs.h.al;
	if (oregs.x.cflag) printf("doesn't support int 15 ax=0x7002\n");

	file=fopen("dump1000.bin","wb");

	/* 0x12 gives (0x12+1)*65536 big file
	   small enough to fit on a 1.2mb disk
	   large enough to hold big roms
	   banks 0x10, 0x11 show if rom is larger than 1 megabyte */
	for (page=0; page<0x12; page++) {

		regs.x.ax=0x7003;
		regs.h.dl=page;
		int86(0x15, &regs, &oregs);
		if (oregs.x.cflag) break;

		for (i=0;i<0x100;i+=0x40) {


			source=MK_FP(0xe000,(i<<8));
			for (j=0;j<0x4000;j++) buffer[j]=source[j];

			fwrite(buffer,1,0x4000,file);
		}
	}

	regs.x.ax=0x7003;
	regs.h.dl=oldpage;
	int86(0x15, &regs, &oregs);
	if (oregs.x.cflag)
		printf("doesn't support int 15 ax=0x7003 dl=%.2x\n",oldpage);

	for (i=0;i<0x100;i+=0x40) {
		source=MK_FP(0xf000,(i<<8));
		for (j=0;j<0x4000;j++) buffer[j]=source[j];
		fwrite(buffer,1,0x4000,file);
	}

	fclose(file);
	return 0;
}

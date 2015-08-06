#include <stdio.h>
#include <dos.h>
#include <mem.h>

/*
simple dos utility to dump the
the extended/protected rom area of 286 or 386 based ibm pc at compatibles
(286 0xf00000-0xffffff
 386 0xfff00000-0xffffffff)
might work on 286 based pure pc's (not at's)

Some at's, tandy 1000's contain additional bios software at the
protected mode addresses, not visible or accessible to normal dos.

usage of dumppc is still recommended, to dump
ega/vga bios roms, network card bootroms, ...

(may have troubles in virtuel 8086
of the 80386 processors)

memory modell large required
*/

/* if you are familiar with
debug or other such utilities
its better to use the utility */

char buffer[0x4000]= { 0 };

struct _TABLE {
	char res[0x10];
	struct {
		unsigned short limit;
		unsigned short address_low;
		unsigned char address_high;
		unsigned char rights;
		unsigned char rights386;
		unsigned char address_high386;
	} src, dest;
	char res2[0x10];
} global_descriptor_table={
	{ 0 },
	{ 0xffff, 0, 0, 0x93, 0, 0 },
	{ 0xffff, 0, 0, 0x93, 0, 0 },
};

int main(void)
{
	unsigned long adr, adr2;
	FILE *file;
	union REGS regs, oregs;
	struct SREGS segs;

	file=fopen("dumpat.bin","wb");

	// hopefully the i386 addresses gives 286 addresses in 286 based systems
//	for (adr=0x00f00000ul; adr!=0x01000000ul; adr+=0x4000) {
	for (adr=0xfff00000ul; adr!=0x00000000ul; adr+=0x4000ul) {

		regs.x.cx=0x4000/2;
		regs.h.ah=0x87;
		segs.es=FP_SEG(&global_descriptor_table);
		regs.x.si=FP_OFF(&global_descriptor_table);

		memset(&global_descriptor_table, 0, sizeof(global_descriptor_table));
		global_descriptor_table.src.limit=0xffff;
		global_descriptor_table.src.address_low=adr&0xffff;
		global_descriptor_table.src.address_high=(adr>>16)&0xff;
		global_descriptor_table.src.address_high386=(adr>>24);
		global_descriptor_table.src.rights=0x93;

		adr2=FP_OFF(buffer)+((unsigned long)FP_SEG(buffer)<<4);
		global_descriptor_table.dest.limit=0xffff;
		global_descriptor_table.dest.address_low=adr2&0xffff;
		global_descriptor_table.dest.address_high=(adr2>>16)&0xff;
		global_descriptor_table.dest.address_high386=(adr2>>24);
		global_descriptor_table.dest.rights=0x93;

asm cli;
		int86x(0x15, &regs, &oregs, &segs);
asm sti;
		printf("ah:%.2x adr %lx\n", oregs.h.ah, adr);
		fwrite(buffer,1,0x4000,file);
	}

	fclose(file);
	return 0;
}

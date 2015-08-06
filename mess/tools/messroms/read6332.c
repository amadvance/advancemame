#include <stdio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/io.h>
#include <fcntl.h>
#include <unistd.h>

/* simple linux utility (needs root privileg)
   dumps mos 6332 roms
   ct eprop needed */
   
// gcc -O
// important because of inb, outb

#define PORT_BASE 0x3e0

void wait_ticks(int c)
{
	struct timeval now, last;
	unsigned long long a,b;
	gettimeofday(&last,NULL);
	a=1000000ull*last.tv_sec+last.tv_usec;
	for (;;) {
		gettimeofday(&now,NULL);
		b=1000000ull*now.tv_sec+now.tv_usec;
		if (b-a>c) break;
	}
}

/*
  ct eprop
  8 bit socket

  1		A7
  2		A6
  3		A5
  4		A4
  5		A3
  6		A2
  7		A1
  8		A0
  9		D0
  10	D1
  11	D2
  12	GND

  13	D3
  14	D4
  15	D5
  16	D6
  17	D7
  18	/CE should be A11!
  19	A10
  20	/OE should be !CS
  21	A11 should be CS
  22	A9
  23	A8
  24	5V

 */

/*
  mos 6332
  4 kbyte 8bit rom

  1		A7
  2		A6
  3		A5
  4		A4
  5		A3
  6		A2
  7		A1
  8		A0
  9		D0
  10	D1
  11	D2
  12	GND

  13	D3
  14	D4
  15	D5
  16	D6
  17	D7
  18	A11
  19	A10
  20	CS1
  21	CS2
  22	A9
  23	A8
  24	5V

 */


int port[8]={0};

void out_port(int value, int off)
{
	port[off]=value;
	outb(value, PORT_BASE+off);
}

void out_addr(int addr)
{
	int temp;

	out_port(addr&0xff,0);

	temp=port[1];
	temp&=~7;
	temp|=(addr>>8)&7;
	out_port(temp, 1);

	temp=port[3];
	temp&=~0x40;
	if (addr&0x800) temp|=0x40;
	out_port(temp,3);
}

int main(void)
{
	int value, i;
	unsigned char data[0x1000];
	FILE *file;

	ioperm(PORT_BASE,8,1);

	out_port(0,0);
	out_port(8,1); // CS on
	out_port(0,2); // 
	out_port(0,6); // vpp off

	out_port(4,3); // VCC on, VCC=5V, Socket=24pin, /OE=0, read

	wait_ticks (100000);

//	for (i=0; i<0x40; i++) {
	for (i=0; i<0x1000; i++) {
		out_addr(i);
		wait_ticks(100);
		value=inb(PORT_BASE+4);
		data[i]=value;
		printf("%.4x %.2x\n", i, value);
	}
	out_port(0xf8,2); // led off
	out_port(0,3); //VCC off
	out_port(0,6); //VPP off
	out_port(0,0);
	
	file=fopen("6332.bin","wb");
	fwrite(data,1,sizeof(data),file);
	fclose(file);

	return 0;
}

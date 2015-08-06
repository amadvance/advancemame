#include <stdio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/io.h>
#include <fcntl.h>
#include <unistd.h>

/* simple linux utility (needs root privileg)
   dumps mos 6530 roms
   ct eprop needed 
   you have to pick out
   the position of registers
   (64 bytes something like ff 00 )
   the position of ram
   (64 bytes, most ff)
   the position of the rom
   (1024 bytes data)
   unused is in my eprommer zero
*/
   
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

  1		A19
  2		A16
  3		A15
  4		A12
  5		A7
  6		A6
  7		A5
  8		A4
  9		A3
  10	A2
  11	A1
  12	A0
  13	D0
  14	D1
  15	D2
  16	GND

  17	D3
  18	D4
  19	D5
  20	D6
  21	D7
  22	/CE
  23	A10
  24	/OE
  25	A11
  26	A9
  27	A8
  28	A13
  29	A14
  30	A17
  31	A18
  32	5V

 */

/*
  mos 6530

  1		GND
  2		PA0
  3		Phi2 connected to ce (simulating clock seams important)
  4		RS0 (A10)
  5		A9 
  6		A8 
  7		A7 
  8		A6 
  9		R/W
  10	A5
  11	A4
  12	A3
  13	A2
  14	A1
  15	A0
  16	/RES connected to a13
  17	PB7, /IRQ
  18	PB6, CS1 (A11)
  19	PB5, CS2 (A12)
  20	5V

  21	PB4
  22	PB3
  23	PB2
  24	PB1
  25	PB0
  26	D7 
  27	D6 
  28	D5 
  29	D4 
  30	D3
  31	D2
  32	D1 
  33	D0 
  34	PA7
  35	PA6
  36	PA5
  37	PA4
  38	PA3
  39	PA2
  40	PA1

connected a0..a12, d0..d7, gnd, 5v, R/W

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
	temp&=~0x1f;
	temp|=(addr>>8)&0x1f;
	out_port(temp, 1);
}

void clk(void)
{
	out_port(port[3]&~0x40,3);
	wait_ticks(1);
	out_port(0x40|port[3],3);
	wait_ticks(1);
}

int main(void)
{
	int value, i;
	unsigned char data[0x2000];
	FILE *file;

	ioperm(PORT_BASE,8,1);

	out_port(0,0);
	out_port(0,1); // reset
	out_port(0x20,2); // 32 pol led
	out_port(0,6); // vpp off

	out_port(0x11,3); // VCC on, VCC=5V, Socket=24pin, /OE=0, read

	wait_ticks (100000);

	clk();
	out_port(0x20|port[1],1); // reset off
	clk();


	for (i=0; i<0x2000; i++) {
		if ((i&0xf)==0) printf("%.4x:",i);
		out_addr(i);
		clk();
		wait_ticks(1);
		value=inb(PORT_BASE+4);
		data[i]=value;
		printf("%.2x",value);
		if ((i&0xf)==0xf) printf("\n");
	}
	out_port(0xf8,2); // led off
	out_port(0,3); //VCC off
	out_port(0,6); //VPP off
	out_port(0,0);
	
	file=fopen("6530.bin","wb");
	fwrite(data,1,sizeof(data),file);
	fclose(file);

	return 0;
}

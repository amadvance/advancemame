/* Sega ST-V (Sega Titan Video)

a.k.a. known as a Sega Saturn for the Arcades.

Driver by David Haywood,Angelo Salese,Olivier Galibert & Mariusz Wojcieszek
SCSP driver provided by R.Belmont,based on ElSemi's SCSP sound chip emulator
CD Block driver provided by ANY,based on sthief original emulator
Many thanks to Guru,Fabien & Runik for the help given.

Hardware overview:
------------------
-two SH-2 CPUs,in a master/slave configuration.The master cpu is used to
boot-up and to do the most of the work,the slave one does extra work that could be
too much for a single cpu.They both shares all the existant devices;
-a M68000 CPU,used to drive sound(the SCSP chip).The program is uploaded via the
SH-2 cpus;
-a SMPC (System Manager & Peripheral Control),used to drive all the
devices on the board;
-a SCU (System Control Unit),mainly used to do DMA operations and to drive interrupts,it
also has a DSP;
-an (optional for the ST-V) SH-1 CPU,used to be the CD driver;
-An A-Bus,where the cart ROM area is located;
-A B-Bus,where the Video Hardware & the SCU sections are located;
-Two VDPs chips(named as 1 & 2),used for the video section:
 -VDP1 is used to render sprites & polygons.
 -VDP2 is for the tilemap system,there are:
 4 effective normal layers;
 2 roz layers;
 1 back layer;
 1 line layer;
 The VDP2 is capable of the following things (in order):
 -dynamic resolution (up to 704x512) & various interlacing modes;
 -mosaic process;
 -scrolling,scaling,horizontal & vertical cell scrolling & linescroll for the
  normal planes, the roz ones can also rotate;
 -versatile window system,used for various effects;
 -alpha-blending,refered as Color Calculation in the docs;
 -shadow effects;
 -global rgb brightness control,separate for every plane;

this hardware comes above hell on the great list of hellish things as far as emulation goes anyway ;-)

Memory map:
-----------

0x00000000, 0x0007ffff  BIOS ROM
0x00080000, 0x000fffff  Unused
0x00100000, 0x00100080  SMPC
0x00100080, 0x0017ffff  Unused
0x00180000, 0x0018ffff  Back Up Ram
0x00190000, 0x001fffff  Unused
0x00200000, 0x002fffff  Work Ram-L
0x00300000, 0x00ffffff  Unused
0x01000000, 0x01000003  MINIT
0x01000004, 0x017fffff  Unused
0x01800000, 0x01800003  SINIT
0x01800004, 0x01ffffff  Unused
0x02000000, 0x03ffffff  A-BUS CS0
0x04000000, 0x04ffffff  A-BUS CS1
0x05000000, 0x057fffff  A-BUS DUMMY
0x05800000, 0x058fffff  A-BUS CS2
0x05900000, 0x059fffff  Unused
0x05a00000, 0x05b00ee3  Sound Region
0x05b00ee4, 0x05bfffff  Unused
0x05c00000, 0x05cbffff  VDP 1
0x05cc0000, 0x05cfffff  Unused
0x05d00000, 0x05d00017  VDP 1 regs
0x05d00018, 0x05dfffff  Unused
0x05e00000, 0x05e7ffff  VDP2
0x05e80000, 0x05efffff  VDP2 Extra RAM,accessible thru the VRAMSZ register
0x05f00000, 0x05f00fff  VDP2 Color RAM
0x05f01000, 0x05f7ffff  Unused
0x05f80000  0x05f8011f  VDP2 regs
0x05f80120, 0x05fdffff  Unused
0x05fe0000, 0x05fe00cf  SCU regs
0x05fe00d0, 0x05ffffff  Unused
0x06000000, 0x060fffff  Work Ram-H
0x06100000, 0x07ffffff  Unused

*the unused locations aren't known if they are really unused or not,needs verification,most
of them seem just mirrors of the previous valid memory allocated.

ToDo / Notes:
-------------

-To enter into an Advanced Test Mode,keep pressed the Test Button (F2) on the start-up.

(Main issues)
-complete the Master/Slave communication.
-clean up the IC13 rom loading.
-Clean-ups and split the various chips(SCU,SMPC)into their respective files.
-CD block:complete it & add proper CD image support into MAME.
-the Cart-Dev mode...we need the proper ST-V Cart-Dev bios to be dumped in order to
 make this work,but probably this will never be done...
-fix some strange sound cpu memory accesses,there are various issues about this.
-finish the DSP core.
-Complete the window system in VDP2 (Still in progress).
-Add the RS232c interface (serial port),needed by fhboxers.
-(PCB owners) check if the clocks documented in the manuals are really right for ST-V.
-SCSP to master irq: see if there is a sound cpu mask bit.
-Does the cpunum_set_clock really works?Investigate.
-We need to check every game if can be completed or there are any hanging/crash/protection
 issues on them.
-Memo: Some tests done on the original & working PCB,to be implemented:
 -The AD-Stick returns 0x00 or a similar value.
 -The Ports E,F & G must return 0xff
 -The regular BIOS tests (Memory Test) changes his background color at some point to
  several gradients of red,green and blue.Current implementation remains black.I dunno
  if this is a framebuffer write or a funky transparency issue (i.e TRANSPARENT_NONE
  should instead show the back layer).
 -RBG0 rotating can be checked on the "Advanced Test" menu thru the VDP1/VDP2 check.
  It rotates clockwise IIRC.Also the ST-V logo when the game is in Multi mode rotates too.
 -The MIDI communication check fails even on a ST-V board,somebody needs to check if there
  is a MIDI port on the real PCB...

(per-game issues)
-groovef: hangs soon after loaded,caused by two memory addresses in the Work RAM-H range.
 Kludged for now to work.
-various: find idle skip if possible.
-suikoenb/shanhigw + others: why do we get 2 credits on startup with sound enabled?
-colmns97/puyosun/mausuke/cotton2/cottonbm: interrupt issues? we can't check the SCU mask
 on SMPC or controls fail
-mausuke/bakubaku/grdforce: need to sort out transparency on the colour mapped sprites
-bakubaku/colmns97/vfkids: no sound? Caused by missing irq?
-myfairld: Doesn't work with -sound enabled because of a sound ram check at relative
 addresses of $700/$710/$720/$730,so I'm not removing the NOT_WORKING flag due of that.Also
 Micronet programmers had the "great" idea to *not* use the ST-V input standards,infact
 joystick panel is mapped with input_port(10) instead of input_port(2),so for now use
 the mahjong panel instead.
-kiwames: the VDP1 sprites refresh is too slow,causing the "Draw by request" mode to
 flicker.Moved back to default ATM...
-pblbeach: Sprites are offset, because it doesn't clear vdp1 local coordinates set by bios,
 I guess that they are cleared when some vdp1 register is written (kludged for now)
-decathlt: Is currently using a strange protection DMA abus control,and it uses some sort of RLE
 compression/encryption that serves as a gfxdecode.
-vmahjong: the vdp1 textures are too dark(women).
-findlove: controls doesn't work?
-Weird design choices:
 introdon: has the working button as BUTTON2 and not BUTTON1
 batmanfr: BUTTON1 isn't used at all.
-seabass: Player sprite is corrupt/missing during movements,caused
 by incomplete framebuffer switching.
-sss: Missing backgrounds during gameplay. <- seems just too dark (night),probably
 just the positioning isn't correct...
-elandore: Polygons structures/textures aren't right in gameplay,known as protection
 for the humans structures,imperfect VDP1 emulation for the dragons.
-hanagumi: ending screens have corrupt graphics. (*untested*)
-znpwfv: missing Gouraud shading on distorted sprites and polygons
-batmanfr: Missing sound,caused by an extra ADSP chip which is on the cart.The CPU is a
 ADSP-2181,and it's the same used by NBA Jam Extreme (ZN game).
-twcup98: missing Tecmo logo
-vfremix: hangs after second match
-sokyugrt: gameplay seems to be not smooth, timing?
-Here's the list of unmapped read/writes:
*<all games>
cpu #0 (PC=0000365C): unmapped program memory dword write to 057FFFFC = 000D0000 & FFFF0000
cpu #0 (PC=00003654): unmapped program memory dword write to 057FFFFC = 000C0000 & FFFF0000
*bakubaku:
cpu #0 (PC=0601022E): unmapped program memory dword write to 02000000 = 00000000 & FFFFFFFF
cpu #0 (PC=0601023A): unmapped program memory dword write to 02000000 = 00000000 & FFFFFFFF

*/

#include "driver.h"
#include "machine/eeprom.h"
#include "cpu/sh2/sh2.h"
#include "machine/stvcd.h"
#include "machine/scudsp.h"
#include "sound/scsp.h"
#include <time.h>

extern UINT32* stv_vdp2_regs;
extern UINT32* stv_vdp2_vram;
extern UINT32* stv_vdp2_cram;
extern UINT32* stv_vdp1_vram;

#define USE_SLAVE 1
/*Enable the following to play 'myfairld' without using the -nosound switch.*/
#define UGLY_SOUND_HACK 0

#ifdef MAME_DEBUG
#define LOG_CDB  0
#define LOG_SMPC 1
#define LOG_SCU  1
#define LOG_IRQ  0
#define LOG_IOGA 1
#else
#define LOG_CDB  0
#define LOG_SMPC 0
#define LOG_SCU  0
#define LOG_IRQ  0
#define LOG_IOGA 0
#endif

#define MASTER_CLOCK_352 57272800
#define MASTER_CLOCK_320 53748200

/* stvhacks.c */
DRIVER_INIT( ic13 );
NVRAM_HANDLER( stv );
void install_stvbios_speedups(void);
DRIVER_INIT(bakubaku);
DRIVER_INIT(mausuke);
DRIVER_INIT(puyosun);
DRIVER_INIT(shienryu);
DRIVER_INIT(prikura);
DRIVER_INIT(hanagumi);
DRIVER_INIT(cottonbm);
DRIVER_INIT(cotton2);
DRIVER_INIT(fhboxers);
DRIVER_INIT(dnmtdeka);
DRIVER_INIT(groovef);
DRIVER_INIT(danchih);
DRIVER_INIT(astrass);
DRIVER_INIT(thunt);
DRIVER_INIT(grdforce);
DRIVER_INIT(batmanfr);
DRIVER_INIT(winterht);
DRIVER_INIT(seabass);
DRIVER_INIT(vfremix);
DRIVER_INIT(diehard);
DRIVER_INIT(sss);
DRIVER_INIT(othellos);
DRIVER_INIT(sassisu);
DRIVER_INIT(gaxeduel);
DRIVER_INIT(suikoenb);
DRIVER_INIT(sokyugrt);
DRIVER_INIT(znpwfv);
DRIVER_INIT(twcup98);
DRIVER_INIT(smleague);
DRIVER_INIT(maruchan);
DRIVER_INIT(sandor);
DRIVER_INIT(colmns97);
DRIVER_INIT(pblbeach);
DRIVER_INIT(shanhigw);
DRIVER_INIT(finlarch);
DRIVER_INIT(elandore);
DRIVER_INIT(rsgun);

/**************************************************************************************/
/*to be added into a stv Header file,remember to remove all the static...*/

static UINT8 *smpc_ram;
//static void stv_dump_ram(void);

UINT32* stv_workram_l;
UINT32* stv_workram_h;
UINT32* stv_backupram;
UINT32* stv_scu;
static UINT32* ioga;
static UINT16* scsp_regs;
static UINT16* sound_ram;

int stv_vblank,stv_hblank;
int stv_enable_slave_sh2;
/*SMPC stuff*/
static UINT8 NMI_reset;
static void system_reset(void);
static UINT8 en_68k;
/*SCU stuff*/
static int 	  timer_0;			/* Counter for Timer 0 irq*/
static int    timer_1;          /* Counter for Timer 1 irq*/
/*Maybe add these in a struct...*/
static UINT32 scu_src_0,		/* Source DMA lv 0 address*/
			  scu_src_1,		/* lv 1*/
			  scu_src_2,		/* lv 2*/
			  scu_dst_0,		/* Destination DMA lv 0 address*/
			  scu_dst_1,		/* lv 1*/
			  scu_dst_2,		/* lv 2*/
			  scu_src_add_0,	/* Source Addition for DMA lv 0*/
			  scu_src_add_1,	/* lv 1*/
			  scu_src_add_2,	/* lv 2*/
			  scu_dst_add_0,	/* Destination Addition for DMA lv 0*/
			  scu_dst_add_1,	/* lv 1*/
			  scu_dst_add_2;	/* lv 2*/
static INT32  scu_size_0,		/* Transfer DMA size lv 0*/
			  scu_size_1,		/* lv 1*/
			  scu_size_2;		/* lv 2*/

static void dma_direct_lv0(void);	/*DMA level 0 direct transfer function*/
static void dma_direct_lv1(void);   /*DMA level 1 direct transfer function*/
static void dma_direct_lv2(void);   /*DMA level 2 direct transfer function*/
static void dma_indirect_lv0(void); /*DMA level 0 indirect transfer function*/
static void dma_indirect_lv1(void); /*DMA level 1 indirect transfer function*/
static void dma_indirect_lv2(void); /*DMA level 2 indirect transfer function*/


int minit_boost,sinit_boost;
double minit_boost_timeslice, sinit_boost_timeslice;

static int scanline;


/*A-Bus IRQ checks,where they could be located these?*/
#define ABUSIRQ(_irq_,_vector_,_mask_) \
	if(!(stv_scu[40] & _mask_)) { cpunum_set_input_line_and_vector(0, _irq_, HOLD_LINE , _vector_); }
#if 0
if(stv_scu[42] & 1)//IRQ ACK
{
	ABUSIRQ(7,0x50,0x00010000);
	ABUSIRQ(7,0x51,0x00020000);
	ABUSIRQ(7,0x52,0x00040000);
	ABUSIRQ(7,0x53,0x00080000);
	ABUSIRQ(4,0x54,0x00100000);
	ABUSIRQ(4,0x55,0x00200000);
	ABUSIRQ(4,0x56,0x00400000);
	ABUSIRQ(4,0x57,0x00800000);
	ABUSIRQ(1,0x58,0x01000000);
	ABUSIRQ(1,0x59,0x02000000);
	ABUSIRQ(1,0x5a,0x04000000);
	ABUSIRQ(1,0x5b,0x08000000);
	ABUSIRQ(1,0x5c,0x10000000);
	ABUSIRQ(1,0x5d,0x20000000);
	ABUSIRQ(1,0x5e,0x40000000);
	ABUSIRQ(1,0x5f,0x80000000);
}
#endif

/**************************************************************************************/

static int DectoBCD(int num)
{
	int i, cnt = 0, tmp, res = 0;

	while (num > 0) {
		tmp = num;
		while (tmp >= 10) tmp %= 10;
		for (i=0; i<cnt; i++)
			tmp *= 16;
		res += tmp;
		cnt++;
		num /= 10;
	}

	return res;
}


/* SMPC
 System Manager and Peripheral Control

*/
/* SMPC Addresses

00
01 -w  Input Register 0 (IREG)
02
03 -w  Input Register 1
04
05 -w  Input Register 2
06
07 -w  Input Register 3
08
09 -w  Input Register 4
0a
0b -w  Input Register 5
0c
0d -w  Input Register 6
0e
0f
10
11
12
13
14
15
16
17
18
19
1a
1b
1c
1d
1e
1f -w  Command Register (COMREG)
20
21 r-  Output Register 0 (OREG)
22
23 r-  Output Register 1
24
25 r-  Output Register 2
26
27 r-  Output Register 3
28
29 r-  Output Register 4
2a
2b r-  Output Register 5
2c
2d r-  Output Register 6
2e
2f r-  Output Register 7
30
31 r-  Output Register 8
32
33 r-  Output Register 9
34
35 r-  Output Register 10
36
37 r-  Output Register 11
38
39 r-  Output Register 12
3a
3b r-  Output Register 13
3c
3d r-  Output Register 14
3e
3f r-  Output Register 15
40
41 r-  Output Register 16
42
43 r-  Output Register 17
44
45 r-  Output Register 18
46
47 r-  Output Register 19
48
49 r-  Output Register 20
4a
4b r-  Output Register 21
4c
4d r-  Output Register 22
4e
4f r-  Output Register 23
50
51 r-  Output Register 24
52
53 r-  Output Register 25
54
55 r-  Output Register 26
56
57 r-  Output Register 27
58
59 r-  Output Register 28
5a
5b r-  Output Register 29
5c
5d r-  Output Register 30
5e
5f r-  Output Register 31
60
61 r-  SR
62
63 rw  SF
64
65
66
67
68
69
6a
6b
6c
6d
6e
6f
70
71
72
73
74
75 rw PDR1
76
77 rw PDR2
78
79 -w DDR1
7a
7b -w DDR2
7c
7d -w IOSEL2/1
7e
7f -w EXLE2/1
*/
static UINT8 IOSEL1;
static UINT8 IOSEL2;
static UINT8 EXLE1;
static UINT8 EXLE2;
static UINT8 PDR1;
static UINT8 PDR2;

#define SH2_DIRECT_MODE_PORT_1 IOSEL1 = 1
#define SH2_DIRECT_MODE_PORT_2 IOSEL2 = 1
#define SMPC_CONTROL_MODE_PORT_1 IOSEL1 = 0
#define SMPC_CONTROL_MODE_PORT_2 IOSEL2 = 0

static void system_reset()
{
	/*Only backup ram and SMPC ram are retained after that this command is issued.*/
	memset(stv_scu      ,0x00,0x000100);
	memset(scsp_regs    ,0x00,0x001000);
	memset(sound_ram    ,0x00,0x080000);
	memset(stv_workram_h,0x00,0x100000);
	memset(stv_workram_l,0x00,0x100000);
	memset(stv_vdp2_regs,0x00,0x040000);
	memset(stv_vdp2_vram,0x00,0x100000);
	memset(stv_vdp2_cram,0x00,0x080000);
	//vdp1
	//A-Bus
	/*Order is surely wrong but whatever...*/
}

static UINT8 stv_SMPC_r8 (int offset)
{
	int return_data;


	return_data = smpc_ram[offset];

	if ((offset == 0x61)) // ?? many games need this or the controls don't work
		return_data = 0x20 ^ 0xff;

	if (offset == 0x75)//PDR1 read
		return_data = readinputport(0);

	if (offset == 0x77)//PDR2 read
		return_data=  (0xfe | EEPROM_read_bit());

//  if (offset == 0x33) //country code
//      return_data = readinputport(7);

	if (activecpu_get_pc()==0x060020E6) return_data = 0x10;//???

	//if(LOG_SMPC) logerror ("cpu #%d (PC=%08X) SMPC: Read from Byte Offset %02x Returns %02x\n", cpu_getactivecpu(), activecpu_get_pc(), offset, return_data);


	return return_data;
}

static void stv_SMPC_w8 (int offset, UINT8 data)
{
	time_t ltime;
	struct tm *today;
	time(&ltime);
	today = localtime(&ltime);

//  if(LOG_SMPC) logerror ("8-bit SMPC Write to Offset %02x with Data %02x\n", offset, data);
	smpc_ram[offset] = data;

	if(offset == 0x75)
	{
		EEPROM_set_clock_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);
		EEPROM_write_bit(data & 0x10);
		EEPROM_set_cs_line((data & 0x04) ? CLEAR_LINE : ASSERT_LINE);


//      if (data & 0x01)
//          if(LOG_SMPC) logerror("bit 0 active\n");
//      if (data & 0x02)
//          if(LOG_SMPC) logerror("bit 1 active\n");
//      if (data & 0x10)
			//if(LOG_SMPC) logerror("bit 4 active\n");//LOT
		//if(LOG_SMPC) logerror("SMPC: ram [0x75] = %02x\n",smpc_ram[0x75]);
		PDR1 = (data & 0x60);
	}

	if(offset == 0x77)
	{
		/*
            ACTIVE LOW
            bit 4(0x10) - Enable Sound System
        */
		//ui_popup("PDR2 = %02x",smpc_ram[0x77]);
		if(!(smpc_ram[0x77] & 0x10))
		{
			if(LOG_SMPC) logerror("SMPC: M68k on\n");
			cpunum_set_input_line(2, INPUT_LINE_RESET, CLEAR_LINE);
			en_68k = 1;
		}
		else
		{
			if(LOG_SMPC) logerror("SMPC: M68k off\n");
			cpunum_set_input_line(2, INPUT_LINE_RESET, ASSERT_LINE);
			en_68k = 0;
		}
		//if(LOG_SMPC) logerror("SMPC: ram [0x77] = %02x\n",smpc_ram[0x77]);
		PDR2 = (data & 0x60);
	}

	if(offset == 0x7d)
	{
		if(smpc_ram[0x7d] & 1)
			SH2_DIRECT_MODE_PORT_1;
		else
			SMPC_CONTROL_MODE_PORT_1;

		if(smpc_ram[0x7d] & 2)
			SH2_DIRECT_MODE_PORT_2;
		else
			SMPC_CONTROL_MODE_PORT_2;
	}

	if(offset == 0x7f)
	{
		//enable PAD irq & VDP2 external latch for port 1/2
		EXLE1 = smpc_ram[0x7f] & 1 ? 1 : 0;
		EXLE2 = smpc_ram[0x7f] & 2 ? 1 : 0;
		if(EXLE1 || EXLE2)
			if(!(stv_scu[40] & 0x0100)) /*Pad irq*/
			{
				if(LOG_SMPC) logerror ("Interrupt: PAD irq at scanline %04x, Vector 0x48 Level 0x08\n",scanline);
				cpunum_set_input_line_and_vector(0, 8, HOLD_LINE , 0x48);
			}
	}

	if (offset == 0x1f)
	{
		switch (data)
		{
			case 0x00:
				if(LOG_SMPC) logerror ("SMPC: Master ON\n");
				smpc_ram[0x5f]=0x00;
				break;
			//in theory 0x01 is for Master OFF,but obviously is not used.
			case 0x02:
				if(LOG_SMPC) logerror ("SMPC: Slave ON\n");
				smpc_ram[0x5f]=0x02;
				#if USE_SLAVE
				stv_enable_slave_sh2 = 1;
				cpunum_set_input_line(1, INPUT_LINE_RESET, CLEAR_LINE);
				#endif
				break;
			case 0x03:
				if(LOG_SMPC) logerror ("SMPC: Slave OFF\n");
				smpc_ram[0x5f]=0x03;
				stv_enable_slave_sh2 = 0;
				cpu_trigger(1000);
				cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
				break;
			case 0x06:
				if(LOG_SMPC) logerror ("SMPC: Sound ON\n");
				/* wrong? */
				smpc_ram[0x5f]=0x06;
				cpunum_set_input_line(2, INPUT_LINE_RESET, CLEAR_LINE);
				break;
			case 0x07:
				if(LOG_SMPC) logerror ("SMPC: Sound OFF\n");
				smpc_ram[0x5f]=0x07;
				break;
			/*CD (SH-1) ON/OFF,guess that this is needed for Sports Fishing games...*/
			//case 0x08:
			//case 0x09:
			case 0x0d:
				if(LOG_SMPC) logerror ("SMPC: System Reset\n");
				smpc_ram[0x5f]=0x0d;
				cpunum_set_input_line(0, INPUT_LINE_RESET, PULSE_LINE);
				system_reset();
				break;
			case 0x0e:
				if(LOG_SMPC) logerror ("SMPC: Change Clock to 352\n");
				smpc_ram[0x5f]=0x0e;
				cpunum_set_clock(0, MASTER_CLOCK_352/2);
				cpunum_set_clock(1, MASTER_CLOCK_352/2);
				cpunum_set_clock(2, MASTER_CLOCK_352/5);
				cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE); // ff said this causes nmi, should we set a timer then nmi?
				break;
			case 0x0f:
				if(LOG_SMPC) logerror ("SMPC: Change Clock to 320\n");
				smpc_ram[0x5f]=0x0f;
				cpunum_set_clock(0, MASTER_CLOCK_320/2);
				cpunum_set_clock(1, MASTER_CLOCK_320/2);
				cpunum_set_clock(2, MASTER_CLOCK_320/5);
				cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE); // ff said this causes nmi, should we set a timer then nmi?
				break;
			/*"Interrupt Back"*/
			case 0x10:
				if(LOG_SMPC) logerror ("SMPC: Status Acquire\n");
				smpc_ram[0x5f]=0x10;
				smpc_ram[0x21] = (0x80) | ((NMI_reset & 1) << 6);
			  	smpc_ram[0x23] = DectoBCD((today->tm_year + 1900)/100);
		    	smpc_ram[0x25] = DectoBCD((today->tm_year + 1900)%100);
	    		smpc_ram[0x27] = (today->tm_wday << 4) | (today->tm_mon+1);
		    	smpc_ram[0x29] = DectoBCD(today->tm_mday);
		    	smpc_ram[0x2b] = DectoBCD(today->tm_hour);
		    	smpc_ram[0x2d] = DectoBCD(today->tm_min);
		    	smpc_ram[0x2f] = DectoBCD(today->tm_sec);

				smpc_ram[0x31]=0x00;  //?

				//smpc_ram[0x33]=readinputport(7);

				smpc_ram[0x35]=0x00;
				smpc_ram[0x37]=0x00;

				smpc_ram[0x39]=0xff;
				smpc_ram[0x3b]=0xff;
				smpc_ram[0x3d]=0xff;
				smpc_ram[0x3f]=0xff;
				smpc_ram[0x41]=0xff;
				smpc_ram[0x43]=0xff;
				smpc_ram[0x45]=0xff;
				smpc_ram[0x47]=0xff;
				smpc_ram[0x49]=0xff;
				smpc_ram[0x4b]=0xff;
				smpc_ram[0x4d]=0xff;
				smpc_ram[0x4f]=0xff;
				smpc_ram[0x51]=0xff;
				smpc_ram[0x53]=0xff;
				smpc_ram[0x55]=0xff;
				smpc_ram[0x57]=0xff;
				smpc_ram[0x59]=0xff;
				smpc_ram[0x5b]=0xff;
				smpc_ram[0x5d]=0xff;

			//  /*This is for RTC,cartridge code and similar stuff...*/
			//  if(!(stv_scu[40] & 0x0080)) /*System Manager(SMPC) irq*/ /* we can't check this .. breaks controls .. probably issues elsewhere? */
				{
					if(LOG_SMPC) logerror ("Interrupt: System Manager (SMPC) at scanline %04x, Vector 0x47 Level 0x08\n",scanline);
					cpunum_set_input_line_and_vector(0, 8, HOLD_LINE , 0x47);
				}
			break;
			/* RTC write*/
			case 0x16:
				if(LOG_SMPC) logerror("SMPC: RTC write\n");
				smpc_ram[0x2f] = smpc_ram[0x0d];
				smpc_ram[0x2d] = smpc_ram[0x0b];
				smpc_ram[0x2b] = smpc_ram[0x09];
				smpc_ram[0x29] = smpc_ram[0x07];
				smpc_ram[0x27] = smpc_ram[0x05];
				smpc_ram[0x25] = smpc_ram[0x03];
				smpc_ram[0x23] = smpc_ram[0x01];
				smpc_ram[0x5f]=0x16;
			break;
			/* SMPC memory setting*/
			case 0x17:
				if(LOG_SMPC) logerror ("SMPC: memory setting\n");
				smpc_ram[0x5f]=0x17;
			break;
			case 0x18:
				if(LOG_SMPC) logerror ("SMPC: NMI request\n");
				smpc_ram[0x5f]=0x18;
				/*NMI is unconditionally requested?*/
				cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
				break;
			case 0x19:
				if(LOG_SMPC) logerror ("SMPC: NMI Enable\n");
				smpc_ram[0x5f]=0x19;
				NMI_reset = 0;
				smpc_ram[0x21] = (0x80) | ((NMI_reset & 1) << 6);
				break;
			case 0x1a:
				if(LOG_SMPC) logerror ("SMPC: NMI Disable\n");
				smpc_ram[0x5f]=0x1a;
				NMI_reset = 1;
				smpc_ram[0x21] = (0x80) | ((NMI_reset & 1) << 6);

				break;
			default:
				if(LOG_SMPC) logerror ("cpu #%d (PC=%08X) SMPC: undocumented Command %02x\n", cpu_getactivecpu(), activecpu_get_pc(), data);
		}

		// we've processed the command, clear status flag
		smpc_ram[0x63] = 0x00;
		/*TODO:emulate the timing of each command...*/
	}
}


static READ32_HANDLER ( stv_SMPC_r32 )
{
	int byte = 0;
	int readdata = 0;
	/* registers are all byte accesses, convert here */
	offset = offset << 2; // multiply offset by 4

	if (!(mem_mask & 0xff000000))	{ byte = 0; readdata = stv_SMPC_r8(offset+byte) << 24; }
	if (!(mem_mask & 0x00ff0000))	{ byte = 1; readdata = stv_SMPC_r8(offset+byte) << 16; }
	if (!(mem_mask & 0x0000ff00))	{ byte = 2; readdata = stv_SMPC_r8(offset+byte) << 8;  }
	if (!(mem_mask & 0x000000ff))	{ byte = 3; readdata = stv_SMPC_r8(offset+byte) << 0;  }

	return readdata;
}


static WRITE32_HANDLER ( stv_SMPC_w32 )
{
	int byte = 0;
	int writedata = 0;
	/* registers are all byte accesses, convert here so we can use the data more easily later */
	offset = offset << 2; // multiply offset by 4

	if (!(mem_mask & 0xff000000))	{ byte = 0; writedata = data >> 24; }
	if (!(mem_mask & 0x00ff0000))	{ byte = 1; writedata = data >> 16; }
	if (!(mem_mask & 0x0000ff00))	{ byte = 2; writedata = data >> 8;  }
	if (!(mem_mask & 0x000000ff))	{ byte = 3; writedata = data >> 0;  }

	writedata &= 0xff;

	offset += byte;

	stv_SMPC_w8(offset,writedata);
}


/*
(Preliminary) explaination about this:
VBLANK-OUT is used at the start of the vblank period.It also sets the timer zero
variable to 0.
If the Timer Compare register is zero too,the Timer 0 irq is triggered.

HBLANK-IN is used at the end of each scanline except when in VBLANK-IN/OUT periods.

The timer 0 is also incremented by one at each HBLANK and checked with the value
of the Timer Compare register;if equal,the timer 0 irq is triggered here too.
Notice that the timer 0 compare register can be more than the VBLANK maximum range,in
this case the timer 0 irq is simply never triggered.This is a known Sega Saturn/ST-V "bug".

VBLANK-IN is used at the end of the vblank period.

SCU register[36] is the timer zero compare register.
SCU register[40] is for IRQ masking.
*/

/* to do, update bios idle skips so they work better with this arrangement.. */


static INTERRUPT_GEN( stv_interrupt )
{
	scanline = 261-cpu_getiloops();

	stv_hblank = 0;

	if(scanline == 0)
	{
		if(!(stv_scu[40] & 2))/*VBLANK-OUT*/
		{
			if(LOG_IRQ) logerror ("Interrupt: VBlank-OUT at scanline %04x, Vector 0x41 Level 0x0e\n",scanline);
			cpunum_set_input_line_and_vector(0, 0xe, HOLD_LINE , 0x41);
		}
		stv_vblank = 0;
		#if UGLY_SOUND_HACK
		sound_ram[0x700/2] = 0x0000;
		sound_ram[0x710/2] = 0x0000;
		sound_ram[0x720/2] = 0x0000;
		sound_ram[0x730/2] = 0x0000;
		#endif
	}
	else if(scanline <= 223 && scanline >= 1)/*Correct?*/
	{
		timer_0++;
		timer_1++;
		stv_hblank = 1;
		/*TODO: check this...*/
		/*Timer 1 handling*/
		if((stv_scu[38] & 1))
		{
			if((!(stv_scu[40] & 0x10)) && (!(stv_scu[38] & 0x80)))
			{
				if(LOG_IRQ) logerror ("Interrupt: Timer 1 at scanline %04x, Vector 0x44 Level 0x0b\n",scanline);
				cpunum_set_input_line_and_vector(0, 0xb, HOLD_LINE, 0x44 );
			}
			else if((!(stv_scu[40] & 0x10)) && (stv_scu[38] & 0x80))
			{
				if(timer_1 == (stv_scu[36] & 0x1ff))
				{
					if(LOG_IRQ) logerror ("Interrupt: Timer 1 at scanline %04x, Vector 0x44 Level 0x0b\n",scanline);
					cpunum_set_input_line_and_vector(0, 0xb, HOLD_LINE, 0x44 );
				}
			}
		}
		if(timer_0 == (stv_scu[36] & 0x1ff))
		{
			if(!(stv_scu[40] & 8))/*Timer 0*/
			{
				if(LOG_IRQ) logerror ("Interrupt: Timer 0 at scanline %04x, Vector 0x43 Level 0x0c\n",scanline);
				cpunum_set_input_line_and_vector(0, 0xc, HOLD_LINE, 0x43 );
			}
		}

		/*TODO:use this *at the end* of the draw line.*/
		if(!(stv_scu[40] & 4))/*HBLANK-IN*/
		{
			if(LOG_IRQ) logerror ("Interrupt: HBlank-In at scanline %04x, Vector 0x42 Level 0x0d\n",scanline);
			cpunum_set_input_line_and_vector(0, 0xd, HOLD_LINE , 0x42);
		}
	}
	else if(scanline == 224)
	{
		timer_0 = 0;
		timer_1 = 0;

		if(!(stv_scu[40] & 1))/*VBLANK-IN*/
		{
			if(LOG_IRQ) logerror ("Interrupt: VBlank IN at scanline %04x, Vector 0x40 Level 0x0f\n",scanline);
			cpunum_set_input_line_and_vector(0, 0xf, HOLD_LINE , 0x40);
		}
		stv_vblank = 1;

		if(timer_0 == (stv_scu[36] & 0x1ff))
		{
			if(!(stv_scu[40] & 8))/*Timer 0*/
			{
				if(LOG_IRQ) logerror ("Interrupt: Timer 0 at scanline %04x, Vector 0x43 Level 0x0c\n",scanline);
				cpunum_set_input_line_and_vector(0, 0xc, HOLD_LINE, 0x43 );
			}
		}
		if(!(stv_scu[40] & 0x2000)) /*Sprite draw end irq*/
			cpunum_set_input_line_and_vector(0, 2, HOLD_LINE , 0x4d);
	}
}

/*
I/O overview:
    PORT-A  1st player inputs
    PORT-B  2nd player inputs
    PORT-C  system input
    PORT-D  system output
    PORT-E  I/O 1
    PORT-F  I/O 2
    PORT-G  I/O 3
    PORT-AD AD-Stick inputs?(Fake for now...)
    SERIAL COM

offsets:
    0h PORT-A
    0l PORT-B
    1h PORT-C
    1l PORT-D
    2h PORT-E
    2l PORT-F (extra button layout)
    3h PORT-G
    3l
    4h PORT-SEL
    4l
    5h SERIAL COM WRITE
    5l
    6h SERIAL COM READ
    6l
    7h
    7l PORT-AD
*/
static UINT8 port_ad[] =
{
	0xcc,0xb2,0x99,0x7f,0x66,0x4c,0x33,0x19
};

static UINT8 port_sel,mux_data;

#define HI_WORD_ACCESS (mem_mask & 0x00ff0000) == 0
#define LO_WORD_ACCESS (mem_mask & 0x000000ff) == 0

READ32_HANDLER ( stv_io_r32 )
{
	static int i= -1;
	//if(LOG_IOGA) logerror("(PC=%08X): I/O r %08X & %08X\n", activecpu_get_pc(), offset*4, mem_mask);

	switch(offset)
	{
		case 0:
		switch(port_sel)
		{
			case 0x77: return 0xff000000|(readinputport(2) << 16) |0x0000ff00|(readinputport(3));
			case 0x67:
			{
				switch(mux_data)
				{
					/*Mahjong panel interface,bit wise(ACTIVE LOW)*/
					case 0xfe:	return 0xff000000 | (readinputport(7)  << 16) | 0x0000ff00 | (readinputport(12));
					case 0xfd:  return 0xff000000 | (readinputport(8)  << 16) | 0x0000ff00 | (readinputport(13));
					case 0xfb:	return 0xff000000 | (readinputport(9)  << 16) | 0x0000ff00 | (readinputport(14));
					case 0xf7:	return 0xff000000 | (readinputport(10) << 16) | 0x0000ff00 | (readinputport(15));
					case 0xef:  return 0xff000000 | (readinputport(11) << 16) | 0x0000ff00 | (readinputport(16));
					/*Joystick panel*/
					default:
					//ui_popup("%02x MUX DATA",mux_data);
				    return (readinputport(2) << 16) | (readinputport(3));
				}
			}
			case 0x47:
			{
				int data1 = 0, data2 = 0;

				/* Critter Crusher */
				data1 = readinputport(7);
				data1 = BITSWAP8(data1, 2, 3, 0, 1, 6, 7, 5, 4) & 0xf3;
				data1 |= (readinputport(2) & 1) ? 0x0 : 0x4;
				data2 = readinputport(8);
				data2 = BITSWAP8(data2, 2, 3, 0, 1, 6, 7, 5, 4) & 0xf3;
				data2 |= (readinputport(2) & 1) ? 0x0 : 0x4;

				return 0xff000000 | data1 << 16 | 0x0000ff00 | data2;
			}
			//default:
			default:
			//ui_popup("%02x PORT SEL",port_sel);
			return (readinputport(2) << 16) | (readinputport(3));
		}
		case 1:
		return (readinputport(4) << 16) | (ioga[1]);
		case 2:
		switch(port_sel)
		{
			case 0x77:	return (readinputport(5) << 16) | (readinputport(6));
			case 0x67:	return 0xffffffff;/**/
			case 0x20:  return 0xffff0000 | (ioga[2] & 0xffff);
			case 0x10:  return ((ioga[2] & 0xffff) << 16) | 0xffff;
			case 0x60:  return 0xffffffff;/**/
			default:
			//ui_popup("offs: 2 %02x",port_sel);
			return 0xffffffff;
		}
		break;
		case 3:
		switch(port_sel)
		{
			case 0x60:  return ((ioga[2] & 0xffff) << 16) | 0xffff;
			default:
			//ui_popup("offs: 3 %02x",port_sel);
			return 0xffffffff;
		}
		break;
		case 6:
		switch(port_sel)
		{
			case 0x60:  return ioga[5];
			default:
			//ui_popup("offs: 6 %02x",port_sel);
			return 0xffffffff;
		}
		break;
		case 7:
		if(LOG_IOGA) logerror("(PC %d=%06x) Warning: READ from PORT_AD\n",cpu_getactivecpu(), activecpu_get_pc());
		ui_popup("Read from PORT_AD");
		i++;
		return port_ad[i & 7];
		default:
		return ioga[offset];
	}
}

WRITE32_HANDLER ( stv_io_w32 )
{
	//if(LOG_IOGA) logerror("(PC=%08X): I/O w %08X = %08X & %08X\n", activecpu_get_pc(), offset*4, data, mem_mask);

	switch(offset)
	{
		case 1:
			if(LO_WORD_ACCESS)
			{
				/*Why does the BIOS tests these as ACTIVE HIGH?A program bug?*/
				ioga[1] = (data) & 0xff;
				coin_counter_w(0,~data & 0x01);
				coin_counter_w(1,~data & 0x02);
				coin_lockout_w(0,~data & 0x04);
				coin_lockout_w(1,~data & 0x08);
				/*
                other bits reserved
                */
			}
		break;
		case 2:
			if(HI_WORD_ACCESS)
			{
				ioga[2] = data >> 16;
				mux_data = ioga[2];
			}
			else if(LO_WORD_ACCESS)
				ioga[2] = data;
		break;
		case 3:
			if(HI_WORD_ACCESS)
				ioga[3] = data;
		break;
		case 4:
			if(HI_WORD_ACCESS)
				port_sel = (data & 0xffff0000) >> 16;
		break;
		case 5:
			if(HI_WORD_ACCESS)
				ioga[5] = data;
		break;
	}
}

/*

SCU Handling

*/

/**********************************************************************************
SCU Register Table
offset,relative address
Registers are in long words.
===================================================================================
0     0000  Level 0 DMA Set Register
1     0004
2     0008
3     000c
4     0010
5     0014
6     0018
7     001c
8     0020  Level 1 DMA Set Register
9     0024
10    0028
11    002c
12    0030
13    0034
14    0038
15    003c
16    0040  Level 2 DMA Set Register
17    0044
18    0048
19    004c
20    0050
21    0054
22    0058
23    005c
24    0060  DMA Forced Stop
25    0064
26    0068
27    006c
28    0070  <Free>
29    0074
30    0078
31    007c  DMA Status Register
32    0080  DSP Program Control Port
33    0084  DSP Program RAM Data Port
34    0088  DSP Data RAM Address Port
35    008c  DSP Data RAM Data Port
36    0090  Timer 0 Compare Register
37    0094  Timer 1 Set Data Register
38    0098  Timer 1 Mode Register
39    009c  <Free>
40    00a0  Interrupt Mask Register
41    00a4  Interrupt Status Register
42    00a8  A-Bus Interrupt Acknowledge
43    00ac  <Free>
44    00b0  A-Bus Set Register
45    00b4
46    00b8  A-Bus Refresh Register
47    00bc  <Free>
48    00c0
49    00c4  SCU SDRAM Select Register
50    00c8  SCU Version Register
51    00cc  <Free>
52    00cf
===================================================================================
DMA Status Register(32-bit):
xxxx xxxx x--- xx-- xx-- xx-- xx-- xx-- UNUSED
---- ---- -x-- ---- ---- ---- ---- ---- DMA DSP-Bus access
---- ---- --x- ---- ---- ---- ---- ---- DMA B-Bus access
---- ---- ---x ---- ---- ---- ---- ---- DMA A-Bus access
---- ---- ---- --x- ---- ---- ---- ---- DMA lv 1 interrupt
---- ---- ---- ---x ---- ---- ---- ---- DMA lv 0 interrupt
---- ---- ---- ---- --x- ---- ---- ---- DMA lv 2 in stand-by
---- ---- ---- ---- ---x ---- ---- ---- DMA lv 2 in operation
---- ---- ---- ---- ---- --x- ---- ---- DMA lv 1 in stand-by
---- ---- ---- ---- ---- ---x ---- ---- DMA lv 1 in operation
---- ---- ---- ---- ---- ---- --x- ---- DMA lv 0 in stand-by
---- ---- ---- ---- ---- ---- ---x ---- DMA lv 0 in operation
---- ---- ---- ---- ---- ---- ---- --x- DSP side DMA in stand-by
---- ---- ---- ---- ---- ---- ---- ---x DSP side DMA in operation

**********************************************************************************/
/*
DMA TODO:
-Verify if there are any kind of bugs,do clean-ups,use better comments
 and macroize for better reading...
-Add timings(but how fast are each DMA?).
-Add level priority & DMA status register.
-Add DMA start factor conditions that are different than 7.
-Add byte data type transfer.
-Set boundaries.
*/

#define DIRECT_MODE(_lv_)			(!(stv_scu[5+(_lv_*8)] & 0x01000000))
#define INDIRECT_MODE(_lv_)			  (stv_scu[5+(_lv_*8)] & 0x01000000)
#define DRUP(_lv_)					  (stv_scu[5+(_lv_*8)] & 0x00010000)
#define DWUP(_lv_)                    (stv_scu[5+(_lv_*8)] & 0x00000100)

#define DMA_STATUS				(stv_scu[31])
/*These macros sets the various DMA status flags.*/
#define D0MV_1	if(!(DMA_STATUS & 0x10))    DMA_STATUS^=0x10
#define D1MV_1	if(!(DMA_STATUS & 0x100))   DMA_STATUS^=0x100
#define D2MV_1	if(!(DMA_STATUS & 0x1000))  DMA_STATUS^=0x1000
#define D0MV_0	if(DMA_STATUS & 0x10) 	    DMA_STATUS^=0x10
#define D1MV_0	if(DMA_STATUS & 0x100) 	    DMA_STATUS^=0x100
#define D2MV_0	if(DMA_STATUS & 0x1000)     DMA_STATUS^=0x1000

UINT32 scu_index_0,scu_index_1,scu_index_2;
static UINT8 scsp_to_main_irq;

static UINT32 scu_add_tmp;

/*For area checking*/
#define ABUS(_lv_)       ((scu_##_lv_ & 0x07ffffff) >= 0x02000000) && ((scu_##_lv_ & 0x07ffffff) <= 0x04ffffff)
#define BBUS(_lv_)       ((scu_##_lv_ & 0x07ffffff) >= 0x05a00000) && ((scu_##_lv_ & 0x07ffffff) <= 0x05ffffff)
#define VDP1_REGS(_lv_)  ((scu_##_lv_ & 0x07ffffff) >= 0x05d00000) && ((scu_##_lv_ & 0x07ffffff) <= 0x05dfffff)
#define VDP2(_lv_)       ((scu_##_lv_ & 0x07ffffff) >= 0x05e00000) && ((scu_##_lv_ & 0x07ffffff) <= 0x05fdffff)
#define WORK_RAM_L(_lv_) ((scu_##_lv_ & 0x07ffffff) >= 0x00200000) && ((scu_##_lv_ & 0x07ffffff) <= 0x002fffff)
#define WORK_RAM_H(_lv_) ((scu_##_lv_ & 0x07ffffff) >= 0x06000000) && ((scu_##_lv_ & 0x07ffffff) <= 0x060fffff)
#define SOUND_RAM(_lv_)  ((scu_##_lv_ & 0x07ffffff) >= 0x05a00000) && ((scu_##_lv_ & 0x07ffffff) <= 0x05afffff)

READ32_HANDLER( stv_scu_r32 )
{
	/*TODO: write only registers must return 0...*/
	//ui_popup("%02x",DMA_STATUS);
	//if (offset == 23)
	//{
		//Super Major League reads here???
	//}
	if (offset == 31)
	{
		if(LOG_SCU) logerror("(PC=%08x) DMA status reg read\n",activecpu_get_pc());
		return stv_scu[offset];
	}
	else if ( offset == 35 )
	{
        if(LOG_SCU) logerror( "DSP mem read at %08X\n", stv_scu[34]);
        return dsp_ram_addr_r();
    }
    else if( offset == 41)
    {
		logerror("(PC=%08x) IRQ status reg read\n",activecpu_get_pc());
		/*TODO:for now we're activating everything here,but we need to return the proper active irqs*/
		return 0xffffffff;
	}
	else if( offset == 50 )
	{
		logerror("(PC=%08x) SCU version reg read\n",activecpu_get_pc());
		return 0x00000000;/*SCU Version 0*/
	}
    else
    {
    	if(LOG_SCU) logerror("(PC=%08x) SCU reg read at %d = %08x\n",activecpu_get_pc(),offset,stv_scu[offset]);
    	return stv_scu[offset];
   	}
}

WRITE32_HANDLER( stv_scu_w32 )
{
	COMBINE_DATA(&stv_scu[offset]);

	switch(offset)
	{
		/*LV 0 DMA*/
		case 0:	scu_src_0  = ((stv_scu[0] & 0x07ffffff) >> 0); break;
		case 1:	scu_dst_0  = ((stv_scu[1] & 0x07ffffff) >> 0); break;
		case 2: scu_size_0 = ((stv_scu[2] & 0x000fffff) >> 0); break;
		case 3:
			/*Read address add value for DMA lv 0*/
			if(stv_scu[3] & 0x100)
				scu_src_add_0 = 4;
			else
				scu_src_add_0 = 1;

			/*Write address add value for DMA lv 0*/
			switch(stv_scu[3] & 7)
			{
				case 0: scu_dst_add_0 = 2;   break;
				case 1: scu_dst_add_0 = 4;   break;
				case 2: scu_dst_add_0 = 8;   break;
				case 3: scu_dst_add_0 = 16;  break;
				case 4: scu_dst_add_0 = 32;  break;
				case 5: scu_dst_add_0 = 64;  break;
				case 6: scu_dst_add_0 = 128; break;
				case 7: scu_dst_add_0 = 256; break;
			}
			break;
		case 4:
/*
-stv_scu[4] bit 0 is DMA starting bit.
    Used when the start factor is 7.Toggle after execution.
-stv_scu[4] bit 8 is DMA Enable bit.
    This is an execution mask flag.
-stv_scu[5] bit 0,bit 1 and bit 2 is DMA starting factor.
    It must be 7 for this specific condition.
-stv_scu[5] bit 24 is Indirect Mode/Direct Mode (0/1).
*/
		if(stv_scu[4] & 1 && ((stv_scu[5] & 7) == 7) && stv_scu[4] & 0x100)
		{
			if(DIRECT_MODE(0)) { dma_direct_lv0(); }
			else			   { dma_indirect_lv0(); }

			stv_scu[4]^=1;//disable starting bit.

			/*Sound IRQ*/
			if(/*(!(stv_scu[40] & 0x40)) &&*/ scsp_to_main_irq == 1)
			{
				//cpunum_set_input_line_and_vector(0, 9, HOLD_LINE , 0x46);
				logerror("SCSP: Main CPU interrupt\n");
				#if 0
				if((scu_dst_0 & 0x7ffffff) != 0x05a00000)
				{
					if(!(stv_scu[40] & 0x1000))
					{
						cpunum_set_input_line_and_vector(0, 3, HOLD_LINE, 0x4c);
						logerror("SCU: Illegal DMA interrupt\n");
					}
				}
				#endif
			}
		}
		break;
		case 5:
		if(INDIRECT_MODE(0))
		{
			if(LOG_SCU) logerror("Indirect Mode DMA lv 0 set\n");
			if(!DWUP(0)) scu_index_0 = scu_dst_0;
		}

		/*Start factor enable bits,bit 2,bit 1 and bit 0*/
		if((stv_scu[5] & 7) != 7)
			if(LOG_SCU) logerror("Start factor chosen for lv 0 = %d\n",stv_scu[5] & 7);
		break;
		/*LV 1 DMA*/
		case 8:	 scu_src_1  = ((stv_scu[8] &  0x07ffffff) >> 0);  break;
		case 9:	 scu_dst_1  = ((stv_scu[9] &  0x07ffffff) >> 0);  break;
		case 10: scu_size_1 = ((stv_scu[10] & 0x00001fff) >> 0);  break;
		case 11:
		/*Read address add value for DMA lv 1*/
		if(stv_scu[11] & 0x100)
			scu_src_add_1 = 4;
		else
			scu_src_add_1 = 1;

		/*Write address add value for DMA lv 1*/
		switch(stv_scu[11] & 7)
		{
			case 0: scu_dst_add_1 = 2;   break;
			case 1: scu_dst_add_1 = 4;   break;
			case 2: scu_dst_add_1 = 8;   break;
			case 3: scu_dst_add_1 = 16;  break;
			case 4: scu_dst_add_1 = 32;  break;
			case 5: scu_dst_add_1 = 64;  break;
			case 6: scu_dst_add_1 = 128; break;
			case 7: scu_dst_add_1 = 256; break;
		}
		break;
		case 12:
		if(stv_scu[12] & 1 && ((stv_scu[13] & 7) == 7) && stv_scu[12] & 0x100)
		{
			if(DIRECT_MODE(1)) { dma_direct_lv1(); }
			else			   { dma_indirect_lv1(); }

			stv_scu[12]^=1;

			/*Sound IRQ*/
			if(/*(!(stv_scu[40] & 0x40)) &&*/ scsp_to_main_irq == 1)
			{
				//cpunum_set_input_line_and_vector(0, 9, HOLD_LINE , 0x46);
				logerror("SCSP: Main CPU interrupt\n");
			}
		}
		break;
		case 13:
		if(INDIRECT_MODE(1))
		{
			if(LOG_SCU) logerror("Indirect Mode DMA lv 1 set\n");
			if(!DWUP(1)) scu_index_1 = scu_dst_1;
		}

		if((stv_scu[13] & 7) != 7)
			if(LOG_SCU) logerror("Start factor chosen for lv 1 = %d\n",stv_scu[13] & 7);
		break;
		/*LV 2 DMA*/
		case 16: scu_src_2  = ((stv_scu[16] & 0x07ffffff) >> 0);  break;
		case 17: scu_dst_2  = ((stv_scu[17] & 0x07ffffff) >> 0);  break;
		case 18: scu_size_2 = ((stv_scu[18] & 0x00001fff) >> 0);  break;
		case 19:
		/*Read address add value for DMA lv 2*/
		if(stv_scu[19] & 0x100)
			scu_src_add_2 = 4;
		else
			scu_src_add_2 = 1;

		/*Write address add value for DMA lv 2*/
		switch(stv_scu[19] & 7)
		{
			case 0: scu_dst_add_2 = 2;   break;
			case 1: scu_dst_add_2 = 4;   break;
			case 2: scu_dst_add_2 = 8;   break;
			case 3: scu_dst_add_2 = 16;  break;
			case 4: scu_dst_add_2 = 32;  break;
			case 5: scu_dst_add_2 = 64;  break;
			case 6: scu_dst_add_2 = 128; break;
			case 7: scu_dst_add_2 = 256; break;
		}
		break;
		case 20:
		if(stv_scu[20] & 1 && ((stv_scu[21] & 7) == 7) && stv_scu[20] & 0x100)
		{
			if(DIRECT_MODE(2)) { dma_direct_lv2(); }
			else			   { dma_indirect_lv2(); }

			stv_scu[20]^=1;

			/*Sound IRQ*/
			if(/*(!(stv_scu[40] & 0x40)) &&*/ scsp_to_main_irq == 1)
			{
				//cpunum_set_input_line_and_vector(0, 9, HOLD_LINE , 0x46);
				logerror("SCSP: Main CPU interrupt\n");
			}
		}
		break;
		case 21:
		if(INDIRECT_MODE(2))
		{
			if(LOG_SCU) logerror("Indirect Mode DMA lv 2 set\n");
			if(!DWUP(2)) scu_index_2 = scu_dst_2;
		}

		if((stv_scu[21] & 7) != 7)
			if(LOG_SCU) logerror("Start factor chosen for lv 2 = %d\n",stv_scu[21] & 7);
		break;
		case 24:
		if(LOG_SCU) logerror("DMA Forced Stop Register set = %02x\n",stv_scu[24]);
		break;
		case 31: if(LOG_SCU) logerror("Warning: DMA status WRITE! Offset %02x(%d)\n",offset*4,offset); break;
		/*DSP section*/
		/*Use functions so it is easier to work out*/
		case 32:
		dsp_prg_ctrl(data);
		if(LOG_SCU) logerror("SCU DSP: Program Control Port Access %08x\n",data);
		break;
		case 33:
		dsp_prg_data(data);
		if(LOG_SCU) logerror("SCU DSP: Program RAM Data Port Access %08x\n",data);
		break;
		case 34:
		dsp_ram_addr_ctrl(data);
		if(LOG_SCU) logerror("SCU DSP: Data RAM Address Port Access %08x\n",data);
		break;
		case 35:
		dsp_ram_addr_w(data);
		if(LOG_SCU) logerror("SCU DSP: Data RAM Data Port Access %08x\n",data);
		break;
		case 36: if(LOG_SCU) logerror("timer 0 compare data = %03x\n",stv_scu[36]);break;
		case 37: if(LOG_SCU) logerror("timer 1 set data = %08x\n",stv_scu[37]); break;
		case 38: if(LOG_SCU) logerror("timer 1 mode data = %08x\n",stv_scu[38]); break;
		case 40:
		/*An interrupt is masked when his specific bit is 1.*/
		/*Are bit 16-bit 31 for External A-Bus irq mask like the status register?*/
		/*What is 0x00000080 setting?Is it valid?*/

		/*Take out the common settings to keep logging quiet.*/
		if(stv_scu[40] != 0xfffffffe &&
		   stv_scu[40] != 0xfffffffc &&
		   stv_scu[40] != 0xffffffff)
		{
			if(LOG_SCU) logerror("cpu #%d (PC=%08X) IRQ mask reg set %08x = %d%d%d%d|%d%d%d%d|%d%d%d%d|%d%d%d%d\n",
			cpu_getactivecpu(), activecpu_get_pc(),
			stv_scu[offset],
			stv_scu[offset] & 0x8000 ? 1 : 0, /*A-Bus irq*/
			stv_scu[offset] & 0x4000 ? 1 : 0, /*<reserved>*/
			stv_scu[offset] & 0x2000 ? 1 : 0, /*Sprite draw end irq(VDP1)*/
			stv_scu[offset] & 0x1000 ? 1 : 0, /*Illegal DMA irq*/
			stv_scu[offset] & 0x0800 ? 1 : 0, /*Lv 0 DMA end irq*/
			stv_scu[offset] & 0x0400 ? 1 : 0, /*Lv 1 DMA end irq*/
			stv_scu[offset] & 0x0200 ? 1 : 0, /*Lv 2 DMA end irq*/
			stv_scu[offset] & 0x0100 ? 1 : 0, /*PAD irq*/
			stv_scu[offset] & 0x0080 ? 1 : 0, /*System Manager(SMPC) irq*/
			stv_scu[offset] & 0x0040 ? 1 : 0, /*Snd req*/
			stv_scu[offset] & 0x0020 ? 1 : 0, /*DSP irq end*/
			stv_scu[offset] & 0x0010 ? 1 : 0, /*Timer 1 irq*/
			stv_scu[offset] & 0x0008 ? 1 : 0, /*Timer 0 irq*/
			stv_scu[offset] & 0x0004 ? 1 : 0, /*HBlank-IN*/
			stv_scu[offset] & 0x0002 ? 1 : 0, /*VBlank-OUT*/
			stv_scu[offset] & 0x0001 ? 1 : 0);/*VBlank-IN*/
		}
		break;
		case 41:
		/*This is r/w by introdon...*/
		if(LOG_SCU) logerror("IRQ status reg set:%08x\n",stv_scu[41]);
		break;
		case 42: if(LOG_SCU) logerror("A-Bus IRQ ACK %08x\n",stv_scu[42]); break;
		case 49: if(LOG_SCU) logerror("SCU SDRAM set: %02x\n",stv_scu[49]); break;
		default: if(LOG_SCU) logerror("Warning: unused SCU reg set %d = %08x\n",offset,data);
	}
}

static void dma_direct_lv0()
{
	static UINT32 tmp_src,tmp_dst,tmp_size;
	if(LOG_SCU) logerror("DMA lv 0 transfer START\n"
			             "Start %08x End %08x Size %04x\n",scu_src_0,scu_dst_0,scu_size_0);
	if(LOG_SCU) logerror("Start Add %04x Destination Add %04x\n",scu_src_add_0,scu_dst_add_0);

	D0MV_1;

	if(scu_size_0 == 0) scu_size_0 = 0x00100000;

	scsp_to_main_irq = 0;

	/*set here the boundaries checks*/
	/*...*/

	if(SOUND_RAM(dst_0))
	{
		logerror("Sound RAM DMA write\n");
		scsp_to_main_irq = 1;
	}

	if((scu_dst_add_0 != scu_src_add_0) && (ABUS(src_0)))
	{
		logerror("A-Bus invalid transfer,sets to default\n");
		scu_add_tmp = (scu_dst_add_0*0x100) | (scu_src_add_0);
		scu_dst_add_0 = scu_src_add_0 = 4;
		scu_add_tmp |= 0x80000000;
	}
	/*Let me know if you encounter any of these three*/
	if(ABUS(dst_0))
	{
		logerror("A-Bus invalid write\n");
		/*...*/
	}
	if(WORK_RAM_L(dst_0))
	{
		logerror("WorkRam-L invalid write\n");
		/*...*/
	}
	if(VDP2(src_0))
	{
		logerror("VDP-2 invalid read\n");
		/*...*/
	}
	if(VDP1_REGS(dst_0))
	{
		logerror("VDP1 register access,must be in word units\n");
		scu_add_tmp = (scu_dst_add_0*0x100) | (scu_src_add_0);
		scu_dst_add_0 = scu_src_add_0 = 2;
		scu_add_tmp |= 0x80000000;
	}
	if(DRUP(0))
	{
		logerror("Data read update = 1,read address add value must be 1 too\n");
		scu_add_tmp = (scu_dst_add_0*0x100) | (scu_src_add_0);
		scu_src_add_0 = 4;
		scu_add_tmp |= 0x80000000;
	}

	if (WORK_RAM_H(dst_0) && (scu_dst_add_0 != 4))
	{
		scu_add_tmp = (scu_dst_add_0*0x100) | (scu_src_add_0);
		scu_dst_add_0 = 4;
		scu_add_tmp |= 0x80000000;
	}

	tmp_size = scu_size_0;
	if(!(DRUP(0))) tmp_src = scu_src_0;
	if(!(DWUP(0))) tmp_dst = scu_dst_0;

	for (; scu_size_0 > 0; scu_size_0-=scu_dst_add_0)
	{
		if(scu_dst_add_0 == 2)
			program_write_word(scu_dst_0,program_read_word(scu_src_0));
		else if(scu_dst_add_0 == 8)
		{
			program_write_word(scu_dst_0,program_read_word(scu_src_0));
			program_write_word(scu_dst_0+2,program_read_word(scu_src_0));
			program_write_word(scu_dst_0+4,program_read_word(scu_src_0+2));
			program_write_word(scu_dst_0+6,program_read_word(scu_src_0+2));
		}
		else
		{
			program_write_word(scu_dst_0,program_read_word(scu_src_0));
			program_write_word(scu_dst_0+2,program_read_word(scu_src_0+2));
		}

		scu_dst_0+=scu_dst_add_0;
		scu_src_0+=scu_src_add_0;
	}

	scu_size_0 = tmp_size;
	if(!(DRUP(0))) scu_src_0 = tmp_src;
	if(!(DWUP(0))) scu_dst_0 = tmp_dst;

	if(LOG_SCU) logerror("DMA transfer END\n");
	if(!(stv_scu[40] & 0x800))/*Lv 0 DMA end irq*/
		cpunum_set_input_line_and_vector(0, 5, HOLD_LINE , 0x4b);

	if(scu_add_tmp & 0x80000000)
	{
		scu_dst_add_0 = (scu_add_tmp & 0xff00) >> 8;
		scu_src_add_0 = (scu_add_tmp & 0x00ff) >> 0;
		scu_add_tmp^=0x80000000;
	}

	D0MV_0;
}

static void dma_direct_lv1()
{
	static UINT32 tmp_src,tmp_dst,tmp_size;
	if(LOG_SCU) logerror("DMA lv 1 transfer START\n"
			 "Start %08x End %08x Size %04x\n",scu_src_1,scu_dst_1,scu_size_1);
	if(LOG_SCU) logerror("Start Add %04x Destination Add %04x\n",scu_src_add_1,scu_dst_add_1);

	D1MV_1;

	if(scu_size_1 == 0) scu_size_1 = 0x00002000;

	scsp_to_main_irq = 0;

	/*set here the boundaries checks*/
	/*...*/

	if(SOUND_RAM(dst_1))
	{
		logerror("Sound RAM DMA write\n");
		scsp_to_main_irq = 1;
	}

	if((scu_dst_add_1 != scu_src_add_1) && (ABUS(src_1)))
	{
		logerror("A-Bus invalid transfer,sets to default\n");
		scu_add_tmp = (scu_dst_add_1*0x100) | (scu_src_add_1);
		scu_dst_add_1 = scu_src_add_1 = 4;
		scu_add_tmp |= 0x80000000;
	}
	/*Let me know if you encounter any of these ones*/
	if(ABUS(dst_1))
	{
		logerror("A-Bus invalid write\n");
		/*...*/
	}
	if(WORK_RAM_L(dst_1))
	{
		logerror("WorkRam-L invalid write\n");
		/*...*/
	}
	if(VDP1_REGS(dst_1))
	{
		logerror("VDP1 register access,must be in word units\n");
		scu_add_tmp = (scu_dst_add_1*0x100) | (scu_src_add_1);
		scu_dst_add_1 = scu_src_add_1 = 2;
		scu_add_tmp |= 0x80000000;
	}
	if(VDP2(src_1))
	{
		logerror("VDP-2 invalid read\n");
		/*...*/
	}
	if(DRUP(1))
	{
		logerror("Data read update = 1,read address add value must be 1 too\n");
		scu_add_tmp = (scu_dst_add_1*0x100) | (scu_src_add_1);
		scu_src_add_1 = 4;
		scu_add_tmp |= 0x80000000;
	}

	if (WORK_RAM_H(dst_1) && (scu_dst_add_1 != 4))
	{
		scu_add_tmp = (scu_dst_add_1*0x100) | (scu_src_add_1);
		scu_src_add_1 = 4;
		scu_add_tmp |= 0x80000000;
	}

	tmp_size = scu_size_1;
	if(!(DRUP(1))) tmp_src = scu_src_1;
	if(!(DWUP(1))) tmp_dst = scu_dst_1;

	for (; scu_size_1 > 0; scu_size_1-=scu_dst_add_1)
	{
		if(scu_dst_add_1 == 2)
			program_write_word(scu_dst_1,program_read_word(scu_src_1));
		else
		{
			program_write_word(scu_dst_1,program_read_word(scu_src_1));
			program_write_word(scu_dst_1+2,program_read_word(scu_src_1+2));
		}

		scu_dst_1+=scu_dst_add_1;
		scu_src_1+=scu_src_add_1;
	}

	scu_size_1 = tmp_size;
	if(!(DRUP(1))) scu_src_1 = tmp_src;
	if(!(DWUP(1))) scu_dst_1 = tmp_dst;

	if(LOG_SCU) logerror("DMA transfer END\n");
	if(!(stv_scu[40] & 0x400))/*Lv 1 DMA end irq*/
		cpunum_set_input_line_and_vector(0, 6, HOLD_LINE , 0x4a);

	if(scu_add_tmp & 0x80000000)
	{
		scu_dst_add_1 = (scu_add_tmp & 0xff00) >> 8;
		scu_src_add_1 = (scu_add_tmp & 0x00ff) >> 0;
		scu_add_tmp^=0x80000000;
	}

	D1MV_0;
}

static void dma_direct_lv2()
{
	static UINT32 tmp_src,tmp_dst,tmp_size;
	if(LOG_SCU) logerror("DMA lv 2 transfer START\n"
			 "Start %08x End %08x Size %04x\n",scu_src_2,scu_dst_2,scu_size_2);
	if(LOG_SCU) logerror("Start Add %04x Destination Add %04x\n",scu_src_add_2,scu_dst_add_2);

	D2MV_1;

	scsp_to_main_irq = 0;

	if(scu_size_2 == 0) scu_size_2 = 0x00002000;

	/*set here the boundaries checks*/
	/*...*/

	if(SOUND_RAM(dst_2))
	{
		logerror("Sound RAM DMA write\n");
		scsp_to_main_irq = 1;
	}

	if((scu_dst_add_2 != scu_src_add_2) && (ABUS(src_2)))
	{
		logerror("A-Bus invalid transfer,sets to default\n");
		scu_add_tmp = (scu_dst_add_2*0x100) | (scu_src_add_2);
		scu_dst_add_2 = scu_src_add_2 = 4;
		scu_add_tmp |= 0x80000000;
	}
	/*Let me know if you encounter any of these ones*/
	if(ABUS(dst_2))
	{
		logerror("A-Bus invalid write\n");
		/*...*/
	}
	if(WORK_RAM_L(dst_2))
	{
		logerror("WorkRam-L invalid write\n");
		/*...*/
	}
	if(VDP1_REGS(dst_2))
	{
		logerror("VDP1 register access,must be in word units\n");
		scu_add_tmp = (scu_dst_add_2*0x100) | (scu_src_add_2);
		scu_dst_add_2 = scu_src_add_2 = 2;
		scu_add_tmp |= 0x80000000;
	}
	if(VDP2(src_2))
	{
		logerror("VDP-2 invalid read\n");
		/*...*/
	}
	if(DRUP(2))
	{
		logerror("Data read update = 1,read address add value must be 1 too\n");
		scu_add_tmp = (scu_dst_add_2*0x100) | (scu_src_add_2);
		scu_src_add_2 = 4;
		scu_add_tmp |= 0x80000000;
	}

	if (WORK_RAM_H(dst_2) && (scu_dst_add_2 != 4))
	{
		scu_add_tmp = (scu_dst_add_2*0x100) | (scu_src_add_2);
		scu_src_add_2 = 4;
		scu_add_tmp |= 0x80000000;
	}

	tmp_size = scu_size_2;
	if(!(DRUP(2))) tmp_src = scu_src_2;
	if(!(DWUP(2))) tmp_dst = scu_dst_2;

	for (; scu_size_2 > 0; scu_size_2-=scu_dst_add_2)
	{
		if(scu_dst_add_2 == 2)
			program_write_word(scu_dst_2,program_read_word(scu_src_2));
		else
		{
			program_write_word(scu_dst_2,program_read_word(scu_src_2));
			program_write_word(scu_dst_2+2,program_read_word(scu_src_2+2));
		}

		scu_dst_2+=scu_dst_add_2;
		scu_src_2+=scu_src_add_2;
	}

	scu_size_2 = tmp_size;
	if(!(DRUP(2))) scu_src_2 = tmp_src;
	if(!(DWUP(2))) scu_dst_2 = tmp_dst;

	if(LOG_SCU) logerror("DMA transfer END\n");
	if(!(stv_scu[40] & 0x200))/*Lv 2 DMA end irq*/
		cpunum_set_input_line_and_vector(0, 6, HOLD_LINE , 0x49);

	if(scu_add_tmp & 0x80000000)
	{
		scu_dst_add_2 = (scu_add_tmp & 0xff00) >> 8;
		scu_src_add_2 = (scu_add_tmp & 0x00ff) >> 0;
		scu_add_tmp^=0x80000000;
	}

	D2MV_0;
}

static void dma_indirect_lv0()
{
	/*Helper to get out of the cycle*/
	UINT8 job_done = 0;
	/*temporary storage for the transfer data*/
	UINT32 tmp_src;

	D0MV_1;

	scsp_to_main_irq = 0;

	if(scu_index_0 == 0) { scu_index_0 = scu_dst_0; }

	do{
		tmp_src = scu_index_0;

		/*Thanks for Runik of Saturnin for pointing this out...*/
		scu_size_0 = program_read_dword(scu_index_0);
		scu_src_0 =  program_read_dword(scu_index_0+8);
		scu_dst_0 =  program_read_dword(scu_index_0+4);

		/*Indirect Mode end factor*/
		if(scu_src_0 & 0x80000000)
			job_done = 1;

		if(SOUND_RAM(dst_0))
		{
			logerror("Sound RAM DMA write\n");
			scsp_to_main_irq = 1;
		}

		if(LOG_SCU) logerror("DMA lv 0 indirect mode transfer START\n"
			 	 "Start %08x End %08x Size %04x\n",scu_src_0,scu_dst_0,scu_size_0);
		if(LOG_SCU) logerror("Start Add %04x Destination Add %04x\n",scu_src_add_0,scu_dst_add_0);

		//guess,but I believe it's right.
		scu_src_0 &=0x07ffffff;
		scu_dst_0 &=0x07ffffff;
		scu_size_0 &=0xfffff;

		for (; scu_size_0 > 0; scu_size_0-=scu_dst_add_0)
		{
			if(scu_dst_add_0 == 2)
				program_write_word(scu_dst_0,program_read_word(scu_src_0));
			else
			{
				/* some games, eg columns97 are a bit weird, I'm not sure this is correct
                  they start a dma on a 2 byte boundary in 4 byte add mode, using the dword reads we
                  can't access 2 byte boundaries, and the end of the sprite list never gets marked,
                  the length of the transfer is also set to a 2 byte boundary, maybe the add values
                  should be different, I don't know */
				program_write_word(scu_dst_0,program_read_word(scu_src_0));
				program_write_word(scu_dst_0+2,program_read_word(scu_src_0+2));
			}
			scu_dst_0+=scu_dst_add_0;
			scu_src_0+=scu_src_add_0;
		}

		//if(DRUP(0))   program_write_dword(tmp_src+8,scu_src_0|job_done ? 0x80000000 : 0);
		//if(DWUP(0)) program_write_dword(tmp_src+4,scu_dst_0);

		scu_index_0 = tmp_src+0xc;

	}while(job_done == 0);

	if(!(stv_scu[40] & 0x800))/*Lv 0 DMA end irq*/
		cpunum_set_input_line_and_vector(0, 5, HOLD_LINE , 0x4b);

	D0MV_0;
}

static void dma_indirect_lv1()
{
	/*Helper to get out of the cycle*/
	UINT8 job_done = 0;
	/*temporary storage for the transfer data*/
	UINT32 tmp_src;

	D1MV_1;

	scsp_to_main_irq = 0;

	if(scu_index_1 == 0) { scu_index_1 = scu_dst_1; }

	do{
		tmp_src = scu_index_1;

		scu_size_1 = program_read_dword(scu_index_1);
		scu_src_1 =  program_read_dword(scu_index_1+8);
		scu_dst_1 =  program_read_dword(scu_index_1+4);

		/*Indirect Mode end factor*/
		if(scu_src_1 & 0x80000000)
			job_done = 1;

		if(SOUND_RAM(dst_1))
		{
			logerror("Sound RAM DMA write\n");
			scsp_to_main_irq = 1;
		}

		if(LOG_SCU) logerror("DMA lv 1 indirect mode transfer START\n"
			 	 "Start %08x End %08x Size %04x\n",scu_src_1,scu_dst_1,scu_size_1);
		if(LOG_SCU) logerror("Start Add %04x Destination Add %04x\n",scu_src_add_1,scu_dst_add_1);

		//guess,but I believe it's right.
		scu_src_1 &=0x07ffffff;
		scu_dst_1 &=0x07ffffff;
		scu_size_1 &=0xffff;


		for (; scu_size_1 > 0; scu_size_1-=scu_dst_add_1)
		{

			if(scu_dst_add_1 == 2)
				program_write_word(scu_dst_1,program_read_word(scu_src_1));
			else
			{
				/* some games, eg columns97 are a bit weird, I'm not sure this is correct
                  they start a dma on a 2 byte boundary in 4 byte add mode, using the dword reads we
                  can't access 2 byte boundaries, and the end of the sprite list never gets marked,
                  the length of the transfer is also set to a 2 byte boundary, maybe the add values
                  should be different, I don't know */
				program_write_word(scu_dst_1,program_read_word(scu_src_1));
				program_write_word(scu_dst_1+2,program_read_word(scu_src_1+2));
			}
			scu_dst_1+=scu_dst_add_1;
			scu_src_1+=scu_src_add_1;
		}

		//if(DRUP(1))   program_write_dword(tmp_src+8,scu_src_1|job_done ? 0x80000000 : 0);
		//if(DWUP(1)) program_write_dword(tmp_src+4,scu_dst_1);

		scu_index_1 = tmp_src+0xc;

	}while(job_done == 0);

	if(!(stv_scu[40] & 0x400))/*Lv 1 DMA end irq*/
		cpunum_set_input_line_and_vector(0, 6, HOLD_LINE , 0x4a);

	D1MV_0;
}

static void dma_indirect_lv2()
{
	/*Helper to get out of the cycle*/
	UINT8 job_done = 0;
	/*temporary storage for the transfer data*/
	UINT32 tmp_src;

	D2MV_1;

	scsp_to_main_irq = 0;

	if(scu_index_2 == 0) { scu_index_2 = scu_dst_2; }

	do{
		tmp_src = scu_index_2;

		scu_size_2 = program_read_dword(scu_index_2);
		scu_src_2 =  program_read_dword(scu_index_2+8);
		scu_dst_2 =  program_read_dword(scu_index_2+4);

		/*Indirect Mode end factor*/
		if(scu_src_2 & 0x80000000)
			job_done = 1;

		if(SOUND_RAM(dst_2))
		{
			logerror("Sound RAM DMA write\n");
			scsp_to_main_irq = 1;
		}

		if(LOG_SCU) logerror("DMA lv 2 indirect mode transfer START\n"
			 	 "Start %08x End %08x Size %04x\n",scu_src_2,scu_dst_2,scu_size_2);
		if(LOG_SCU) logerror("Start Add %04x Destination Add %04x\n",scu_src_add_2,scu_dst_add_2);

		//guess,but I believe it's right.
		scu_src_2 &=0x07ffffff;
		scu_dst_2 &=0x07ffffff;
		scu_size_2 &=0xffff;

		for (; scu_size_2 > 0; scu_size_2-=scu_dst_add_2)
		{
			if(scu_dst_add_2 == 2)
				program_write_word(scu_dst_2,program_read_word(scu_src_2));
			else
			{
				/* some games, eg columns97 are a bit weird, I'm not sure this is correct
                  they start a dma on a 2 byte boundary in 4 byte add mode, using the dword reads we
                  can't access 2 byte boundaries, and the end of the sprite list never gets marked,
                  the length of the transfer is also set to a 2 byte boundary, maybe the add values
                  should be different, I don't know */
				program_write_word(scu_dst_2,program_read_word(scu_src_2));
				program_write_word(scu_dst_2+2,program_read_word(scu_src_2+2));
			}

			scu_dst_2+=scu_dst_add_2;
			scu_src_2+=scu_src_add_2;
		}

		//if(DRUP(2))   program_write_dword(tmp_src+8,scu_src_2|job_done ? 0x80000000 : 0);
		//if(DWUP(2)) program_write_dword(tmp_src+4,scu_dst_2);

		scu_index_2 = tmp_src+0xc;

	}while(job_done == 0);

	if(!(stv_scu[40] & 0x200))/*Lv 2 DMA end irq*/
		cpunum_set_input_line_and_vector(0, 6, HOLD_LINE , 0x49);

	D2MV_0;
}


/**************************************************************************************/

WRITE32_HANDLER( stv_sh2_soundram_w )
{
	COMBINE_DATA(sound_ram+offset*2+1);
	data >>= 16;
	mem_mask >>= 16;
	COMBINE_DATA(sound_ram+offset*2);
}

READ32_HANDLER( stv_sh2_soundram_r )
{
	return (sound_ram[offset*2]<<16)|sound_ram[offset*2+1];
}

static READ32_HANDLER( stv_scsp_regs_r32 )
{
	offset <<= 1;
	return (SCSP_0_r(offset+1, 0xffff) | (SCSP_0_r(offset, 0xffff)<<16));
}

static WRITE32_HANDLER( stv_scsp_regs_w32 )
{
	offset <<= 1;
	SCSP_0_w(offset, data>>16, mem_mask >> 16);
	SCSP_0_w(offset+1, data, mem_mask);
}

/* communication,SLAVE CPU acquires data from the MASTER CPU and triggers an irq.  *
 * Enter into Radiant Silver Gun specific menu for a test...                       */
static WRITE32_HANDLER( minit_w )
{
	logerror("cpu #%d (PC=%08X) MINIT write = %08x\n",cpu_getactivecpu(), activecpu_get_pc(),data);
	cpu_boost_interleave(minit_boost_timeslice, TIME_IN_USEC(minit_boost));
	cpu_trigger(1000);
	cpunum_set_info_int(1, CPUINFO_INT_SH2_FRT_INPUT, PULSE_LINE);
}

static WRITE32_HANDLER( sinit_w )
{
	logerror("cpu #%d (PC=%08X) SINIT write = %08x\n",cpu_getactivecpu(), activecpu_get_pc(),data);
	cpu_boost_interleave(sinit_boost_timeslice, TIME_IN_USEC(sinit_boost));
	cpunum_set_info_int(0, CPUINFO_INT_SH2_FRT_INPUT, PULSE_LINE);
}

/****************************************************************************************

Protection & cartridge handling

*****************************************************************************************

These are the known ST-V games that uses this area as a valid protection,I have written
the data used by the games in the various circumstances for reference:
-Astra Super Stars [astrass]
 [0]        [1]        [2]        [3]
 0x000y0000 0x00000000 0x06130027 0x01230000 test mode,char transfer (3)
 0x???????? 0x???????? 0x???????? 0x???????? attract mode
 0x000y0000 0x00000000 0x06130027 0x01230000 gameplay,char transfer (3)

-Elan Doree : Legend of Dragon [elandore]
 [0]        [1]        [2]        [3]
 No protection                               test mode
 No protection                               attract mode
 0x000y0000 0x00000000 0xe69000f9 0xff7f0000 gameplay,VDP-1 write (textures on humans)

-Final Fight Revenge [ffreveng]
 [0]        [1]        [2]        [3]
 0x000y0000 0x00000000 0x4bcc0013 0x10da0000 test mode,boot vectors at $06080000
 0x000y0000 0x00000000 0x0b780013 0x10d70000 attract mode,boot vectors at $06080000
 0x???????? 0x???????? 0x???????? 0x???????? gameplay

-Radiant Silvergun [rsgun]
 [0]        [1]        [2]        [3]
 No protection                               test mode
 0x000y0000 0x00000000 0x08000010 0x77770000 attract mode,work ram-h $60ff1ec and so on (1)
 0x???????? 0x???????? 0x???????? 0x???????? gameplay

-Steep Slope Sliders [sss]
 [0]        [1]        [2]        [3]
 No protection                               test mode
*0x000y0000 0x00000000 0x000000a6 0x2c5b0000 attract mode,VDP-1 write
*0x000y0000 0x00000000 0x000000a6 0x2c5b0000 gameplay,VDP-1 write character 1 (2)
*0x000y0000 0x00000000 0x0f9800a6 0x47f10000 gameplay,VDP-1 write character 2
*0x000y0000 0x00000000 0x1d4800a6 0xfcda0000 gameplay,VDP-1 write character 3
*0x000y0000 0x00000000 0x29e300a6 0xb5e60000 gameplay,VDP-1 write character 4
*0x000y0000 0x00000000 0x38e900a6 0x392c0000 gameplay,VDP-1 write character 5
*0x000y0000 0x00000000 0x462500a6 0x77c30000 gameplay,VDP-1 write character 6
*0x000y0000 0x00000000 0x555c00a6 0x8a620000 gameplay,VDP-1 write character 7

=========================================================================================
y = setted as a 0,then after the ctrl data is moved is toggled to 1 then again toggled
    to 0 after the reading,this bit is likely to be a "calculate protection values"
    if 1,use normal ram if 0.
* = working checks
[3,low word]AFAIK this is the cartridge area and it's read-only.
(1)That area is usually (but not always) used as system registers.
(2)Same as P.O.S.T. check,it was really simple to look-up because of that.
(3)Wrong offset,or it requires something else like a bitswap?
=========================================================================================
Protection works as a sort of data transfer,it could also be that it uses
encryption on the data used...

For now I'm writing this function with a command basis so I can work better with it.
****************************************************************************************/

static UINT32 a_bus[4];
static UINT32 ctrl_index;
static UINT32 internal_counter;
static UINT8 char_offset; //helper to jump the decoding of the NULL chars.
/*
ffreveng protection notes
Global:
R2 is the vector read (where to jump to)
R3 is the vector pointer

Notes:
-0x234 is a dummy vector to get safe in the debugger without doing
anything and to read the registers.

Right vectors:
0x0603B158 (but not as first,will garbage the registers)

Wrong vectors (at least not where I tested it):
0x060016fc (1st)
0x0603AFE0 (1st) (attempts to read to the sh2 internal register fffffe11)
0x060427FC (1st) (resets the sh2)
0x0603B1B2 (1st) (crashes the sh2)
*/
static UINT32 vector_prot[] = { 0x0603B1B2,0x234 };

static READ32_HANDLER( a_bus_ctrl_r )
{
	UINT32 *ROM = (UINT32 *)memory_region(REGION_USER1);

	if(a_bus[0] & 0x00010000)//protection calculation is activated
	{
		if(offset == 3)
		{
			logerror("A-Bus control protection read at %06x with data = %08x\n",activecpu_get_pc(),a_bus[3]);
			#ifdef MAME_DEBUG
			ui_popup("Prot read at %06x with data = %08x",activecpu_get_pc(),a_bus[3]);
			#endif
			switch(a_bus[3])
			{
				case 0x01230000://astrass,char data in test mode PC=60118f2
				/*
                TODO: the chars that are used are 8x8x4,but we need to convert them to
                16x16x4 format.
                [01] NULL
                [02] NULL
                [03] NULL
                [04] NULL
                [05] NULL
                [06] NULL
                [07] NULL
                [08] NULL
                [09] data 1
                [10] data 2
                [11] data 3
                [12] data 4
                [13] data 5
                [14] data 6
                [15] data 7
                [16] data 8

                2b5 = A (up-left)
                2b6 = A (up-right)
                2d1 = A (down-left)
                2d0 = A (down-right)
                2f2 = S (up-left)
                2f3 = S (up-right)
                311 = S (down-left)
                310 = S (down-right)
                2f4 = T (up-left)
                2f5 = T (up-right)
                2b7 = E (up-left) (not 2bc,maybe because the up-left corner of E is equal of B?)
                2bd = E (up-right)
                */
					internal_counter++;
					if(char_offset == 0)
					{
						if(internal_counter > ((8*0x234)-1))
						{
							internal_counter = 0;
							char_offset = 1;
						}

						return 0;
					}
					else
					{
						if(internal_counter > 8)
						{
							if(internal_counter > 15)
							{
								ctrl_index-=8;
								internal_counter = 0;
							}

							ctrl_index++;
							return ROM[ctrl_index];
						}
						else
						{
							ctrl_index++;
							return ROM[ctrl_index];
						}
					}
				case 0x10da0000://ffreveng, boot vectors at $6080000,test mode
					ctrl_index++;
					if(ctrl_index > 2)
						return 0x234;
					else
						return vector_prot[ctrl_index-1];
				case 0x10d70000://ffreveng, boot vectors at $6080000,attract mode
					ctrl_index++;
					return ROM[ctrl_index];
				case 0x2c5b0000://sss
				case 0x47f10000:
				case 0xfcda0000:
				case 0xb5e60000:
				case 0x392c0000:
				case 0x77c30000:
				case 0x8a620000:
					ctrl_index++;
					return ROM[ctrl_index];
				case 0x77770000: {//rsgun
					UINT32 val =
						((ctrl_index & 0xff)<<24) |
						(((ctrl_index+1) & 0xff)<<16) |
						(((ctrl_index+2) & 0xff)<<8) |
						((ctrl_index+3) & 0xff);
					if(ctrl_index & 0x100)
						val &= 0x0f0f0f0f;
					else
						val &= 0xf0f0f0f0;

					ctrl_index += 4;
					return val;
				}
				case 0xff7f0000://elandore
					if(a_bus[2] == 0xe69000f9)
					{
						ctrl_index++;
						return ROM[ctrl_index];
					}
					else return 0x12345678;
				case 0xffbf0000:
					ctrl_index++;
					return ROM[ctrl_index];
			}
		}
		return a_bus[offset];
	}
	else
	{
		if(a_bus[offset] != 0) return a_bus[offset];
		else return ROM[(0x02fffff0/4)+offset];
	}
}

static WRITE32_HANDLER ( a_bus_ctrl_w )
{
	COMBINE_DATA(&a_bus[offset]);
	logerror("A-Bus control protection write at %06x: [%02x] <- %08x\n",activecpu_get_pc(),offset,data);
	if(offset == 3)
	{
		switch(a_bus[3])
		{
			/*astrass,I need an original test mode screen to compare...*/
			case 0x01230000:
			char_offset = 0;
			internal_counter = 0;
			ctrl_index = ((0x400000)/4)-1; break;
			/*ffreveng*/
			case 0x10d70000: ctrl_index = 0; break;
			case 0x10da0000: ctrl_index = 0; break;
			/*sss,TRUSTED*/
			case 0x2c5b0000: ctrl_index = (0x145ffac/4)-1; break;
			/*sss,might be offset...*/
			case 0x47f10000: ctrl_index = ((0x145ffac+0xbaf0)/4)-1; break;
			case 0xfcda0000: ctrl_index = ((0x145ffac+0x12fd0)/4)-1; break;
			case 0xb5e60000: ctrl_index = ((0x145ffac+0x1a4c4)/4)-1; break;
			case 0x392c0000: ctrl_index = ((0x145ffac+0x219b0)/4)-1; break;
			case 0x77c30000: ctrl_index = ((0x145ffac+0x28ea0)/4)-1; break;
			case 0x8a620000: ctrl_index = ((0x145ffac+0x30380)/4)-1; break;
			/*rsgun*/
			case 0x77770000: ctrl_index = 0; break;
			/*elandore*/
			case 0xff7f0000: ctrl_index = ((0x400000)/4)-1; break;
			case 0xffbf0000: ctrl_index = (0x1c40000/4)-1; break;
		}
	}
	//ui_popup("%04x %04x",data,offset/4);
}

extern WRITE32_HANDLER ( stv_vdp2_vram_w );
extern READ32_HANDLER ( stv_vdp2_vram_r );

extern WRITE32_HANDLER ( stv_vdp2_cram_w );
extern READ32_HANDLER ( stv_vdp2_cram_r );

extern WRITE32_HANDLER ( stv_vdp2_regs_w );
extern READ32_HANDLER ( stv_vdp2_regs_r );

extern VIDEO_START ( stv_vdp2 );
extern VIDEO_UPDATE( stv_vdp2 );

extern READ32_HANDLER( stv_vdp1_regs_r );
extern WRITE32_HANDLER( stv_vdp1_regs_w );
extern READ32_HANDLER ( stv_vdp1_vram_r );
extern WRITE32_HANDLER ( stv_vdp1_vram_w );

extern WRITE32_HANDLER ( stv_vdp1_framebuffer0_w );
extern READ32_HANDLER ( stv_vdp1_framebuffer0_r );

extern WRITE32_HANDLER ( stv_vdp1_framebuffer1_w );
extern READ32_HANDLER ( stv_vdp1_framebuffer1_r );

static READ32_HANDLER( stv_sh2_random_r )
{
	return 0xffffffff;
}

static ADDRESS_MAP_START( stv_mem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0007ffff) AM_ROM   // bios
	AM_RANGE(0x00100000, 0x0010007f) AM_READWRITE(stv_SMPC_r32, stv_SMPC_w32)
	AM_RANGE(0x00180000, 0x0018ffff) AM_RAM AM_SHARE(1) AM_BASE(&stv_backupram)
	AM_RANGE(0x00200000, 0x002fffff) AM_RAM AM_MIRROR(0x100000) AM_SHARE(2) AM_BASE(&stv_workram_l)
	AM_RANGE(0x00400000, 0x0040001f) AM_READWRITE(stv_io_r32, stv_io_w32) AM_BASE(&ioga) AM_SHARE(4) AM_MIRROR(0x20)
	AM_RANGE(0x01000000, 0x01000003) AM_WRITE(minit_w)
	AM_RANGE(0x01406f40, 0x01406f43) AM_WRITE(minit_w) // prikura seems to write here ..
//  AM_RANGE(0x01000000, 0x01000003) AM_WRITE(minit_w) AM_MIRROR(0x00080000)
	AM_RANGE(0x01800000, 0x01800003) AM_WRITE(sinit_w)
	AM_RANGE(0x02000000, 0x04ffffef) AM_ROM AM_ROMBANK(1) AM_REGION(REGION_USER1, 0) // cartridge
	AM_RANGE(0x04fffff0, 0x04ffffff) AM_READWRITE(a_bus_ctrl_r,a_bus_ctrl_w)
	AM_RANGE(0x05800000, 0x0589ffff) AM_READWRITE(stvcd_r, stvcd_w)
	/* Sound */
	AM_RANGE(0x05a00000, 0x05a7ffff) AM_READWRITE(stv_sh2_soundram_r, stv_sh2_soundram_w)
	//AM_RANGE(0x05a80000, 0x05afffff) AM_READ(stv_sh2_random_r)
	AM_RANGE(0x05b00000, 0x05b00fff) AM_READWRITE(stv_scsp_regs_r32, stv_scsp_regs_w32)
	/* VDP1 */
	/*0x05c00000-0x05c7ffff VRAM*/
	/*0x05c80000-0x05c9ffff Frame Buffer 0*/
	/*0x05ca0000-0x05cbffff Frame Buffer 1*/
	/*0x05d00000-0x05d7ffff VDP1 Regs */
	AM_RANGE(0x05c00000, 0x05c7ffff) AM_READWRITE(stv_vdp1_vram_r, stv_vdp1_vram_w)
	AM_RANGE(0x05c80000, 0x05cbffff) AM_READWRITE(stv_vdp1_framebuffer0_r, stv_vdp1_framebuffer0_w)
	AM_RANGE(0x05d00000, 0x05d0001f) AM_READWRITE(stv_vdp1_regs_r, stv_vdp1_regs_w)
	AM_RANGE(0x05e00000, 0x05efffff) AM_READWRITE(stv_vdp2_vram_r, stv_vdp2_vram_w)
	AM_RANGE(0x05f00000, 0x05f7ffff) AM_READWRITE(stv_vdp2_cram_r, stv_vdp2_cram_w)
	AM_RANGE(0x05f80000, 0x05fbffff) AM_READWRITE(stv_vdp2_regs_r, stv_vdp2_regs_w)
	AM_RANGE(0x05fe0000, 0x05fe00cf) AM_READWRITE(stv_scu_r32, stv_scu_w32)
	AM_RANGE(0x06000000, 0x060fffff) AM_RAM AM_MIRROR(0x01f00000) AM_SHARE(3) AM_BASE(&stv_workram_h)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_mem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_RAM AM_BASE(&sound_ram)
	AM_RANGE(0x100000, 0x100fff) AM_READWRITE(SCSP_0_r, SCSP_0_w)
ADDRESS_MAP_END

#define STV_PLAYER_INPUTS(_n_, _b1_, _b2_, _b3_, _b4_) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_##_b1_         ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_##_b2_         ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_##_b3_         ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_##_b4_         ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(_n_) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(_n_)

INPUT_PORTS_START( stv )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "PDR1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "PDR2" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	STV_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START
	STV_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, BUTTON4)
/*
    PORT_START
    STV_PLAYER_INPUTS(3, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

    PORT_START
    STV_PLAYER_INPUTS(4, BUTTON1, BUTTON2, BUTTON3, BUTTON4)
*/

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("1P Push Switch") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("2P Push Switch") PORT_CODE(KEYCODE_8)

	/*This *might* be unused...*/
	PORT_START
	PORT_BIT ( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	/*Extra button layout,used by Power Instinct 3 & Suikoenbu*/
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	/*We don't need these,AFAIK the country code doesn't work either...*/
	#if 0
	PORT_START							//7
	PORT_DIPNAME( 0x0f, 0x01, "Country" )
	PORT_DIPSETTING(    0x01, DEF_STR( Japan ) )
	PORT_DIPSETTING(    0x02, "Asia Ntsc" )
	PORT_DIPSETTING(    0x04, DEF_STR( USA ) )
	PORT_DIPSETTING(    0x08, "Sud America Ntsc" )
	PORT_DIPSETTING(    0x06, "Korea" )
	PORT_DIPSETTING(    0x0a, "Asia Pal" )
	PORT_DIPSETTING(    0x0c, "Europe/Other Pal" )
	PORT_DIPSETTING(    0x0d, "Sud America Pal" )

	PORT_START	/* Pad data 1a */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("B") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("C") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("A") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Start") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Up") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Down") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Left") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Right") PORT_CODE(KEYCODE_L)

	PORT_START	/* Pad data 1b */
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("L trig") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Z") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Y") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("X") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("R trig") PORT_CODE(KEYCODE_S)
	#endif
INPUT_PORTS_END

INPUT_PORTS_START( critcrsh )
	PORT_START /* 0 */
	PORT_DIPNAME( 0x01, 0x01, "PDR1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* 1 */
	PORT_DIPNAME( 0x01, 0x01, "PDR2" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* 2 */
	STV_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START /* 3 */
	STV_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START /* 4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("1P Push Switch") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("2P Push Switch") PORT_CODE(KEYCODE_8)

	/*This *might* be unused...*/
	PORT_START /* 5 */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	/*Extra button layout,used by Power Instinct 3 & Suikoenbu*/
	PORT_START /* 6 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	/* IN 7 */
	PORT_START /* mask default type                     sens delta min max */
	PORT_BIT( 0x3f, 0x00, IPT_LIGHTGUN_X ) PORT_MINMAX(0,0x3f) PORT_SENSITIVITY(1) PORT_KEYDELTA(1) PORT_PLAYER(1)

	/* IN 8 */
	PORT_START
	PORT_BIT( 0x3f, 0x00, IPT_LIGHTGUN_Y ) PORT_MINMAX(0x0,0x3f) PORT_SENSITIVITY(1) PORT_KEYDELTA(1) PORT_PLAYER(1)

INPUT_PORTS_END

/*Same as the regular one,but with an additional & optional mahjong panel*/
INPUT_PORTS_START( stvmp )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "PDR1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "PDR2" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	STV_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START
	STV_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, BUTTON4)
/*
    PORT_START
    STV_PLAYER_INPUTS(3, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

    PORT_START
    STV_PLAYER_INPUTS(4, BUTTON1, BUTTON2, BUTTON3, BUTTON4)
*/

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("1P Push Switch") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("2P Push Switch") PORT_CODE(KEYCODE_8)

	/*This *might* be unused...*/
	PORT_START
	PORT_BIT ( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	/*Extra button layout,used by Power Instinct 3*/
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	/*Mahjong panel/player 1 side*/
	PORT_START/*7*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_KAN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_E )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_A )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_MAHJONG_M )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_MAHJONG_I )

	PORT_START/*8*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_RON )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_F )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_B )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_MAHJONG_N )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_MAHJONG_J )

	PORT_START/*9*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_REACH )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_G )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_C )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_MAHJONG_CHI )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_MAHJONG_K )

	PORT_START/*10*/
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_H )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_D )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_MAHJONG_PON )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_MAHJONG_L )

	PORT_START/*11*/
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	/*Mahjong panel/player 2 side*/
	PORT_START/*12*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_KAN ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_E ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_A ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_MAHJONG_M ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_MAHJONG_I ) PORT_PLAYER(2)

	PORT_START/*13*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_RON ) PORT_PLAYER(2)
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_F ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_B ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_MAHJONG_N ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_MAHJONG_J ) PORT_PLAYER(2)

	PORT_START/*14*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_REACH ) PORT_PLAYER(2)
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_G ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_C ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_MAHJONG_CHI ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_MAHJONG_K ) PORT_PLAYER(2)

	PORT_START/*15*/
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_H ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_MAHJONG_D ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_MAHJONG_PON ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_MAHJONG_L ) PORT_PLAYER(2)

	PORT_START/*16*/
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_MAHJONG_FLIP_FLOP ) PORT_PLAYER(2)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

WRITE32_HANDLER ( w60ffc44_write )
{
	COMBINE_DATA(&stv_workram_h[0xffc44/4]);

	logerror("cpu #%d (PC=%08X): 60ffc44_write write = %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), data, mem_mask ^ 0xffffffff);
	//sinit_w(offset,data,mem_mask);
}

WRITE32_HANDLER ( w60ffc48_write )
{
	COMBINE_DATA(&stv_workram_h[0xffc48/4]);

	logerror("cpu #%d (PC=%08X): 60ffc48_write write = %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), data, mem_mask ^ 0xffffffff);
	//minit_w(offset,data,mem_mask);
}

static void print_game_info(void);

DRIVER_INIT ( stv )
{
	time_t ltime;
	struct tm *today;
	time(&ltime);
	today = localtime(&ltime);

	/* amount of time to boost interleave for on MINIT / SINIT, needed for communication to work */
	minit_boost = 400;
	sinit_boost = 400;
	minit_boost_timeslice = 0;
	sinit_boost_timeslice = 0;

	smpc_ram = auto_malloc (0x80);
	stv_scu = auto_malloc (0x100);
	scsp_regs = auto_malloc (0x1000);

	install_stvbios_speedups();

	/* debug .. watch the command buffer rsgun, cottonbm etc. appear to use to communicate between cpus */
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60ffc44, 0x60ffc47, 0, 0, w60ffc44_write );
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60ffc48, 0x60ffc4b, 0, 0, w60ffc48_write );
	memory_install_write32_handler(1, ADDRESS_SPACE_PROGRAM, 0x60ffc44, 0x60ffc47, 0, 0, w60ffc44_write );
	memory_install_write32_handler(1, ADDRESS_SPACE_PROGRAM, 0x60ffc48, 0x60ffc4b, 0, 0, w60ffc48_write );

  	smpc_ram[0x23] = DectoBCD((today->tm_year + 1900)/100);
    smpc_ram[0x25] = DectoBCD((today->tm_year + 1900)%100);
    smpc_ram[0x27] = (today->tm_wday << 4) | (today->tm_mon+1);
    smpc_ram[0x29] = DectoBCD(today->tm_mday);
    smpc_ram[0x2b] = DectoBCD(today->tm_hour);
    smpc_ram[0x2d] = DectoBCD(today->tm_min);
    smpc_ram[0x2f] = DectoBCD(today->tm_sec);
    smpc_ram[0x31] = 0x00; //CTG1=0 CTG0=0 (correct??)
//  smpc_ram[0x33] = readinputport(7);
 	smpc_ram[0x5f] = 0x10;

 	#ifdef MAME_DEBUG
 	/*Uncomment this to enable header info*/
 	//print_game_info();
	#endif
}

#define DATA_TRANSFER(_max_) \
	for(dst_i=0;dst_i<_max_;dst_i++,src_i++) 	\
		STR[(dst_i & 0xfc) | (~dst_i & 3)] = ROM[src_i];

#define DATA_DELETE \
	for(dst_i=0;dst_i<0x100;dst_i++) \
		STR[dst_i] = 0x00;

static void print_game_info(void)
{
	UINT8 *ROM = memory_region(REGION_USER1);
	static FILE *print_file = NULL;
	UINT8 STR[0x100];
	UINT32 src_i,dst_i;

	if(print_file == NULL)
		print_file = fopen( "stvinfo.txt", "a" );

	src_i = 0;

	/*IC13?*/
	if(ROM[src_i] == 0x00)
		src_i+=0x200000;

	DATA_TRANSFER(0x100);
	for(src_i=0;src_i<0x100;src_i++)
	{
		if((src_i % 0x10) == 0) fprintf( print_file, "\n");
		if(src_i < 0xc0)		fprintf( print_file, "%c",STR[src_i] );
		else                    fprintf( print_file, "%02x",STR[src_i] );
	}
	DATA_DELETE;

	fclose(print_file);
	print_file = NULL;
}

MACHINE_START( stv )
{
	SCSP_set_ram_base(0, sound_ram);
	return 0;
}

MACHINE_RESET( stv )
{
	// don't let the slave cpu and the 68k go anywhere
	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
	stv_enable_slave_sh2 = 0;
	cpunum_set_input_line(2, INPUT_LINE_RESET, ASSERT_LINE);

	timer_0 = 0;
	timer_1 = 0;
	en_68k = 0;
	NMI_reset = 1;
	smpc_ram[0x21] = (0x80) | ((NMI_reset & 1) << 6);

	cpunum_set_clock(0, MASTER_CLOCK_320/2);
	cpunum_set_clock(1, MASTER_CLOCK_320/2);
	cpunum_set_clock(2, MASTER_CLOCK_320/5);

	stvcd_reset();
}

static const gfx_layout tiles8x8x4_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout tiles16x16x4_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28,
	  32*8+0, 32*8+4, 32*8+8, 32*8+12, 32*8+16, 32*8+20, 32*8+24, 32*8+28,

	  },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  32*16, 32*17,32*18, 32*19,32*20,32*21,32*22,32*23

	  },
	32*32
};

static const gfx_layout tiles8x8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8, 16, 24, 32, 40, 48, 56 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8
};

static const gfx_layout tiles16x16x8_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8, 16, 24, 32, 40, 48, 56,
	64*8+0, 65*8, 66*8, 67*8, 68*8, 69*8, 70*8, 71*8

	},
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
	64*16, 64*17, 64*18, 64*19, 64*20, 64*21, 64*22, 64*23
	},
	128*16
};




static const gfx_decode gfxdecodeinfo[] =
{
	{ 0, 0, &tiles8x8x4_layout,   0x00, (0x80*(2+1))  },
	{ 0, 0, &tiles16x16x4_layout, 0x00, (0x80*(2+1))  },
	{ 0, 0, &tiles8x8x8_layout,   0x00, (0x08*(2+1))  },
	{ 0, 0, &tiles16x16x8_layout, 0x00, (0x08*(2+1))  },

	/* vdp1 .. pointless for drawing but can help us debug */
	{ 0, 0, &tiles8x8x4_layout,   0x00, 0x100  },
	{ 0, 0, &tiles16x16x4_layout, 0x00, 0x100  },
	{ 0, 0, &tiles8x8x8_layout,   0x00, 0x20  },
	{ 0, 0, &tiles16x16x8_layout, 0x00, 0x20  },

	{ -1 } /* end of array */
};

struct sh2_config sh2_conf_master = { 0 };
struct sh2_config sh2_conf_slave  = { 1 };

static int scsp_last_line = 0;

static void scsp_irq(int irq)
{
	// don't bother the 68k if it's off
	if (!en_68k)
	{
		return;
	}

	if (irq)
	{
		scsp_last_line = irq;
		cpunum_set_input_line(2, irq, ASSERT_LINE);
	}
	else
	{
		cpunum_set_input_line(2, scsp_last_line, CLEAR_LINE);
	}
}

static struct SCSPinterface scsp_interface =
{
	0,
	0,
	scsp_irq
};

static VIDEO_UPDATE(critcrsh)
{
	int gun_x, gun_y;
	video_update_stv_vdp2(screen, bitmap, cliprect);
	gun_x = readinputport(7);
	gun_y = readinputport(8);
	if ( gun_y <= 46 )
		draw_crosshair(bitmap,
					  readinputport(7)*Machine->visible_area.max_x/64,
					  readinputport(8)*Machine->visible_area.max_y/46,
					  cliprect,
					  0);
}

static MACHINE_DRIVER_START( stv )

	/* basic machine hardware */
	MDRV_CPU_ADD(SH2, MASTER_CLOCK_352/2) // 28.6364 MHz
	MDRV_CPU_PROGRAM_MAP(stv_mem, 0)
	MDRV_CPU_VBLANK_INT(stv_interrupt,264)/*264 lines,224 display lines*/
	MDRV_CPU_CONFIG(sh2_conf_master)

	MDRV_CPU_ADD(SH2, MASTER_CLOCK_352/2) // 28.6364 MHz
	MDRV_CPU_PROGRAM_MAP(stv_mem, 0)
	MDRV_CPU_CONFIG(sh2_conf_slave)

	MDRV_CPU_ADD(M68000, MASTER_CLOCK_352/5) //11.46 MHz
	MDRV_CPU_PROGRAM_MAP(sound_mem, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(192);	// guess, needed to force video update after V-Blank OUT interrupt

	MDRV_MACHINE_START(stv)
	MDRV_MACHINE_RESET(stv)
	MDRV_NVRAM_HANDLER(stv) /* Actually 93c45 */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK | VIDEO_RGB_DIRECT )
	MDRV_SCREEN_SIZE(1024, 1024)
	MDRV_VISIBLE_AREA(0*8, 320-1, 0*8, 224-1)
	MDRV_PALETTE_LENGTH(2048+(2048*2))//standard palette + extra memory for rgb brightness.
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(stv_vdp2)
	MDRV_VIDEO_UPDATE(stv_vdp2)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(SCSP, 0)
	MDRV_SOUND_CONFIG(scsp_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( critcrsh )
	MDRV_IMPORT_FROM( stv )
	MDRV_VIDEO_UPDATE( critcrsh )
MACHINE_DRIVER_END

#define ROM_LOAD16_WORD_SWAP_BIOS(bios,name,offset,length,hash) \
		ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(bios+1)) /* Note '+1' */

#define STV_BIOS \
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 0, "epr19730.ic8",   0x000000, 0x080000, CRC(d0e0889d) SHA1(fae53107c894e0c41c49e191dbe706c9cd6e50bd) ) /* jp */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 1, "mp17951a.s",     0x000000, 0x080000, CRC(2672f9d8) SHA1(63cf4a6432f6c87952f9cf3ab0f977aed2367303) ) /* jp alt */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 2, "mp17952a.s",     0x000000, 0x080000, CRC(d1be2adf) SHA1(eaf1c3e5d602e1139d2090a78d7e19f04f916794) ) /* us */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 3, "20091.bin",      0x000000, 0x080000, CRC(59ed40f4) SHA1(eff0f54c70bce05ff3a289bf30b1027e1c8cd117) ) /* jp alt 2 */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 4, "mp17953a.ic8",   0x000000, 0x080000, CRC(a4c47570) SHA1(9efc73717ec8a13417e65c54344ded9fc25bf5ef) ) /* taiwan */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 5, "mp17954a.s",     0x000000, 0x080000, CRC(f7722da3) SHA1(af79cff317e5b57d49e463af16a9f616ed1eee08) ) /* Europe */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 6, "stv110.bin",     0x000000, 0x080000, CRC(3dfeda92) SHA1(8eb33192a57df5f3a1dfb57263054867c6b2db6d) ) /* ?? */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 7, "stv1061.bin",    0x000000, 0x080000, CRC(728dbca3) SHA1(0ed2030177f0aa8285645c395ae9ad9f568ab1d6) ) /* ST-V Dev Bios (1.061) - Sega 1994, Noted "ST-V Ver 1.061 94/11/25" on EPROM sticker, Coming from a S-TV SG5001A dev board */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 8, "epr-17740.bin",  0x000000, 0x080000, CRC(5c5aa63d) SHA1(06860d96923b81afbc21e0ad32ee19487d8ff6e7) ) /* ST-V Bios (Japan early) - Sega 1995, Found on a early board dated 02/1995 */ \
	/*ROM_LOAD16_WORD_SWAP_BIOS( 9, "saturn.bin",       0x000000, 0x080000, CRC(653ff2d8) SHA1(20994ae7ee177ddaf3a430b010c7620dca000fb4) )*/ /* Saturn Eu Bios */ \
	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* SH2 code */ \
	ROM_COPY( REGION_CPU1,0,0,0x080000) \

ROM_START( stvbios )
	STV_BIOS
ROM_END

SYSTEM_BIOS_START( stvbios )
	SYSTEM_BIOS_ADD( 0, "japan",       "Japan (bios epr19730)" )
	SYSTEM_BIOS_ADD( 1, "japana",      "Japan (bios mp17951a)" )
	SYSTEM_BIOS_ADD( 2, "us",          "USA (bios mp17952a)" )
	SYSTEM_BIOS_ADD( 3, "japanb",      "Japan (bios 20091)" )
	SYSTEM_BIOS_ADD( 4, "taiwan",      "Taiwan (bios mp17953a)" )
	SYSTEM_BIOS_ADD( 5, "europe",      "Europe (bios mp17954a)" )
	SYSTEM_BIOS_ADD( 6, "unknown",     "Unknown (debug?)" )
	SYSTEM_BIOS_ADD( 7, "dev1061",     "Development (bios 1.061)" )
	SYSTEM_BIOS_ADD( 8, "japanc",      "Japan (bios epr17740)" )
//  SYSTEM_BIOS_ADD( 9, "saturn",      "Saturn bios :)" )
	/*Korea*/
	/*Asia (Pal Area)*/
	/*Brazil*/
	/*Latin America*/
SYSTEM_BIOS_END

/* the roms marked as bad almost certainly aren't bad, theres some very weird
   mirroring going on, or maybe its meant to transfer the rom data to the region it
   tests from rearranging it a bit (dma?)

   comments merely indicate the status the rom gets in the rom check at the moment

*/

/*

there appears to only be one main cartridge layout, just some having different positions populated if you use the ic named in
the test mode you have the following

some of the rom names were using something else and have been renamed to match test mode, old extension left in comments

( add 0x2000000 for real memory map location )

0x0000000 - 0x01fffff IC13 Header can be read from here .. *IC13 ALWAYS fails on the games if they have one, something weird going on
0x0200000 - 0x03fffff IC7  .. or here (some games have both ic7 and ic13 but the header is in ic13 in these cases)
0x0400000 - 0x07fffff IC2
0x0800000 - 0x0bfffff IC3
0x0c00000 - 0x0ffffff IC4
0x1000000 - 0x13fffff IC5
0x1400000 - 0x17fffff IC6
0x1800000 - 0x1bfffff IC1
0x1c00000 - 0x1ffffff IC8
0x2000000 - 0x23fffff IC9
0x2400000 - 0x27fffff IC10
0x2800000 - 0x2bfffff IC11
0x2c00000 - 0x2ffffff IC12

Update:Issue fixed,see stvhacks.c for more details (TODO: Clean-up that)
*/




ROM_START( astrass )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20825.13",                0x0000000, 0x0100000, CRC(94a9ad8f) SHA1(861311c14cfa9f560752aa5b023c147a539cf135) ) // ic13 bad?! (was .24)
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr20827.2",     0x0400000, 0x0400000, CRC(65cabbb3) SHA1(5e7cb090101dc42207a4084465e419f4311b6baf) ) // good (was .12)
	ROM_LOAD16_WORD_SWAP( "mpr20828.3",     0x0800000, 0x0400000, CRC(3934d44a) SHA1(969406b8bfac43b30f4d732702ca8cffeeefffb9) ) // good (was .13)
	ROM_LOAD16_WORD_SWAP( "mpr20829.4",     0x0c00000, 0x0400000, CRC(814308c3) SHA1(45c3f551690224c95acd156ae8f8397667927a04) ) // good (was .14)
	ROM_LOAD16_WORD_SWAP( "mpr20830.5",     0x1000000, 0x0400000, CRC(ff97fd19) SHA1(f37bcdce5f3f522527a44d59f1b8184ef290f829) ) // good (was .15)
	ROM_LOAD16_WORD_SWAP( "mpr20831.6",     0x1400000, 0x0400000, CRC(4408e6fb) SHA1(d4228cad8a1128e9426dac9ac62e9513a7a0117b) ) // good (was .16)
	ROM_LOAD16_WORD_SWAP( "mpr20826.1",     0x1800000, 0x0400000, CRC(bdc4b941) SHA1(c5e8b1b186324c2ccab617915f7bdbfe6897ca9f) ) // good (was .17)
	ROM_LOAD16_WORD_SWAP( "mpr20832.8",     0x1c00000, 0x0400000, CRC(af1b0985) SHA1(d7a0e4e0a8b0556915f924bdde8c3d14e5b3423e) ) // good (was .18s)
	ROM_LOAD16_WORD_SWAP( "mpr20833.9",     0x2000000, 0x0400000, CRC(cb6af231) SHA1(4a2e5d7c2fd6179c19cdefa84a03f9a34fbb9e70) ) // good (was .19s)
ROM_END

ROM_START( bakubaku )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr17969.13",               0x0000000, 0x0100000, CRC(bee327e5) SHA1(1d226db72d6ef68fd294f60659df7f882b25def6) ) // ic13 bad?!
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr17970.2",    0x0400000, 0x0400000, CRC(bc4d6f91) SHA1(dcc241dcabea59325decfba3fd5e113c07958422) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17971.3",    0x0800000, 0x0400000, CRC(c780a3b3) SHA1(99587eea528a6413cacc3e4d3d1dbfff57b03dca) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17972.4",    0x0c00000, 0x0400000, CRC(8f29815a) SHA1(e86acd8096f2aee5f5e3ddfd3abb4f5c2b11df66) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17973.5",    0x1000000, 0x0400000, CRC(5f6e0e8b) SHA1(eeb5efb5216ab8b8fdee4656774bbd5a2a5b2d42) ) // good
ROM_END

ROM_START( colmns97 )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	/* it tests .13 at 0x000000 - 0x1fffff but reports as bad even if we put the rom there */
	ROM_LOAD( "fpr19553.13",    0x000000, 0x100000, CRC(d4fb6a5e) SHA1(bd3cfb4f451b6c9612e42af5ddcbffa14f057329) ) // ic13 bad?!
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr19554.2",     0x400000, 0x400000, CRC(5a3ebcac) SHA1(46e3d1cf515a7ff8a8f97e5050b29dbbeb5060c0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19555.3",     0x800000, 0x400000, CRC(74f6e6b8) SHA1(8080860550eb770e04447e344fb337748a249761) ) // good
ROM_END

ROM_START( cotton2 )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_FILL(                              0x0000000, 0x0200000, 0x00 )
	ROM_LOAD16_WORD_SWAP( "mpr20122.7",    0x0200000, 0x0200000, CRC(d616f78a) SHA1(8039dcdfdafb8327a19a1da46a67c0b3f7eee53a) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20117.2",    0x0400000, 0x0400000, CRC(893656ea) SHA1(11e3160083ba018fbd588f07061a4e55c1efbebb) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20118.3",    0x0800000, 0x0400000, CRC(1b6a1d4c) SHA1(6b234d6b2d24df7f6d400a56698c0af2f78ce0e7) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20119.4",    0x0c00000, 0x0400000, CRC(5a76e72b) SHA1(0a058627ddf78a0bcdaba328a58712419f24e33b) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20120.5",    0x1000000, 0x0400000, CRC(7113dd7b) SHA1(f86add67c4e1349a9b9ebcd0145a30b1667df811) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20121.6",    0x1400000, 0x0400000, CRC(8c8fd521) SHA1(c715681330b5ed37a8506ac58ee2143baa721206) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20116.1",    0x1800000, 0x0400000, CRC(d30b0175) SHA1(2da5c3c02d68b8324948a8cdc93946d97fccdd8f) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20123.8",    0x1c00000, 0x0400000, CRC(35f1b89f) SHA1(1d6007c380f817def734fc3030d4fe56df4a15be) ) // good
ROM_END

ROM_START( cottonbm )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_FILL(                              0x0000000, 0x0200000, 0x00 )
	ROM_LOAD16_WORD_SWAP( "mpr21075.7",    0x0200000, 0x0200000, CRC(200b58ba) SHA1(6daad6d70a3a41172e8d9402af775c03e191232d) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21070.2",    0x0400000, 0x0400000, CRC(56c0bf1d) SHA1(c2b564ce536c637bb723ed96683b27596e87ebe7) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21071.3",    0x0800000, 0x0400000, CRC(2bb18df2) SHA1(e900adb94ad3f48be00a4ce33e915147dc6a8737) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21072.4",    0x0c00000, 0x0400000, CRC(7c7cb977) SHA1(376dfb8014050605b00b6545520bd544768f5828) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21073.5",    0x1000000, 0x0400000, CRC(f2e5a5b7) SHA1(9258d508ef6f6529efc4ad172fd29e69877a99eb) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21074.6",    0x1400000, 0x0400000, CRC(6a7e7a7b) SHA1(a0b1e7a85e623b59886b28797281df1d65b8a5aa) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21069.1",    0x1800000, 0x0400000, CRC(6a28e3c5) SHA1(60454b71db49b872e0cb89fae2259fed601588bd) ) // good
ROM_END

ROM_START( decathlt )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18967.13",               0x0000000, 0x0100000, CRC(c0446674) SHA1(4917089d95613c9d2a936ed9fe3ebd22f461aa4f) ) // ic13 bad?!
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr18968.2",    0x0400000, 0x0400000, CRC(11a891de) SHA1(1a4fa8d7e07e1d8fdc8122ef8a5b93723c007cda) ) // good (was .1)
	ROM_LOAD16_WORD_SWAP( "mpr18969.3",    0x0800000, 0x0400000, CRC(199cc47d) SHA1(d78f7c6be7e9b43e208244c5c8722245f4c653e1) ) // good (was .2)
	ROM_LOAD16_WORD_SWAP( "mpr18970.4",    0x0c00000, 0x0400000, CRC(8b7a509e) SHA1(8f4d36a858231764ed09b26a1141d1f055eee092) ) // good (was .3)
	ROM_LOAD16_WORD_SWAP( "mpr18971.5",    0x1000000, 0x0400000, CRC(c87c443b) SHA1(f2fedb35c80e5c4855c7aebff88186397f4d51bc) ) // good (was .4)
	ROM_LOAD16_WORD_SWAP( "mpr18972.6",    0x1400000, 0x0400000, CRC(45c64fca) SHA1(ae2f678b9885426ce99b615b7f62a451f9ef83f9) ) // good (was .5)
ROM_END

ROM_START( diehard )
 	STV_BIOS // must use USA
	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr19119.13",               0x0000000, 0x0100000, CRC(de5c4f7c) SHA1(35f670a15e9c86edbe2fe718470f5a75b5b096ac) ) // ic13 bad?!
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr19115.2",    0x0400000, 0x0400000, CRC(6fe06a30) SHA1(dedb90f800bae8fd9df1023eb5bec7fb6c9d0179) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19116.3",    0x0800000, 0x0400000, CRC(af9e627b) SHA1(a53921c3185a93ec95299bf1c29e744e2fa3b8c0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19117.4",    0x0c00000, 0x0400000, CRC(74520ff1) SHA1(16c1acf878664b3bd866c9b94f3695ae892ac12f) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19118.5",    0x1000000, 0x0400000, CRC(2c9702f0) SHA1(5c2c66de83f2ccbe97d3b1e8c7e65999e1fa2de1) ) // good
ROM_END

ROM_START( dnmtdeka )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr19114.13",               0x0000000, 0x0100000, CRC(1fd22a5f) SHA1(c3d9653b12354a73a3e15f23a2ab7992ffb83e46) ) // ic13 bad?!
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr19115.2",    0x0400000, 0x0400000, CRC(6fe06a30) SHA1(dedb90f800bae8fd9df1023eb5bec7fb6c9d0179) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19116.3",    0x0800000, 0x0400000, CRC(af9e627b) SHA1(a53921c3185a93ec95299bf1c29e744e2fa3b8c0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19117.4",    0x0c00000, 0x0400000, CRC(74520ff1) SHA1(16c1acf878664b3bd866c9b94f3695ae892ac12f) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19118.5",    0x1000000, 0x0400000, CRC(2c9702f0) SHA1(5c2c66de83f2ccbe97d3b1e8c7e65999e1fa2de1) ) // good
ROM_END

ROM_START( ejihon )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18137.13",               0x0000000, 0x0080000, CRC(151aa9bc) SHA1(0959c60f31634816825acb57413838dcddb17d31) ) // ic13 bad?!
	ROM_RELOAD ( 0x0080000, 0x0080000 )
	ROM_RELOAD ( 0x0100000, 0x0080000 )
	ROM_RELOAD ( 0x0180000, 0x0080000 )
	ROM_RELOAD ( 0x0200000, 0x0080000 )
	ROM_RELOAD ( 0x0280000, 0x0080000 )
	ROM_RELOAD ( 0x0300000, 0x0080000 )
	ROM_RELOAD ( 0x0380000, 0x0080000 )
	ROM_LOAD16_WORD_SWAP( "mpr18138.2",    0x0400000, 0x0400000, CRC(f5567049) SHA1(6eb35e4b5fbda39cf7e8c42b6a568bd53a364d6d) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18139.3",    0x0800000, 0x0400000, CRC(f36b4878) SHA1(e3f63c0046bd37b7ab02fb3865b8ebcf4cf68e75) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18140.4",    0x0c00000, 0x0400000, CRC(228850a0) SHA1(d83f7fa7df08407fa45a13661393679b88800805) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18141.5",    0x1000000, 0x0400000, CRC(b51eef36) SHA1(2745cba48dc410d6d31327b956886ec284b9eac3) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18142.6",    0x1400000, 0x0400000, CRC(cf259541) SHA1(51e2c8d16506d6074f6511112ec4b6b44bed4886) ) // good
ROM_END

ROM_START( elandore )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_FILL(                              0x0000000, 0x0200000, 0x00 )
	ROM_LOAD16_WORD_SWAP( "mpr21307.7",    0x0200000, 0x0200000, CRC(966ad472) SHA1(d6db41d1c40d08eb6bce8a8a2f491e7533daf670) ) // good (was .11s)
	ROM_LOAD16_WORD_SWAP( "mpr21301.2",    0x0400000, 0x0400000, CRC(1a23b0a0) SHA1(f9dbc7ba96dadfb00e5827622b557080449acd83) ) // good (was .12)
	ROM_LOAD16_WORD_SWAP( "mpr21302.3",    0x0800000, 0x0400000, CRC(1c91ca33) SHA1(ae11209088e3bf8fc4a92dca850d7303ce949b29) ) // good (was .13)
	ROM_LOAD16_WORD_SWAP( "mpr21303.4",    0x0c00000, 0x0400000, CRC(07b2350e) SHA1(f32f63fd8bec4e667f61da203d63be9a27798dfe) ) // good (was .14)
	ROM_LOAD16_WORD_SWAP( "mpr21304.5",    0x1000000, 0x0400000, CRC(cfea52ae) SHA1(4b6d27e0b2a95300ee9e07ebcdc4953d77c4efbe) ) // good (was .15)
	ROM_LOAD16_WORD_SWAP( "mpr21305.6",    0x1400000, 0x0400000, CRC(46cfc2a2) SHA1(8ca26bf8fa5ced040e815c125c13dd06d599e189) ) // good (was .16)
	ROM_LOAD16_WORD_SWAP( "mpr21306.1",    0x1800000, 0x0400000, CRC(87a5929c) SHA1(b259341d7b0e1fa98959bf52d23db5c308a8efdd) ) // good (was .17)
	ROM_LOAD16_WORD_SWAP( "mpr21308.8",    0x1c00000, 0x0400000, CRC(336ec1a4) SHA1(20d1fce050cf6132d284b91853a4dd5626372ef0) ) // good (was .18s)
ROM_END

ROM_START( ffreveng )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_FILL(                             0x0000000, 0x0200000, 0x00 )
	ROM_LOAD16_WORD_SWAP( "opr21872.7",   0x0200000, 0x0200000, CRC(32d36fee) SHA1(441c4254ef2e9301e1006d69462a850ce339314b) ) // good (was .11s)
	ROM_LOAD16_WORD_SWAP( "mpr21873.2",   0x0400000, 0x0400000, CRC(dac5bd98) SHA1(6102035ce9eb2f83d7d9b20f989a151f45087c67) ) // good (was .12)
	ROM_LOAD16_WORD_SWAP( "mpr21874.3",   0x0800000, 0x0400000, CRC(0a7be2f1) SHA1(e2d13f36e54d1e2cb9d584db829c04a6ff65108c) ) // good (was .13)
	ROM_LOAD16_WORD_SWAP( "mpr21875.4",   0x0c00000, 0x0400000, CRC(ccb75029) SHA1(9611a08a2ad0e0e82137ded6205440a948a339a4) ) // good (was .14)
	ROM_LOAD16_WORD_SWAP( "mpr21876.5",   0x1000000, 0x0400000, CRC(bb92a7fc) SHA1(d9e0fab1104a46adeb0a0cfc0d070d4c63a28d55) ) // good (was .15)
	ROM_LOAD16_WORD_SWAP( "mpr21877.6",   0x1400000, 0x0400000, CRC(c22a4a75) SHA1(3276bc0628e71b432f21ba9a4f5ff7ccc8769cd9) ) // good (was .16)
	ROM_LOAD16_WORD_SWAP( "opr21878.1",   0x1800000, 0x0200000, CRC(2ea4a64d) SHA1(928a973dce5eba0a1628d61ba56a530de990a946) ) // good (was .17)
ROM_END

/* set system to 1 player to test rom */
ROM_START( fhboxers )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fr18541a.13",               0x0000000, 0x0100000, CRC(8c61a17c) SHA1(a8aef27b53482923a506f7daa4b7a38653b4d8a4) ) // ic13 bad?! (header is read from here, not ic7 even if both are populated on this board)
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )

	ROM_LOAD16_WORD_SWAP( "mpr18538.7",    0x0200000, 0x0200000, CRC(7b5230c5) SHA1(70cebc3281580b43adf42c37318e12159c28a13d) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18533.2",    0x0400000, 0x0400000, CRC(7181fe51) SHA1(646f95e1a5b64d721e961352cee6fd5adfd031ec) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18534.3",    0x0800000, 0x0400000, CRC(c87ef125) SHA1(c9ced130faf6dd9e626074b6519615654d8beb19) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18535.4",    0x0c00000, 0x0400000, CRC(929a64cf) SHA1(206dfc2a46befbcea974df1e27515c5759d88d00) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18536.5",    0x1000000, 0x0400000, CRC(51b9f64e) SHA1(bfbdfb73d24f26ce1cc5294c23a1712fb9631691) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18537.6",    0x1400000, 0x0400000, CRC(c364f6a7) SHA1(4db21bcf6ea3e75f9eb34f067b56a417589271c0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18532.1",    0x1800000, 0x0400000, CRC(39528643) SHA1(e35f4c35c9eb13e1cdcc26cb2599bb846f2c1af7) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18539.8",    0x1c00000, 0x0400000, CRC(62b3908c) SHA1(3f00e49beb0e5575cc4250a25c41f04dc91d6ed0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18540.9",    0x2000000, 0x0400000, CRC(4c2b59a4) SHA1(4d15503fcff0e9e0d1ed3bac724278102b506da0) ) // good
ROM_END

/* set system to 1 player to test rom */
ROM_START( findlove )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20424.13",               0x0000000, 0x0100000, CRC(4e61fa46) SHA1(e34624d98cbdf2dd04d997167d3c4decd2f208f7) ) // ic13 bad?! (header is read from here, not ic7 even if both are populated on this board)
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	//ROM_LOAD16_WORD_SWAP( "mpr20431.7",    0x0200000, 0x0200000, CRC(ea656ced) SHA1(b2d6286081bd46a89d1284a2757b87d0bca1bbde) ) // good
	//ROM_COPY(REGION_USER1,0x080000,0x100000,(0x80000))//WRONG,will be worked out...
	//ROM_FILL(                              0x0180000, 0x0080000, 0x00 )
	ROM_LOAD16_WORD_SWAP( "mpr20431.7",    0x0200000, 0x0200000, CRC(ea656ced) SHA1(b2d6286081bd46a89d1284a2757b87d0bca1bbde) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20426.2",    0x0400000, 0x0400000, CRC(897d1747) SHA1(f3fb2c4ef8bc2c1658907e822f2ee2b88582afdd) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20427.3",    0x0800000, 0x0400000, CRC(a488a694) SHA1(80ec81f32e4b5712a607208b2a45cfdf6d5e1849) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20428.4",    0x0c00000, 0x0400000, CRC(4353b3b6) SHA1(f5e56396b345ff65f57a23f391b77d401f1f58b5) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20429.5",    0x1000000, 0x0400000, CRC(4f566486) SHA1(5b449288e33f02f2362ebbd515c87ea11cc02633) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20430.6",    0x1400000, 0x0400000, CRC(d1e11979) SHA1(14405997eefac22c42f0c86dca9411ba1dee9bf9) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20425.1",    0x1800000, 0x0400000, CRC(67f104c4) SHA1(8e965d2ce554ba8d37254f6bf3931dff4bce1a43) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20432.8",    0x1c00000, 0x0400000, CRC(79fcdecd) SHA1(df8e7733a51e24196914fc66a024515ee1565599) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20433.9",    0x2000000, 0x0400000, CRC(82289f29) SHA1(fb6a1015621b1afa3913da162ae71ded6b674649) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20434.10",   0x2400000, 0x0400000, CRC(85c94afc) SHA1(dfc2f16614bc499747ea87567a21c86e7bddce45) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20435.11",   0x2800000, 0x0400000, CRC(263a2e48) SHA1(27ef4bf577d240e36dcb6e6a09b9c5f24e59ce8c) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20436.12",   0x2c00000, 0x0400000, CRC(e3823f49) SHA1(754d48635bd1d4fb01ff665bfe2a71593d92f688) ) // good
ROM_END

ROM_START( finlarch )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "finlarch.13",               0x0000000, 0x0100000, CRC(4505fa9e) SHA1(96c6399146cf9c8f1d27a8fb6a265f937258004a) ) // ic13 bad?!
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr18257.2",    0x0400000, 0x0400000, CRC(137fdf55) SHA1(07a02fe531b3707e063498f5bc9749bd1b4cadb3) ) // good
	ROM_RELOAD(                            0x1400000, 0x0400000 )
	ROM_LOAD16_WORD_SWAP( "mpr18258.3",    0x0800000, 0x0400000, CRC(f519c505) SHA1(5cad39314e46b98c24a71f1c2c10c682ef3bdcf3) ) // good
	ROM_RELOAD(                            0x1800000, 0x0400000 )
	ROM_LOAD16_WORD_SWAP( "mpr18259.4",    0x0c00000, 0x0400000, CRC(5cabc775) SHA1(84383a4cbe3b1a9dcc6c140cff165425666dc780) ) // good
	ROM_RELOAD(                            0x1c00000, 0x0400000 )
	ROM_LOAD16_WORD_SWAP( "mpr18260.5",    0x1000000, 0x0400000, CRC(f5b92082) SHA1(806ad85a187a23a5cf867f2f3dea7d8150065b8e) ) // good
	ROM_RELOAD(                            0x2000000, 0x0400000 )
ROM_END


ROM_START( gaxeduel )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr17766.13",               0x0000000, 0x0080000, CRC(a83fcd62) SHA1(4ce77ebaa0e93c6553ad8f7fb87cbdc32433402b) ) // ic13 bad?!
	ROM_RELOAD ( 0x0080000, 0x0080000 )
	ROM_RELOAD ( 0x0100000, 0x0080000 )
	ROM_RELOAD ( 0x0180000, 0x0080000 )
	ROM_RELOAD ( 0x0200000, 0x0080000 )
	ROM_RELOAD ( 0x0280000, 0x0080000 )
	ROM_RELOAD ( 0x0300000, 0x0080000 )
	ROM_RELOAD ( 0x0380000, 0x0080000 )
	ROM_LOAD16_WORD_SWAP( "mpr17768.2",    0x0400000, 0x0400000, CRC(d6808a7d) SHA1(83a97bbe1160cb45b3bdcbde8adc0d9bae5ded60) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17769.3",    0x0800000, 0x0400000, CRC(3471dd35) SHA1(24febddfe70984cebc0e6948ad718e0e6957fa82) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17770.4",    0x0c00000, 0x0400000, CRC(06978a00) SHA1(a8d1333a9f4322e28b23724937f595805315b136) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17771.5",    0x1000000, 0x0400000, CRC(aea2ea3b) SHA1(2fbe3e10d3f5a3b3099a7ed5b38b93b6e22e19b8) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17772.6",    0x1400000, 0x0400000, CRC(b3dc0e75) SHA1(fbe2790c84466d186ea3e9d41edfcb7afaf54bea) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17767.1",    0x1800000, 0x0400000, CRC(9ba1e7b1) SHA1(f297c3697d2e8ba4476d672267163f91f371b362) ) // good
ROM_END

ROM_START( grdforce )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_FILL(                              0x0000000, 0x0200000, 0x00 )
	ROM_LOAD16_WORD_SWAP( "mpr20844.7",    0x0200000, 0x0200000, CRC(283e7587) SHA1(477fabc27cfe149ad17757e31f10665dcf8c0860) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20839.2",    0x0400000, 0x0400000, CRC(facd4dd8) SHA1(2582894c98b31ab719f1865d4623dad6736dc877) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20840.3",    0x0800000, 0x0400000, CRC(fe0158e6) SHA1(73460effe69fb8f16dd952271542b7803471a599) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20841.4",    0x0c00000, 0x0400000, CRC(d87ac873) SHA1(35b8fa3862e09dca530e9597f983f5a22919cf08) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20842.5",    0x1000000, 0x0400000, CRC(baebc506) SHA1(f5f59f9263956d0c49c729729cf6db31dc861d3b) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20843.6",    0x1400000, 0x0400000, CRC(263e49cc) SHA1(67979861ca2784b3ce39d87e7994e6e7351b40e5) ) // good
ROM_END

ROM_START( groovef )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_FILL(                              0x0000000, 0x0200000, 0x00 )
	ROM_LOAD16_WORD_SWAP( "mpr19820.7",    0x0200000, 0x0100000, CRC(e93c4513) SHA1(f9636529224880c49bd2cc5572bd5bf41dbf911a) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19815.2",    0x0400000, 0x0400000, CRC(1b9b14e6) SHA1(b1828c520cb108e2927a23273ebd2939dca52304) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19816.3",    0x0800000, 0x0400000, CRC(83f5731c) SHA1(2f645737f945c59a1a2fabf3b21a761be9e8c8a6) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19817.4",    0x0c00000, 0x0400000, CRC(525bd6c7) SHA1(2db2501177fb0b44d0fad2054eddf356c4ea08f2) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19818.5",    0x1000000, 0x0400000, CRC(66723ba8) SHA1(0a8379e46a8f8cab11befeadd9abdf59dba68e27) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19819.6",    0x1400000, 0x0400000, CRC(ee8c55f4) SHA1(f6d86b2c2ab43ec5baefb8ccc25e11af4d82712d) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19814.1",    0x1800000, 0x0400000, CRC(8f20e9f7) SHA1(30ff5ad0427208e7265cb996e870c4dc0fbbf7d2) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19821.8",    0x1c00000, 0x0400000, CRC(f69a76e6) SHA1(b7e41f34d8b787bf1b4d587e5d8bddb241c043a8) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19822.9",    0x2000000, 0x0200000, CRC(5e8c4b5f) SHA1(1d146fbe3d0bfa68993135ba94ef18081ab65d31) ) // good
ROM_END

ROM_START( hanagumi )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_FILL(                              0x0000000, 0x0200000, 0x00 )
	ROM_LOAD16_WORD_SWAP( "mpr20143.7",    0x0200000, 0x0100000, CRC(7bfc38d0) SHA1(66f223e7ff2b5456a6f4185b7ab36f9cd833351a) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20138.2",    0x0400000, 0x0400000, CRC(fdcf1046) SHA1(cbb1f03879833c17feffdd6f5a4fbff06e1059a2) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20139.3",    0x0800000, 0x0400000, CRC(7f0140e5) SHA1(f2f7de7620d66a596d552e1af491a0592ebc4e51) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20140.4",    0x0c00000, 0x0400000, CRC(2fa03852) SHA1(798ce008f6fc24a00f85298188c8d0d01933640d) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20141.5",    0x1000000, 0x0400000, CRC(45d6d21b) SHA1(fe0f0b2195b74e79b8efb6a7c0b7bedca7194c48) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20142.6",    0x1400000, 0x0400000, CRC(e38561ec) SHA1(c04c400be033bc74a7bb2a60f6ae00853a2220d4) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20137.1",    0x1800000, 0x0400000, CRC(181d2688) SHA1(950059f89eda30d8a5bce145421f507e226b8b3e) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20144.8",    0x1c00000, 0x0400000, CRC(235b43f6) SHA1(e35d9bf15ac805513ab3edeca4f264647a2dc0b0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20145.9",    0x2000000, 0x0400000, CRC(aeaac7a1) SHA1(5c75ecce49a5c53dbb0b07e75f3a76e6db9976d0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20146.10",   0x2400000, 0x0400000, CRC(39bab9a2) SHA1(077132e6a03afd181ee9ca9ca4f7c9cbf418e57e) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20147.11",   0x2800000, 0x0400000, CRC(294ab997) SHA1(aeba269ae7d056f07edecf96bc138231c66c3637) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20148.12",   0x2c00000, 0x0400000, CRC(5337ccb0) SHA1(a998bb116eb10c4044410f065c5ddeb845f9dab5) ) // good
ROM_END

ROM_START( introdon )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18937.13",               0x0000000, 0x0080000, CRC(1f40d766) SHA1(35d9751c1b23cfbf448f2a9e9cf3b121929368ae) ) // ic13 bad
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr18944.7",    0x0200000, 0x0100000, CRC(f7f75ce5) SHA1(0787ece9f89cc1847889adbf08ba5d3ccbc405de) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18939.2",    0x0400000, 0x0400000, CRC(ef95a6e6) SHA1(3026c52ad542997d5b0e621b389c0e01240cb486) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18940.3",    0x0800000, 0x0400000, CRC(cabab4cd) SHA1(b251609573c4b0ccc933188f32226855b25fd9da) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18941.4",    0x0c00000, 0x0400000, CRC(f4a33a20) SHA1(bf0f33495fb5c9de4ae5036cedda65b3ece217e8) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18942.5",    0x1000000, 0x0400000, CRC(8dd0a446) SHA1(a75e3552b0fb99e0b253c0906f62fabcf204b735) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18943.6",    0x1400000, 0x0400000, CRC(d8702a9e) SHA1(960dd3cb0b9eb1f18b8d0bc0da532b600d583ceb) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18938.1",    0x1800000, 0x0400000, CRC(580ecb83) SHA1(6c59f7da408b53f9fa7aa32c1b53328b5fd6334d) ) // good
ROM_END

/* set system to 1 player to test rom */
ROM_START( kiwames )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18737.13",               0x0000000, 0x0080000, CRC(cfad6c49) SHA1(fc69980a351ed13307706db506c79c774eabeb66) ) // bad
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr18738.2",    0x0400000, 0x0400000, CRC(4b3c175a) SHA1(b6d2438ae1d3d51950a7ed1eaadf2dae45c4e7b1) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18739.3",    0x0800000, 0x0400000, CRC(eb41fa67) SHA1(d12acebb1df9eafd17aff1841087f5017225e7e7) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18740.4",    0x0c00000, 0x0200000, CRC(9ca7962f) SHA1(a09e0db2246b34ca7efa3165afbc5ba292a95398) ) // good
ROM_END

ROM_START( maruchan )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20416.13",               0x0000000, 0x0100000, CRC(8bf0176d) SHA1(5bd468e2ffed042ee84e2ceb8712ff5883a1d824) ) // bad
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr20417.2",    0x0400000, 0x0400000, CRC(636c2a08) SHA1(47986b71d68f6a1852e4e2b03ca7b6e48e83718b) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20418.3",    0x0800000, 0x0400000, CRC(3f0d9e34) SHA1(2ec81e40ebf689d17b6421820bfb0a1280a8ef25) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20419.4",    0x0c00000, 0x0400000, CRC(ec969815) SHA1(b59782174051f5717b06f43e57dd8a2a6910d95f) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20420.5",    0x1000000, 0x0400000, CRC(f2902c88) SHA1(df81e137e8aa4bd37e1d14fce4d593cfd14608f0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20421.6",    0x1400000, 0x0400000, CRC(cd0b477c) SHA1(5169cc47fae465b11bc50f5e8410d84c2b2eee42) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20422.1",    0x1800000, 0x0400000, CRC(66335049) SHA1(59f1968001d1e9fe30990a56309bae18033eee62) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20423.8",    0x1c00000, 0x0400000, CRC(2bd55832) SHA1(1a1a510f30882d4d726b594a6541a12c552fafb4) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20443.9",    0x2000000, 0x0400000, CRC(8ac288f5) SHA1(0c08874e6ab2b07b17438721fb535434a626115f) ) // good
ROM_END

/* set system to 1 player to test rom */
ROM_START( myfairld )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_FILL(                              0x0000000, 0x0200000, 0x00 )
	ROM_LOAD16_WORD_SWAP( "mpr21000.7",    0x0200000, 0x0200000, CRC(2581c560) SHA1(5fb64f0e09583d50dfea7ad613d45aad30b677a5) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20995.2",    0x0400000, 0x0400000, CRC(1bb73f24) SHA1(8773654810de760c5dffbb561f43e259b074a61b) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20996.3",    0x0800000, 0x0400000, CRC(993c3859) SHA1(93f95e3e080a08961784482607919c1ab3eeb5e5) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20997.4",    0x0c00000, 0x0400000, CRC(f0bf64a4) SHA1(f51431f1a736bbc498fa0baa1f8570f89984d9f9) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20998.5",    0x1000000, 0x0400000, CRC(d3b19786) SHA1(1933e57272cd68cc323922fa93a9af97dcef8450) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20999.6",    0x1400000, 0x0400000, CRC(82e31f25) SHA1(0cf74af14abb6ede21d19bc22041214232751594) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20994.1",    0x1800000, 0x0400000, CRC(a69243a0) SHA1(e5a1b6ec62bdd5b015ed6cf48f5a6aabaf4bd837) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr21001.8",    0x1c00000, 0x0400000, CRC(95fbe549) SHA1(8cfb48f353b2849600373d66f293f103bca700df) ) // good
ROM_END

ROM_START( othellos )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_FILL(                              0x0000000, 0x0200000, 0x00 )
	ROM_LOAD16_WORD_SWAP( "mpr20967.7",    0x0200000, 0x0200000, CRC(efc05b97) SHA1(a533366c3aaba90dcac8f3654db9ad902efca258) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20963.2",    0x0400000, 0x0400000, CRC(2cc4f141) SHA1(8bd1998aff8615b34d119fab3637a08ed6e8e1e4) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20964.3",    0x0800000, 0x0400000, CRC(5f5cda94) SHA1(616be219a2512e80c875eddf05137c23aedf6f65) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20965.4",    0x0c00000, 0x0400000, CRC(37044f3e) SHA1(cbc071554cfd8bb12a337c04b169de6c6309c3ab) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20966.5",    0x1000000, 0x0400000, CRC(b94b83de) SHA1(ba1b3135d0ad057f0786f94c9d06b5e347bedea8) ) // good
ROM_END

ROM_START( pblbeach )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18852.13",               0x0000000, 0x0080000, CRC(d12414ec) SHA1(0f42ec9e41983781b6892622b00398a102072aa7) ) // bad
	ROM_RELOAD ( 0x0080000, 0x0080000 )
	ROM_RELOAD ( 0x0100000, 0x0080000 )
	ROM_RELOAD ( 0x0180000, 0x0080000 )
	ROM_RELOAD ( 0x0200000, 0x0080000 )
	ROM_RELOAD ( 0x0280000, 0x0080000 )
	ROM_RELOAD ( 0x0300000, 0x0080000 )
	ROM_RELOAD ( 0x0380000, 0x0080000 )
	ROM_LOAD16_WORD_SWAP( "mpr18853.2",    0x0400000, 0x0400000, CRC(b9268c97) SHA1(8734e3f0e6b2849d173e3acc9d0308084a4e84fd) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18854.3",    0x0800000, 0x0400000, CRC(3113c8bc) SHA1(4e4600646ddd1978988d27430ffdf0d1d405b804) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18855.4",    0x0c00000, 0x0400000, CRC(daf6ad0c) SHA1(2a14a6a42e4eb68abb7a427e43062dfde2d13c5c) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18856.5",    0x1000000, 0x0400000, CRC(214cef24) SHA1(f62b462170b377cff16bb6c6126cbba00b013a87) ) // good
ROM_END

ROM_START( prikura )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_FILL(                              0x0000000, 0x0200000, 0x00 )
	ROM_LOAD16_WORD_SWAP( "mpr19337.7",    0x0200000, 0x0200000, CRC(76f69ff3) SHA1(5af2e1eb3288d70c2a1c71d0b6370125d65c7757) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19333.2",    0x0400000, 0x0400000, CRC(eb57a6a6) SHA1(cdacaa7a2fb1a343195e2ac5fd02eabf27f89ccd) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19334.3",    0x0800000, 0x0400000, CRC(c9979981) SHA1(be491a4ac118d5025d6a6f2d9267a6d52f21d2b6) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19335.4",    0x0c00000, 0x0400000, CRC(9e000140) SHA1(9b7dc3dc7f9dc048d2fcbc2b44ae79a631ceb381) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19336.5",    0x1000000, 0x0400000, CRC(2363fa4b) SHA1(f45e53352520be4ea313eeab87bcab83f479d5a8) ) // good
ROM_END

ROM_START( puyosun )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr19531.13",               0x0000000, 0x0080000, CRC(ac81024f) SHA1(b22c7c1798fade7ae992ff83b138dd23e6292d3f) ) // bad
	ROM_RELOAD ( 0x0080000, 0x0080000 )
	ROM_RELOAD ( 0x0100000, 0x0080000 )
	ROM_RELOAD ( 0x0180000, 0x0080000 )
	ROM_RELOAD ( 0x0200000, 0x0080000 )
	ROM_RELOAD ( 0x0280000, 0x0080000 )
	ROM_RELOAD ( 0x0300000, 0x0080000 )
	ROM_RELOAD ( 0x0380000, 0x0080000 )
	ROM_LOAD16_WORD_SWAP( "mpr19533.2",    0x0400000, 0x0400000, CRC(17ec54ba) SHA1(d4cdc86926519291cc78980ec513e1cfc677e76e) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19534.3",    0x0800000, 0x0400000, CRC(820e4781) SHA1(7ea5626ad4e1929a5ec28a99ec12bc364df8f70d) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19535.4",    0x0c00000, 0x0400000, CRC(94fadfa4) SHA1(a7d0727cf601e00f1ea31e6bf3e591349c3f6030) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19536.5",    0x1000000, 0x0400000, CRC(5765bc9c) SHA1(b217c292e7cc8ed73a39a3ae7009bc9dd031e376) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19537.6",    0x1400000, 0x0400000, CRC(8b736686) SHA1(aec347c0f3e5dd8646e85f68d71ca9acc3bf62c3) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19532.1",    0x1800000, 0x0400000, CRC(985f0c9d) SHA1(de1ad42ef3cf3f4f071e9801696407be7ae29d21) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19538.8",    0x1c00000, 0x0400000, CRC(915a723e) SHA1(96480441a69d6aad3887ed6f46b0a6bebfb752aa) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19539.9",    0x2000000, 0x0400000, CRC(72a297e5) SHA1(679987e62118dd1bf7c074f4b88678e1a1187437) ) // good
ROM_END

ROM_START( rsgun )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_FILL(                             0x0000000, 0x0200000, 0x00 )
	ROM_LOAD16_WORD_SWAP( "mpr20958.7",   0x0200000, 0x0200000, CRC(cbe5a449) SHA1(b4744ab71ccbadda1921ba43dd1148e57c0f84c5) ) // good (was .11s)
	ROM_LOAD16_WORD_SWAP( "mpr20959.2",   0x0400000, 0x0400000, CRC(a953330b) SHA1(965274a7297cb88e281fcbdd3ec5025c6463cc7b) ) // good (was .12)
	ROM_LOAD16_WORD_SWAP( "mpr20960.3",   0x0800000, 0x0400000, CRC(b5ab9053) SHA1(87c5d077eb1219c35fa65b4e11d5b62e826f5236) ) // good (was .13)
	ROM_LOAD16_WORD_SWAP( "mpr20961.4",   0x0c00000, 0x0400000, CRC(0e06295c) SHA1(0ec2842622f3e9dc5689abd58aeddc7e5603b97a) ) // good (was .14)
	ROM_LOAD16_WORD_SWAP( "mpr20962.5",   0x1000000, 0x0400000, CRC(f1e6c7fc) SHA1(0ba0972f1bc7c56f4e0589d3e363523cea988bb0) ) // good (was .15)
	/*Without the following the game crashes,maybe we need to mirror the previous roms in this location?*/
	ROM_FILL(                             0x1400000, 0x0c00000, 0x00 )
ROM_END

ROM_START( sandor )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "sando-r.13",               0x0000000, 0x0100000, CRC(fe63a239) SHA1(01502d4494f968443581cd2c74f25967d41f775e) ) // ic13 bad
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr18635.8",   0x1c00000, 0x0400000, CRC(441e1368) SHA1(acb2a7e8d44c2203b8d3c7a7b70e20ffb120bebf) ) // good
	ROM_RELOAD(                           0x0400000, 0x0400000 )
	ROM_LOAD16_WORD_SWAP( "mpr18636.9",   0x2000000, 0x0400000, CRC(fff1dd80) SHA1(36b8e1526a4370ae33fd4671850faf51c448bca4) ) // good
	ROM_RELOAD(                           0x0800000, 0x0400000 )
	ROM_LOAD16_WORD_SWAP( "mpr18637.10",  0x2400000, 0x0400000, CRC(83aced0f) SHA1(6cd1702b9c2655dc4f56c666607c333f62b09fc0) ) // good
	ROM_RELOAD(                           0x0c00000, 0x0400000 )
	ROM_LOAD16_WORD_SWAP( "mpr18638.11",  0x2800000, 0x0400000, CRC(caab531b) SHA1(a77bdcc27d183896c0ed576eeebcc1785d93669e) ) // good
	ROM_RELOAD(                           0x1000000, 0x0400000 )
ROM_END

ROM_START( thunt )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_FILL(                             0x0000000, 0x0200000, 0x00 )
	ROM_LOAD16_BYTE( "th-ic7_2.stv",    0x0200000, 0x0080000, CRC(c4e993de) SHA1(7aa433bc2623cb19a09d4ef4c8233a2d29901020) )
	ROM_LOAD16_BYTE( "th-ic7_1.stv",    0x0200001, 0x0080000, CRC(1355cc18) SHA1(a9b731228a807b2b01f933fe0f7dcdbadaf89b7e) )

	ROM_LOAD16_WORD_SWAP( "th-e-2.ic2",   0x0400000, 0x0400000, CRC(47315694) SHA1(4a7cc195b98dbca146f5efe3b5b35be0f13b2f4a) )
	ROM_LOAD16_WORD_SWAP( "th-e-3.ic3",   0x0800000, 0x0400000, CRC(c9290b44) SHA1(4e51e667edf330cc20e600cf1500f332cc533e20) )
	ROM_LOAD16_WORD_SWAP( "th-e-4.ic4",   0x0c00000, 0x0400000, CRC(c672e40b) SHA1(4ba189de7b8fb5ca45fb15c89cdaf0f860831d52) )
	ROM_LOAD16_WORD_SWAP( "th-e-5.ic5",   0x1000000, 0x0400000, CRC(3914b805) SHA1(1331ce82ba0bdfc76fe3456a5252e69c00e2cf1f) )
ROM_END

ROM_START( sassisu )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20542.13",               0x0000000, 0x0100000, CRC(0e632db5) SHA1(9bc52794892eec22d381387d13a0388042e30714) ) // ic13 bad
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr20544.2",    0x0400000, 0x0400000, CRC(661fff5e) SHA1(41f4ddda7adf004b52cc9a076606a60f31947d19) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20545.3",    0x0800000, 0x0400000, CRC(8e3a37be) SHA1(a3227cdc4f03bb088e7f9aed225b238da3283e01) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20546.4",    0x0c00000, 0x0400000, CRC(72020886) SHA1(e80bdeb11b726eb23f2283950d65d55e31a5672e) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20547.5",    0x1000000, 0x0400000, CRC(8362e397) SHA1(71f13689a60572a04b91417a9a48adfd3bd0f5dc) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20548.6",    0x1400000, 0x0400000, CRC(e37534d9) SHA1(79988cbb1537ca99fdd0288a86564fe1f714d052) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20543.1",    0x1800000, 0x0400000, CRC(1f688cdf) SHA1(a90c1011119adb50e0d9d5cd3d7616a307b2d7e8) ) // good
ROM_END

/* set to 1 player to test */
ROM_START( seabass )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "seabassf.13",               0x0000000, 0x0100000, CRC(6d7c39cc) SHA1(d9d1663134420b75c65ee07d7d547254785f2f83) ) // ic13 bad
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr20551.2",    0x0400000, 0x0400000, CRC(9a0c6dd8) SHA1(26600372cc673ce3678945f4b5dc4e3ab31643a4) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20552.3",    0x0800000, 0x0400000, CRC(5f46b0aa) SHA1(1aa576b15971c0ffb4e08d4802246841b31b6f35) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20553.4",    0x0c00000, 0x0400000, CRC(c0f8a6b6) SHA1(2038b9231a950450267be0db24b31d8035db79ad) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20554.5",    0x1000000, 0x0400000, CRC(215fc1f9) SHA1(f042145622ba4bbbcce5f050a4c9eae42cb7adcd) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20555.6",    0x1400000, 0x0400000, CRC(3f5186a9) SHA1(d613f307ab150a7eae358aa449206af05db5f9d7) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20550.1",    0x1800000, 0x0400000, CRC(083e1ca8) SHA1(03944dd8fe86f305ca4bd2d71e2140e03798ffc9) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20556.8",    0x1c00000, 0x0400000, CRC(1fd70c6c) SHA1(d9d2e362d13238216f4f7e10095fb8383bbd91e8) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20557.9",    0x2000000, 0x0400000, CRC(3c9ba442) SHA1(2e5b795cf4cdc11ab3e4887b2f77c7147c6e3eec) ) // good
ROM_END

ROM_START( shanhigw )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_FILL(                              0x0000000, 0x0200000, 0x00 )
	ROM_LOAD16_WORD_SWAP( "mpr18341.7",    0x0200000, 0x0200000, CRC(cc5e8646) SHA1(a733616c118140ff3887d30d595533f9a1beae06) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18340.2",    0x0400000, 0x0200000, CRC(8db23212) SHA1(85d604a5c6ab97188716dbcd77d365af12a238fe) ) // good
ROM_END

ROM_START( shienryu )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_FILL(                              0x0000000, 0x0200000, 0x00 )
	ROM_LOAD16_WORD_SWAP( "mpr19631.7",    0x0200000, 0x0200000, CRC(3a4b1abc) SHA1(3b14b7fdebd4817da32ea374c15a38c695ffeff1) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19632.2",    0x0400000, 0x0400000, CRC(985fae46) SHA1(f953bde91805b97b60d2ab9270f9d2933e064d95) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19633.3",    0x0800000, 0x0400000, CRC(e2f0b037) SHA1(97861d09e10ce5d2b10bf5559574b3f489e28077) ) // good
ROM_END

ROM_START( smleague )
	STV_BIOS // must use USA
	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr18777.13",               0x0000000, 0x0080000, CRC(8d180866) SHA1(d47ebabab6e06400312d39f68cd818852e496b96) ) // ic13 bad
	ROM_RELOAD ( 0x0080000, 0x0080000 )
	ROM_RELOAD ( 0x0100000, 0x0080000 )
	ROM_RELOAD ( 0x0180000, 0x0080000 )
	ROM_RELOAD ( 0x0200000, 0x0080000 )
	ROM_RELOAD ( 0x0280000, 0x0080000 )
	ROM_RELOAD ( 0x0300000, 0x0080000 )
	ROM_RELOAD ( 0x0380000, 0x0080000 )
	ROM_LOAD16_WORD_SWAP( "mpr18778.8",    0x1c00000, 0x0400000, CRC(25e1300e) SHA1(64f3843f62cee34a47244ad5ee78fb2aa35289e3) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18779.9",    0x2000000, 0x0400000, CRC(51e2fabd) SHA1(3aa361149af516f16d7d422596ee82014a183c2b) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18780.10",   0x2400000, 0x0400000, CRC(8cd4dd74) SHA1(9ffec1280b3965d52f643894bdfecdd792028191) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18781.11",   0x2800000, 0x0400000, CRC(13ee41ae) SHA1(cdbaeac4c90b5ee84233c299612f7f28280a6ba6) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18782.12",   0x2c00000, 0x0200000, CRC(9be2270a) SHA1(f2de5cd6b269f123305e30bed2b474019e4f05b8) ) // good
ROM_END

ROM_START( sokyugrt )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr19188.13",               0x0000000, 0x0100000, CRC(45a27e32) SHA1(96e1bab8bdadf7071afac2a0a6dd8fd8989f12a6) ) // ic13 bad
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr19189.2",    0x0400000, 0x0400000, CRC(0b202a3e) SHA1(6691b5af2cacd6092ec03886b78c2565953fa297) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19190.3",    0x0800000, 0x0400000, CRC(1777ded8) SHA1(dd332ac79f0a6d82b6bde35b795b2845003dd1a5) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19191.4",    0x0c00000, 0x0400000, CRC(ec6eb07b) SHA1(01fe4832ece8638ea6f4060099d9105fe8092c88) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19192.5",    0x1000000, 0x0200000, CRC(cb544a1e) SHA1(eb3ba9758487d0e8c4bbfc41453fe35b35cce3bf) ) // good
ROM_END

/* set to 1 player to test */
ROM_START( sss )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr21488.13",               0x0000000, 0x0080000, CRC(71c9def1) SHA1(a544a0b4046307172d2c1bf426ed24845f87d894) ) // ic13 bad (was .24)
	ROM_RELOAD ( 0x0080000, 0x0080000 )
	ROM_RELOAD ( 0x0100000, 0x0080000 )
	ROM_RELOAD ( 0x0180000, 0x0080000 )
	ROM_RELOAD ( 0x0200000, 0x0080000 )
	ROM_RELOAD ( 0x0280000, 0x0080000 )
	ROM_RELOAD ( 0x0300000, 0x0080000 )
	ROM_RELOAD ( 0x0380000, 0x0080000 )
	ROM_LOAD16_WORD_SWAP( "mpr21489.2",    0x0400000, 0x0400000, CRC(4c85152b) SHA1(78f2f1c31718d5bf631d8813daf9a11ea2a0e451) ) // ic2 good (was .12)
	ROM_LOAD16_WORD_SWAP( "mpr21490.3",    0x0800000, 0x0400000, CRC(03da67f8) SHA1(02f9ba7549ca552291dc0ff1b631103015838bba) ) // ic3 good (was .13)
	ROM_LOAD16_WORD_SWAP( "mpr21491.4",    0x0c00000, 0x0400000, CRC(cf7ee784) SHA1(af823df2d60d8ef3d17628b95a04136b807ca095) ) // ic4 good (was .14)
	ROM_LOAD16_WORD_SWAP( "mpr21492.5",    0x1000000, 0x0400000, CRC(57753894) SHA1(5c51167c158443d02a53d724a5ceb73055876c06) ) // ic5 good (was .15)
	ROM_LOAD16_WORD_SWAP( "mpr21493.6",    0x1400000, 0x0400000, CRC(efb2d271) SHA1(a591e48206704fbda5fef3ce69ad279da1017ed6) ) // ic6 good (was .16)
ROM_END

ROM_START( suikoenb )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr17834.13",               0x0000000, 0x0100000, CRC(746ef686) SHA1(e31c317991a687662a8a2a45aed411001e5f1941) ) // ic13 bad
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr17836.2",    0x0400000, 0x0400000, CRC(55e9642d) SHA1(5198291cd1dce0398eb47760db2c19eae99273b0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17837.3",    0x0800000, 0x0400000, CRC(13d1e667) SHA1(cd513ceb33cc20032090113b61227638cf3b3998) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17838.4",    0x0c00000, 0x0400000, CRC(f9e70032) SHA1(8efdbcce01bdf77acfdb293545c59bf224a9c7d2) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17839.5",    0x1000000, 0x0400000, CRC(1b2762c5) SHA1(5c7d5fc8a4705249a5b0ea64d51dc3dc95d723f5) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17840.6",    0x1400000, 0x0400000, CRC(0fd4c857) SHA1(42caf22716e834d59e60d45c24f51d95734e63ae) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17835.1",    0x1800000, 0x0400000, CRC(77f5cb43) SHA1(a4f54bc08d73a56caee5b26bea06360568655bd7) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17841.8",    0x1c00000, 0x0400000, CRC(f48beffc) SHA1(92f1730a206f4a0abf7fb0ee1210e083a464ad70) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17842.9",    0x2000000, 0x0400000, CRC(ac8deed7) SHA1(370eb2216b8080d3ddadbd32804db63c4ebac76f) ) // good
ROM_END

ROM_START( twcup98 )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20819.13",    0x0000000, 0x0100000, CRC(d930dfc8) SHA1(f66cc955181720661a0334fe67fa5750ddf9758b) ) // ic13 bad (was .24)
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr20821.2",    0x0400000, 0x0400000, CRC(2d930d23) SHA1(5fcaf4257f3639cb3aa407d2936f616499a09d97) ) // ic2 good (was .12)
	ROM_LOAD16_WORD_SWAP( "mpr20822.3",    0x0800000, 0x0400000, CRC(8b33a5e2) SHA1(d5689ac8aad63509febe9aa4077351be09b2d8d4) ) // ic3 good (was .13)
	ROM_LOAD16_WORD_SWAP( "mpr20823.4",    0x0c00000, 0x0400000, CRC(6e6d4e95) SHA1(c387d03ba27580c62ac0bf780915fdf41552df6f) ) // ic4 good (was .14)
	ROM_LOAD16_WORD_SWAP( "mpr20824.5",    0x1000000, 0x0400000, CRC(4cf18a25) SHA1(310961a5f114fea8938a3f514dffd5231e910a5a) ) // ic5 good (was .15)
ROM_END

ROM_START( vfkids )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr18914.13",               0x0000000, 0x0100000, CRC(cd35730a) SHA1(645b52b449766beb740ab8f99957f8f431351ceb) ) // ic13 bad
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr18916.4",    0x0c00000, 0x0400000, CRC(4aae3ddb) SHA1(b75479e73f1bce3f0c27fbd90820fa51eb1914a6) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18917.5",    0x1000000, 0x0400000, CRC(edf6edc3) SHA1(478e958f4f10a8126a00c83feca4a55ad6c25503) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18918.6",    0x1400000, 0x0400000, CRC(d3a95036) SHA1(e300bbbb71fb06027dc539c9bbb12946770ffc95) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18915.1",    0x1800000, 0x0400000, CRC(09cc38e5) SHA1(4dfe0e2f21f746020ec557e62487aa7558cbc1fd) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18919.8",    0x1c00000, 0x0400000, CRC(4ac700de) SHA1(b1a8501f1683de380dfa49c9cabbe28bd70a5b26) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18920.9",    0x2000000, 0x0400000, CRC(0106e36c) SHA1(f7c30dc9fedb9da079dd7d52fdecbeb8721c5dee) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18921.10",   0x2400000, 0x0400000, CRC(c23d51ad) SHA1(0169b7e2df84e8caa2b349843bd0673f6de2195f) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18922.11",   0x2800000, 0x0400000, CRC(99d0ab90) SHA1(e9c82a826cc76ffbe2423913645cf5d5ba2506d6) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr18923.12",   0x2c00000, 0x0400000, CRC(30a41ae9) SHA1(78a3d88b5e6cf669b660460ac967daf408038883) ) // good
ROM_END

ROM_START( vfremix )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr17944.13",               0x0000000, 0x0100000, CRC(a5bdc560) SHA1(d3830480a611b7d88760c672ce46a2ea74076487) ) // ic13 bad
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr17946.2",    0x0400000, 0x0400000, CRC(4cb245f7) SHA1(363d9936b27043b5858c956a45736ac05aefc54e) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17947.3",    0x0800000, 0x0400000, CRC(fef4a9fb) SHA1(1b4bd095962db769da17d3644df10f62d041e914) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17948.4",    0x0c00000, 0x0400000, CRC(3e2b251a) SHA1(be6191c18727d7cbc6399fd4c1aaae59304af30c) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17949.5",    0x1000000, 0x0400000, CRC(b2ecea25) SHA1(320c0e7ce34e81e2fe6400cbeb2cb3ca74426cc8) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17950.6",    0x1400000, 0x0400000, CRC(5b1f981d) SHA1(693b5744d210a2ac8b77e7c8c87f07ca859f8aed) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr17945.1",    0x1800000, 0x0200000, CRC(03ede188) SHA1(849c7fab5b97e043fea3deb8df6cc195ccced0e0) ) // good
ROM_END

/* set to 1 player to test */
ROM_START( vmahjong )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_FILL(                              0x0000000, 0x0200000, 0x00 )
	ROM_LOAD16_WORD_SWAP( "mpr19620.7",    0x0200000, 0x0200000, CRC(c98de7e5) SHA1(5346f884793bcb080aa01967e91b54ced4a9802f) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19615.2",    0x0400000, 0x0400000, CRC(c62896da) SHA1(52a5b10ca8af31295d2d700349eca038c418b522) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19616.3",    0x0800000, 0x0400000, CRC(f62207c7) SHA1(87e60183365c6f7e62c7a0667f88df0c7f5457fd) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19617.4",    0x0c00000, 0x0400000, CRC(ab667e19) SHA1(2608a567888fe052753d0679d9a831d7706dbc86) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19618.5",    0x1000000, 0x0400000, CRC(9782ceee) SHA1(405dd42706416e128b1e2fde225b5343e9330092) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19619.6",    0x1400000, 0x0400000, CRC(0b76866c) SHA1(10add2993dfe9daf757ec2ff8675390081a93c0a) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19614.1",    0x1800000, 0x0400000, CRC(b83b3f03) SHA1(e5a5919ee74964633eaaf4af2fe04c38604ccf16) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr19621.8",    0x1c00000, 0x0400000, CRC(f92616b3) SHA1(61a9dda92a86a02d027260e11b1bad3b0dda9f02) ) // good
ROM_END

ROM_START( winterht )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "fpr20108.13",    0x0000000, 0x0100000, CRC(1ef9ced0) SHA1(abc90ce341cd17bb77349d611d6879389611f0bf) ) // bad
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr20110.2",    0x0400000, 0x0400000, CRC(238ef832) SHA1(20fade5730ff8e249a1450c41bfdff6e133f4768) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20111.3",    0x0800000, 0x0400000, CRC(b0a86f69) SHA1(e66427f70413ad43fccc38423962c5eeda01094f) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20112.4",    0x0c00000, 0x0400000, CRC(3ba2b49b) SHA1(5ad154a8b774075479d791e29cbaf221d47557fc) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20113.5",    0x1000000, 0x0400000, CRC(8c858b41) SHA1(d05d2980363c8440863fe2fdb39274de246bd4b9) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20114.6",    0x1400000, 0x0400000, CRC(b723862c) SHA1(1e0a08669f16fc4cb647124e0c215233ccb98e5a) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20109.1",    0x1800000, 0x0400000, CRC(c1a713b8) SHA1(a7fefa6e9a1e3aecff5ead41da6fd3aec2ef502a) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20115.8",    0x1c00000, 0x0400000, CRC(dd01f2ad) SHA1(3bb48dc8670d9460fea2a67400ddb573472c2f4f) ) // good
ROM_END

ROM_START( znpwfv )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr20398.13",    0x0000000, 0x0100000, CRC(3fb56a0b) SHA1(13c2fa2d94b106d39e46f71d15fbce3607a5965a) ) // bad
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr20400.2",    0x0400000, 0x0400000, CRC(1edfbe05) SHA1(b0edd3f3d57408101ae6eb0aec742afbb4d289ca) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20401.3",    0x0800000, 0x0400000, CRC(99e98937) SHA1(e1b4d12a0b4d0fe97a62fcc085e19cce77657c99) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20402.4",    0x0c00000, 0x0400000, CRC(4572aa60) SHA1(8b2d76ea8c6e2f472c6ee7c9b6ad6e80e6a1a85a) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20403.5",    0x1000000, 0x0400000, CRC(26a8e13e) SHA1(07f5564b704598e3c3580d3d620ecc4f14549dbd) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20404.6",    0x1400000, 0x0400000, CRC(0b70275d) SHA1(47b8672e19c698dc948760f7091f4c6280e728d0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20399.1",    0x1800000, 0x0400000, CRC(c178a96e) SHA1(65f4aa05187d48ba8ad4fe75ff6ffe1f8524831d) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20405.8",    0x1c00000, 0x0400000, CRC(f53337b7) SHA1(09a21f81016ee54f10554ae1f790415d7436afe0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20406.9",    0x2000000, 0x0400000, CRC(b677c175) SHA1(d0de7b5a29928036df0bdfced5a8021c0999eb26) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr20407.10",   0x2400000, 0x0400000, CRC(58356050) SHA1(f8fb5a14f4ec516093c785891b05d55ae345754e) ) // good
ROM_END

ROM_START( danchih )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_FILL(                              0x0000000, 0x0200000, 0x00 )
	ROM_LOAD16_WORD_SWAP( "mpr21974.7",    0x0200000, 0x0200000, CRC(e7472793) SHA1(11b7b11cf492eb9cf69b50e7cfac46a5b86849ac) )// good
	ROM_LOAD16_WORD_SWAP( "mpr21970.2",    0x0400000, 0x0400000, CRC(34dd7f4d) SHA1(d5c45da94ec5b6584049caf09516f1ad4ba3adb5) )// good
	ROM_LOAD16_WORD_SWAP( "mpr21971.3",    0x0800000, 0x0400000, CRC(8995158c) SHA1(fbbd171d67eebf43630d6054bc1b9132f6b38183) )// good
	ROM_LOAD16_WORD_SWAP( "mpr21972.4",    0x0c00000, 0x0400000, CRC(68a39090) SHA1(cff1b909c4191660570012eb5e4cb6a7467bc79e) )// good
	ROM_LOAD16_WORD_SWAP( "mpr21973.5",    0x1000000, 0x0400000, CRC(b0f23f14) SHA1(4e7076c29fd57bb3ef9af50a6104e39ecda94e06) )// good
ROM_END

ROM_START( mausuke )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD(             "ic13.bin",      0x0000000, 0x0100000, CRC(b456f4cd) SHA1(91cbe703ec7c1dd45eb3b05bdfeb06e3570599d1) )
	/* mirroring is essential on this one */
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )

	ROM_LOAD16_WORD_SWAP( "mcj-00.2",      0x0400000, 0x0200000, CRC(4eeacd6f) SHA1(104ca230f22cd11cc536b34abd482e54791b4d0f) )// good
	ROM_LOAD16_WORD_SWAP( "mcj-01.3",      0x0800000, 0x0200000, CRC(365a494b) SHA1(29713dfc83a9ade63ebcc7994d14cd785c4500b9) )// good
	ROM_LOAD16_WORD_SWAP( "mcj-02.4",      0x0c00000, 0x0200000, CRC(8b8e4931) SHA1(0c94e2ccb72902d7786d1101a3958504f7151077) )// good
	ROM_LOAD16_WORD_SWAP( "mcj-03.5",      0x1000000, 0x0200000, CRC(9015a0e7) SHA1(8ba8a3723267e631169dc1e06620260fbccce4bd) )// good
	ROM_LOAD16_WORD_SWAP( "mcj-04.6",      0x1400000, 0x0200000, CRC(9d1beaee) SHA1(c63b61378860319fff2e605c7b9afaa5f1bc4cd2) )// good
	ROM_LOAD16_WORD_SWAP( "mcj-05.1",      0x1800000, 0x0200000, CRC(a7626a82) SHA1(c12a099132c5b9234a2de5674f3b8ba5fdd35289) )// good
	ROM_LOAD16_WORD_SWAP( "mcj-06.8",      0x1c00000, 0x0200000, CRC(1ab8e90e) SHA1(8e22f03c1791a983eb330b2a9199e5349a0b1baa) )// good
ROM_END

/* acclaim game, not a standard cart ... */
ROM_START( batmanfr )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	/* Many thanks to Runik to point this out*/
	ROM_LOAD16_BYTE( "350-mpa1.u19",    0x0000000, 0x0100000, CRC(2a5a8c3a) SHA1(374ec55a39ea909cc672e4a629422681d1f2da05) )
	ROM_RELOAD(                         0x0200000, 0x0100000 )
	ROM_LOAD16_BYTE( "350-mpa1.u16",    0x0000001, 0x0100000, CRC(735e23ab) SHA1(133e2284a07a611aed8ada2707248f392f4509aa) )
	ROM_RELOAD(                         0x0200001, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "gfx0.u1",    0x0400000, 0x0400000, CRC(a82d0b7e) SHA1(37a7a177634d51620b1b43e58732987df166c7e6) )
	ROM_LOAD16_WORD_SWAP( "gfx1.u3",    0x0800000, 0x0400000, CRC(a41e55d9) SHA1(b896d3a6c36d325c3cece699da54f340a4512703) )
	ROM_LOAD16_WORD_SWAP( "gfx2.u5",    0x0c00000, 0x0400000, CRC(4c1ebeb7) SHA1(cdd139652d9484ae5837a39c2fd48d0a8d966d43) )
	ROM_LOAD16_WORD_SWAP( "gfx3.u8",    0x1000000, 0x0400000, CRC(f679a3e7) SHA1(db11b033b8bbdd80b81e3bc098bd40ad3a8784f2) )
	ROM_LOAD16_WORD_SWAP( "gfx4.u12",   0x1400000, 0x0400000, CRC(52d95242) SHA1(b554a95933c2be4c72fb4226d3bc4775695da2c1) )
	ROM_LOAD16_WORD_SWAP( "gfx5.u15",   0x1800000, 0x0400000, CRC(e201f830) SHA1(5aa22fcc8f2e153d1abc3aa4050c594b3942ee67) )
	ROM_LOAD16_WORD_SWAP( "gfx6.u18",   0x1c00000, 0x0400000, CRC(c6b381a3) SHA1(46431f1e47c084a0bf85535d35af27471653b008) )

	/* it also has an extra adsp sound board, i guess this isn't tested */
	ROM_REGION( 0x080000, REGION_USER2, 0 ) /* ADSP code */
	ROM_LOAD( "350snda1.u52",   0x000000, 0x080000, CRC(9027e7a0) SHA1(678df530838b078964a044ce734776f391654e6c) )

	ROM_REGION( 0x800000, REGION_USER3, 0 ) /* Sound */
	ROM_LOAD( "snd0.u48",   0x000000, 0x200000, CRC(02b1927c) SHA1(08b21d8b31b0f15c59fb5bb7eaf425e6fe04f7b5) )
	ROM_LOAD( "snd1.u49",   0x200000, 0x200000, CRC(58b18eda) SHA1(7f3105fe04d9c0cdfd76e3323f623a4d0f7dad06) )
	ROM_LOAD( "snd2.u50",   0x400000, 0x200000, CRC(51d626d6) SHA1(0e68b79dcb653dcba48121ca2d4f692f90afa85e) )
	ROM_LOAD( "snd3.u51",   0x600000, 0x200000, CRC(31af26ae) SHA1(2c9f4c078afec55964b5c2a4d00f5c43f2661a04) )
ROM_END

/*
Critter Crusher EXP
Sega, 1995.

This is a cart for Sega STV system. The game involves hitting 'critters' on
screen with a rubber/plastic hammer. The cab has a large LED display to show how many
'critters' were hit. The hammer positioning might work like a lightgun and the 'hitting'
may be like pulling the lightgun trigger?

The ROM cart appears to be a re-used Virtua Fighter Remix cart??
On top there is one 27C040 EPROM, EPR-18821 @ IC13
6 maskROMs, MPR-17945 to MPR-17950 (already dumped, known Virtua Fighter Remix ROMs)
On the other side of the PCB are 2 more maskROMs, MPR-18788 @ IC9 and MPR-18789 @ IC8

*/

ROM_START( critcrsh )
	STV_BIOS

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD( "epr-18821.ic13",  0x0000000, 0x0080000, CRC(9a6658e2) SHA1(16dbae3d9ab584713afcb403f89fe71049609245) )
	ROM_RELOAD ( 0x0080000, 0x0080000 )
	ROM_RELOAD ( 0x0100000, 0x0080000 )
	ROM_RELOAD ( 0x0180000, 0x0080000 )
	ROM_RELOAD ( 0x0200000, 0x0080000 )
	ROM_RELOAD ( 0x0280000, 0x0080000 )
	ROM_RELOAD ( 0x0300000, 0x0080000 )
	ROM_RELOAD ( 0x0380000, 0x0080000 )
//  ROM_LOAD16_WORD_SWAP( "mpr17946.2",    0x0400000, 0x0400000, CRC(4cb245f7) SHA1(363d9936b27043b5858c956a45736ac05aefc54e) ) // good
//  ROM_LOAD16_WORD_SWAP( "mpr17947.3",    0x0800000, 0x0400000, CRC(fef4a9fb) SHA1(1b4bd095962db769da17d3644df10f62d041e914) ) // good
//  ROM_LOAD16_WORD_SWAP( "mpr17948.4",    0x0c00000, 0x0400000, CRC(3e2b251a) SHA1(be6191c18727d7cbc6399fd4c1aaae59304af30c) ) // good
//  ROM_LOAD16_WORD_SWAP( "mpr17949.5",    0x1000000, 0x0400000, CRC(b2ecea25) SHA1(320c0e7ce34e81e2fe6400cbeb2cb3ca74426cc8) ) // good
//  ROM_LOAD16_WORD_SWAP( "mpr17950.6",    0x1400000, 0x0400000, CRC(5b1f981d) SHA1(693b5744d210a2ac8b77e7c8c87f07ca859f8aed) ) // good
//  ROM_LOAD16_WORD_SWAP( "mpr17945.1",    0x1800000, 0x0200000, CRC(03ede188) SHA1(849c7fab5b97e043fea3deb8df6cc195ccced0e0) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr-18789.ic8", 0x1c00000, 0x0400000, CRC(b388616f) SHA1(0b2c5a547c3a6a8fb9f4ca54336cf6dc9adb8c6a) ) // good
	ROM_LOAD16_WORD_SWAP( "mpr-18788.ic9", 0x2000000, 0x0400000, CRC(feae5867) SHA1(7d2e47d5ab18700a246d53fdb7872a905cdac55a) ) // good
ROM_END

ROM_START( sfish2 )
//  STV_BIOS // - sports fishing 2 uses its own bios

	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr18343.bin",   0x000000, 0x080000, CRC(48e2eecf) SHA1(a38bfbd5f279525e413b18b5ed3f37f6e9e31cdc) ) /* sport fishing 2 bios */
	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* SH2 code */
	ROM_COPY( REGION_CPU1,0,0,0x080000)

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD(             "epr18427.bin",      0x0000000, 0x0100000, CRC(3f25bec8) SHA1(43a5342b882d5aec0f35a8777cb475659f43b1c4) )
	ROM_LOAD16_WORD_SWAP( "mpr18273.ic2",	   0x0400000, 0x0200000, NO_DUMP )
	ROM_LOAD16_WORD_SWAP( "mpr18274.ic3",      0x0800000, 0x0200000, NO_DUMP )
	ROM_LOAD16_WORD_SWAP( "mpr18275.ic4",      0x0c00000, 0x0200000, NO_DUMP )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "sfish2", 0, MD5(3c95f7e19c5475d0ffa0d89cc45a36e5) SHA1(d07e0aa09a0858b308cc2218996b2242885a2609) )
ROM_END

ROM_START( sfish2j )
//  STV_BIOS // - sports fishing 2 uses its own bios

	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD16_WORD_SWAP( "epr18343.bin",   0x000000, 0x080000, CRC(48e2eecf) SHA1(a38bfbd5f279525e413b18b5ed3f37f6e9e31cdc) ) /* sport fishing 2 bios */
	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* SH2 code */
	ROM_COPY( REGION_CPU1,0,0,0x080000)

	ROM_REGION32_BE( 0x3000000, REGION_USER1, 0 ) /* SH2 code */
	ROM_LOAD(             "epr18344.a",      0x0000000, 0x0100000,  CRC(5a7de018) SHA1(88e0c2a9a9d4ebf699878c0aa9737af85f95ccf8) )
	ROM_RELOAD ( 0x0100000, 0x0100000 )
	ROM_RELOAD ( 0x0200000, 0x0100000 )
	ROM_RELOAD ( 0x0300000, 0x0100000 )
	ROM_LOAD16_WORD_SWAP( "mpr18273.ic2",	   0x0400000, 0x0200000, NO_DUMP )
	ROM_LOAD16_WORD_SWAP( "mpr18274.ic3",      0x0800000, 0x0200000, NO_DUMP )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "sfish2", 0, MD5(3c95f7e19c5475d0ffa0d89cc45a36e5) SHA1(d07e0aa09a0858b308cc2218996b2242885a2609) )
ROM_END



DRIVER_INIT( sfish2 )
{
	/* this is WRONG but works for some games */
	UINT32 *rom = (UINT32 *)memory_region(REGION_USER1);
	rom[0xf10/4] = (rom[0xf10/4] & 0xff000000)|((rom[0xf10/4]/2)&0x00ffffff);
	rom[0xf20/4] = (rom[0xf20/4] & 0xff000000)|((rom[0xf20/4]/2)&0x00ffffff);
	rom[0xf30/4] = (rom[0xf30/4] & 0xff000000)|((rom[0xf30/4]/2)&0x00ffffff);
	init_stv();
}

DRIVER_INIT( sfish2j )
{
	/* this is WRONG but works for some games */
	UINT32 *rom = (UINT32 *)memory_region(REGION_USER1);
	rom[0xf10/4] = (rom[0xf10/4] & 0xff000000)|((rom[0xf10/4]/2)&0x00ffffff);
	rom[0xf20/4] = (rom[0xf20/4] & 0xff000000)|((rom[0xf20/4]/2)&0x00ffffff);
	rom[0xf30/4] = (rom[0xf30/4] & 0xff000000)|((rom[0xf30/4]/2)&0x00ffffff);
	init_stv();
}


/*
country codes:
J = Japan
U = United States
E = Europe
T = Taiwan
B = Brazil
K = Korea
A = PAL Asia
L = Latin America
date codes:
(Original is yyyy/mm/dd,changed to reflect Capcom's one (yy/mm/dd))
Version codes:
V = Version(obviously ;)
(number before the dot) = game status (0=Sample,1=Master,2=Upgrade)
(number after the dot) = game version/release
There is also another internal code (called the "Product Number"),but AFAIK it's only used
by introdon in ST-V ("SG0000000"),and according to the manual it's even wrong! (SG is used
by Sega titles,and this is a Sunsoft game)It's likely to be a left-over...
*/

GAMEB( 1996, stvbios,   0,       stvbios, stv, stv,  stv,       ROT0,   "Sega",                      "ST-V Bios", NOT_A_DRIVER )

//GBX   YEAR, NAME,      PARENT,  BIOS,    MACH,INP,  INIT,      MONITOR
/* Playable */
GAMEB( 1996, bakubaku,  stvbios, stvbios, stv, stv,  bakubaku,  ROT0,   "Sega",     				  "Baku Baku Animal (J 950407 V1.000)", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1996, batmanfr,  stvbios, stvbios, stv, stv,  batmanfr,  ROT0,   "Acclaim",    				  "Batman Forever (JUE 960507 V1.000)", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1996, colmns97,  stvbios, stvbios, stv, stv,  colmns97,  ROT0,   "Sega", 	 				  "Columns '97 (JET 961209 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_NOT_WORKING ) // graphics issues
GAMEB( 1997, cotton2,   stvbios, stvbios, stv, stv,  cotton2,   ROT0,   "Success",  				  "Cotton 2 (JUET 970902 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1998, cottonbm,  stvbios, stvbios, stv, stv,  cottonbm,  ROT0,   "Success",  				  "Cotton Boomerang (JUET 980709 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1999, danchih,   stvbios, stvbios, stv, stvmp,danchih,   ROT0,   "Altron (Tecmo license)", 	  "Danchi de Hanafuoda (J 990607 V1.400)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1996, diehard,   stvbios, stvbios, stv, stv,  diehard,   ROT0,   "Sega", 	 				  "Die Hard Arcade (UET 960515 V1.000)", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS  )
GAMEB( 1996, dnmtdeka,  diehard, stvbios, stv, stv,  dnmtdeka,  ROT0,   "Sega", 	 				  "Dynamite Deka (J 960515 V1.000)", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS  )
GAMEB( 1995, ejihon,    stvbios, stvbios, stv, stv,  ic13,      ROT0,   "Sega", 	 				  "Ejihon Tantei Jimusyo (J 950613 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1995, fhboxers,  stvbios, stvbios, stv, stv,  fhboxers,  ROT0,   "Sega", 	 				  "Funky Head Boxers (JUETBKAL 951218 V1.000)", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1994, gaxeduel,  stvbios, stvbios, stv, stv,  gaxeduel,	ROT0,   "Sega", 	     			  "Golden Axe - The Duel (JUETL 950117 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1998, grdforce,  stvbios, stvbios, stv, stv,  grdforce,  ROT0,   "Success",  				  "Guardian Force (JUET 980318 V0.105)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1996, groovef,   stvbios, stvbios, stv, stv,  groovef,   ROT0,   "Atlus",    				  "Power Instinct 3 - Groove On Fight (J 970416 V1.001)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1998, hanagumi,  stvbios, stvbios, stv, stv,  hanagumi,  ROT0,   "Sega",     				  "Hanagumi Taisen Columns - Sakura Wars (J 971007 V1.010)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1996, introdon,  stvbios, stvbios, stv, stv,  ic13,		ROT0,   "Sunsoft / Success", 		  "Karaoke Quiz Intro Don Don! (J 960213 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1995, kiwames,   stvbios, stvbios, stv, stvmp,ic13,      ROT0,   "Athena",   				  "Pro Mahjong Kiwame S (J 951020 V1.208)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_NOT_WORKING ) // not passing the test screen, error on cartridge
GAMEB( 1997, maruchan,  stvbios, stvbios, stv, stv,  maruchan,  ROT0,   "Sega / Toyosuisan", 	      "Maru-Chan de Goo! (J 971216 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1995, mausuke,   stvbios, stvbios, stv, stv,  mausuke,   ROT0,   "Data East",				  "Mausuke no Ojama the World (J 960314 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1998, othellos,  stvbios, stvbios, stv, stv,  othellos,  ROT0,   "Success",  				  "Othello Shiyouyo (J 980423 V1.002)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1995, pblbeach,  stvbios, stvbios, stv, stv,  pblbeach,  ROT0,   "T&E Soft",   				  "Pebble Beach - The Great Shot (JUE 950913 V0.990)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1996, prikura,   stvbios, stvbios, stv, stv,  prikura,   ROT0,   "Atlus",    				  "Princess Clara Daisakusen (J 960910 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1996, puyosun,   stvbios, stvbios, stv, stv,  puyosun,   ROT0,   "Compile",  				  "Puyo Puyo Sun (J 961115 V0.001)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1998, rsgun,     stvbios, stvbios, stv, stv,  rsgun,     ROT0,   "Treasure",   				  "Radiant Silvergun (JUET 980523 V1.000)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1996, sassisu,   stvbios, stvbios, stv, stv,  sassisu,   ROT0,   "Sega", 	     			  "Taisen Tanto-R Sashissu!! (J 980216 V1.000)", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1998, seabass,   stvbios, stvbios, stv, stv,  seabass,   ROT0,   "A wave inc. (Able license)","Sea Bass Fishing (JUET 971110 V0.001)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_NOT_WORKING ) // not passing the test screen, error on cartridge
GAMEB( 1995, shanhigw,  stvbios, stvbios, stv, stv,  shanhigw,	ROT0,   "Sunsoft / Activision", 	  "Shanghai - The Great Wall / Shanghai Triple Threat (JUE 950623 V1.005)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1997, shienryu,  stvbios, stvbios, stv, stv,  shienryu,  ROT270, "Warashi",  				  "Shienryu (JUET 961226 V1.000)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1995, smleague,  stvbios, stvbios, stv, stv,  smleague,  ROT0,   "Sega", 	     			  "Super Major League (U 960108 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_NOT_WORKING ) // not passing the test screen, error on cartridge
GAMEB( 1995, finlarch,  smleague,stvbios, stv, stv,  finlarch,  ROT0,   "Sega", 	     			  "Final Arch (J 950714 V1.001)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_NOT_WORKING ) // wrong graphics
GAMEB( 1998, sss,       stvbios, stvbios, stv, stv,  sss,       ROT0,   "Capcom / Cave / Victor",	  "Steep Slope Sliders (JUET 981110 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_NOT_WORKING ) // not passing the test screen, error on cartridge
GAMEB( 1995, sandor,    stvbios, stvbios, stv, stv,  sandor,    ROT0,   "Sega", 	     			  "Sando-R (J 951114 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1996, sokyugrt,  stvbios, stvbios, stv, stv,  sokyugrt,  ROT0,   "Raizing / 8ing",    		  "Soukyugurentai / Terra Diver (JUET 960821 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1995, suikoenb,  stvbios, stvbios, stv, stv,  suikoenb,  ROT0,   "Data East",  				  "Suikoenbu (J 950314 V2.001)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1995, thunt,     sandor,  stvbios, stv, stv,  thunt,     ROT0,   "Sega (Deniam license?)",	  "Treasure Hunt (JUET 970901 V2.00E)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1996, vfkids,    stvbios, stvbios, stv, stv,  ic13,      ROT0,   "Sega", 	 				  "Virtua Fighter Kids (JUET 960319 V0.000)", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1997, winterht,  stvbios, stvbios, stv, stv,  winterht,  ROT0,   "Sega", 	 				  "Winter Heat (JUET 971012 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )

/* Almost */
GAMEB( 1997, vmahjong,  stvbios, stvbios, stv, stvmp,stv,       ROT0,   "Micronet",   				  "Virtual Mahjong (J 961214 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_NOT_WORKING ) // not passing the test screen, error on cartridge
GAMEB( 1998, myfairld,  stvbios, stvbios, stv, stvmp,stv,       ROT0,   "Micronet",   				  "Virtual Mahjong 2 - My Fair Lady (J 980608 V1.000)", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_NOT_WORKING )
GAMEB( 1998, astrass,   stvbios, stvbios, stv, stv,  astrass,   ROT0,   "Sunsoft",    				  "Astra SuperStars (J 980514 V1.002)", GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING) // wrong background
GAMEB( 1998, twcup98,   stvbios, stvbios, stv, stv,  twcup98,   ROT0,   "Tecmo",      				  "Tecmo World Cup '98 (JUET 980410 V1.000)", GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS|GAME_NOT_WORKING ) // player movement
GAMEB( 1997, znpwfv,    stvbios, stvbios, stv, stv,  znpwfv,    ROT0,   "Sega", 	     			  "Zen Nippon Pro-Wrestling Featuring Virtua (J 971123 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )

GAMEB( 1995, critcrsh,  stvbios, stvbios, critcrsh, critcrsh, ic13, ROT0, "Sega", 	     			  "Critter Crusher (EA 951204 V1.000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_NOT_WORKING )

/* Doing Something.. but not enough yet */
GAMEB( 1998, elandore,  stvbios, stvbios, stv, stv,  elandore,  ROT0,   "Sai-Mate",   				  "Elan Doree - Legend of Dragon (JUET 980922 V1.006)", GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )//japanese name?
GAMEB( 1995, vfremix,   stvbios, stvbios, stv, stv,  vfremix,   ROT0,   "Sega", 	     			  "Virtua Fighter Remix (JUETBKAL 950428 V1.000)", GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
GAMEB( 1996, findlove,  stvbios, stvbios, stv, stv,  ic13,      ROT0,   "Daiki / FCF",    			  "Find Love (J 971212 V1.000)", GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
GAMEB( 1995, decathlt,  stvbios, stvbios, stv, stv,  ic13,  	 ROT0,   "Sega", 	     			  "Decathlete (JUET 960424 V1.000)", GAME_NO_SOUND | GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION )

/* not working,black screen */
GAMEB( 1999, ffreveng,  stvbios, stvbios, stv, stv,  stv,       ROT0,   "Capcom",     				  "Final Fight Revenge (JUET 990714 V1.000)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_NOT_WORKING )
/* CD games */
GAMEB( 1995, sfish2,    0,       stvbios, stv, stv,  sfish2,    ROT0,   "Sega",	     			  "Sport Fishing 2 (UET 951106 V1.10e)", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEB( 1995, sfish2j,   sfish2,  stvbios, stv, stv,  sfish2j,   ROT0,   "Sega",	     			  "Sport Fishing 2 (J 951201 V1.100)", GAME_NO_SOUND | GAME_NOT_WORKING )

/*
This is the known list of undumped ST-V games:
    Kiss Off (US version of mausuke)
    Outlaws of the Lost Dynasty (US version of suikoenb)
    Baku Baku (US version of Baku Baku Animal,dunno if it exists for the ST-V)
    Sport Fishing
    Aroma Club
    Movie Club
    Name Club
    NBA Action (?)
    Quiz Omaeni Pipon Cho! (?)

Others may exist as well...
*/

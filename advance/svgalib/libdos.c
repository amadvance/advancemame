/*

LIBDOS - SVGALIB port to MSDOS for the DJGPP compiler.

Copyright (C) 1999-2001 Andrea Mazzoleni

*/

#include "libvga.h"
#include "timing.h"
#include "driver.h"
#include "vga.h"
#include "vgaio.h"
#include "ramdac/ramdac.h"

#include "card.h"
#include "map.h"
#include "pci.h"

#include <unistd.h>
#include <pc.h>
#include <stdlib.h>
#include <crt0.h>
#include <sys/nearptr.h>

#ifndef __MSDOS__
#error This module is for MSDOS only
#endif

card_crtc libdos_crtc;
int libdos_tvpal;
int libdos_tvntsc;
card_mode libdos_mode;
int libdos_mode_number;
int libdos_divideclock; /* if set uses the VGA sequencer register to divide the dot clock by 2 */

/**************************************************************************/
/* os */

void* mmap(void* start, unsigned length, int prot, int flags, int fd, unsigned offset) {
	unsigned long linear;

	(void)prot;
	(void)fd;

	if ((flags & MAP_FIXED) != 0)
		return MAP_FAILED;

	if ((_crt0_startup_flags & _CRT0_FLAG_NEARPTR) == 0) {
		if (!__djgpp_nearptr_enable())
			return MAP_FAILED;
		_crt0_startup_flags |= _CRT0_FLAG_NEARPTR;
	}

	if (map_create_linear_mapping(&linear, offset, length)!=0)
		return MAP_FAILED;

	card_log("card: mmap %x -> %x, size %x\n", offset, linear, length);

	linear += __djgpp_conventional_base;

	return (void*)linear;
}

int munmap(void* start, unsigned length) {
	unsigned long offset;

	offset = (unsigned)start;

	offset -= __djgpp_conventional_base;

	card_log("card: munmap %x, size %x\n", offset, length);

	map_remove_linear_mapping(offset,length);

	return 0;
}

int iopl(int perm) {
	(void)perm;
	return 0;
}

/**************************************************************************/
/* vga_help.c */

void port_rep_outb(unsigned char* string, int length, int port)
{
	while (length) {
		outportb(port,*string);
		++string;
		--length;
	}
}

void port_out(int value, int port)
{
	outportb(port,value);
}

void port_outw(int value, int port)
{
	outportw(port,value);
}

void port_outl(int value, int port)
{
	outportl(port,value);
}

int port_in(int port)
{
	return inportb(port);
}

int port_inw(int port)
{
	return inportw(port);
}

int port_inl(int port)
{
	return inportl(port);
}

/**************************************************************************/
/* vgapci.c */

struct pci_find {
	unsigned vendor;
	unsigned cont;
	unsigned bus_device_func;
};

static int pci_scan_device_callback(unsigned bus_device_func, unsigned vendor, unsigned device, void* _arg) {
	DWORD dw;
	unsigned base_class;
	struct pci_find* arg = (struct pci_find*)_arg;
	(void)device;

	if (vendor != arg->vendor)
		return 0;

	if (pci_read_dword(bus_device_func,0x8,&dw)!=0)
		return 0;

	base_class = (dw >> 16) & 0xFFFF;
	if (base_class != 0x300 /* DISPLAY | VGA */)
		return 0;

	if (arg->cont) {
		--arg->cont;
		return 0;
	}

	arg->bus_device_func = bus_device_func;

	return 1;
}

int __svgalib_pci_find_vendor_vga(unsigned int vendor, unsigned long *conf, int cont)
{
	int r;
	int i;
	struct pci_find find;
	find.vendor = vendor;
	find.cont = cont;

	r = pci_scan_device(pci_scan_device_callback,&find);
	if (r!=1)
		return 1; /* not found */

	for(i=0;i<64;++i) {
		DWORD v;
		pci_read_dword(find.bus_device_func,i*4,&v);
		conf[i] = v;
	}

	return 0;
}

int __svgalib_pci_find_vendor_vga_pos(unsigned int vendor, unsigned long *conf, int cont)
{
	int r;
	int i;
	struct pci_find find;
	find.vendor = vendor;
	find.cont = cont;

	r = pci_scan_device(pci_scan_device_callback,&find);
	if (r!=1)
		return 0; /* not found */

	for(i=0;i<64;++i) {
		DWORD v;
		pci_read_dword(find.bus_device_func,i*4,&v);
		conf[i] = v;
	}

	return find.bus_device_func;
}

int __svgalib_pci_read_config_byte(int pos, int address)
{
	BYTE r;
	pci_read_byte(pos,address,&r);
	return r;
}

int __svgalib_pci_read_config_word(int pos, int address)
{
	WORD r;
	pci_read_word(pos,address,&r);
	return r;
}

int __svgalib_pci_read_config_dword(int pos, int address)
{
	DWORD r;
	pci_read_dword(pos,address,&r);
	return r;
}

int __svgalib_pci_read_aperture_len(int pos, int reg)
{
	DWORD r;
	unsigned address;
	address = 16+4*reg; /* this is the memory register number */
	if (pci_read_dword_aperture_len(pos,address,&r) != 0)
		return 0;
	else
		return r;
}

void __svgalib_pci_write_config_byte(int pos, int address, unsigned char data)
{
	pci_write_byte(pos,address,data);
}

void __svgalib_pci_write_config_word(int pos, int address, unsigned short data)
{
	pci_write_word(pos,address,data);
}

void __svgalib_pci_write_config_dword(int pos, int address, unsigned int data)
{
	pci_write_dword(pos,address,data);
}

#if 0
int memorytest(unsigned char *m, int max_mem) {
    unsigned char sav[256];
    int i, j;

    max_mem*=4;
    for(i=0;i<max_mem;i++) {
        sav[i]=*(m+256*1024*i);
    }
    for(i=max_mem-1;i>=0;i--) {
        *(m+256*1024*i)=i;
    }
    for(i=0;i<max_mem;i++) {
        if(*(m+256*1024*i)!=i) break;
    }
    for(j=0;j<i;j++) {
        *(m+256*1024*j)=sav[j];
    }
    return i*256;
}
#else
int memorytest(unsigned char* m, int max_mem) {
	int i; /* number of 256 kByte block */

	for(i=1;i<=max_mem*4;i*=2) {
		/* last byte of the memory */
		unsigned char* p = m + 256*1024*i - 1;
		unsigned char save = *p;

		/* check the memory functionality */
		*p = 0xff;
		if (*p != 0xff)
			break;

		*p = 0x00;
		if (*p != 0x00)
			break;

		*p = save;
	}

	return (i/2) * 256;
}
#endif

/***************************************************************************/
/* vga */

int  (*__svgalib_inmisc)(void);
void (*__svgalib_outmisc)(int);
int  (*__svgalib_incrtc)(int);
void (*__svgalib_outcrtc)(int,int);
int  (*__svgalib_inseq)(int);
void (*__svgalib_outseq)(int,int);
int  (*__svgalib_ingra)(int);
void (*__svgalib_outgra)(int,int);
int  (*__svgalib_inatt)(int);
void (*__svgalib_outatt)(int,int);
void (*__svgalib_attscreen)(int);
void (*__svgalib_inpal)(int,int*,int*,int*);
void (*__svgalib_outpal)(int,int,int,int);
int  (*__svgalib_inis1)(void);

void __svgalib_delay(void) {
	__asm__ __volatile__ (
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		:
		:
		: "cc", "%eax"
	);
}

int __svgalib_CRT_I = CRT_IC; /* fix */
int __svgalib_CRT_D = CRT_DC; /* fix */
int __svgalib_IS1_R = IS1_RC; /* fix */
int __svgalib_driver_report = 0; /* report driver used after chipset detection */

unsigned char* __svgalib_banked_pointer = (unsigned char*)0x80000000; /* UNUSED */
unsigned long int __svgalib_banked_mem_base, __svgalib_banked_mem_size; /* UNUSED */
unsigned char* __svgalib_linear_pointer = (unsigned char*)0x80000000; /* UNUSED */
unsigned long int __svgalib_linear_mem_base, __svgalib_linear_mem_size;
unsigned char* __svgalib_mmio_pointer;
unsigned long int __svgalib_mmio_base, __svgalib_mmio_size;
static int mmio_mapped=0;

int inrestore; /* UNUSED */

DriverSpecs* __svgalib_driverspecs;
struct info __svgalib_infotable[16];
int __svgalib_cur_mode; /* Current mode */
struct info __svgalib_cur_info; /* Current mode info */
unsigned char __svgalib_novga = 0; /* fix to 0 */
int __svgalib_chipset;
int __svgalib_mem_fd = 0; /* fix to 0 */
unsigned char __svgalib_secondary = 0; /* fix to 0 */
unsigned char __svgalib_emulatepage = 0; /* fix to 0 */
unsigned char __svgalib_ragedoubleclock = 0; /* fix to 0 */
unsigned char* __svgalib_graph_mem = (unsigned char*)0x80000000; /* fix, UNUSED */
int __svgalib_modeinfo_linearset = IS_LINEAR; /* fix */
int __svgalib_screenon = 1;

char *__svgalib_token(char **ptr) 
{
    char *p;
    p=*ptr;
    while(*p==' ')p++;
    
    if(*p != '\0' ) {
        char *t;
        t=p;
        while((*t != '\0') && (*t != ' '))t++;
        if(*t==' ') {
            *t='\0';
            t++;
        }
        *ptr=t;
        return p;
    } else {
        *ptr=NULL;
        return NULL; 
    }
}

int __svgalib_saveregs(unsigned char *regs)
{
    int i;

    if (__svgalib_chipset == EGA || __svgalib_novga) {
	/* Special case: Don't save standard VGA registers. */
	return chipset_saveregs(regs);
    }
    /* save VGA registers */
    for (i = 0; i < CRT_C; i++) {
        regs[CRT + i] = __svgalib_incrtc(i);
    }
    for (i = 0; i < ATT_C; i++) {
	regs[ATT + i] = __svgalib_inatt(i);
    }
    for (i = 0; i < GRA_C; i++) {
	regs[GRA + i] = __svgalib_ingra(i);
    }
    for (i = 0; i < SEQ_C; i++) {
	regs[SEQ + i] = __svgalib_inseq(i);
    }
    regs[MIS] = __svgalib_inmisc();

    i = chipset_saveregs(regs);	/* save chipset-specific registers */
    /* i : additional registers */
    if (!SCREENON) {		/* We turned off the screen */
        __svgalib_attscreen(0x20);
    }
    return CRT_C + ATT_C + GRA_C + SEQ_C + 1 + i;
}

int __svgalib_setregs(const unsigned char *regs)
{
    int i;

    if(__svgalib_novga) return 1;

    if (__svgalib_chipset == EGA) {
	/* Enable graphics register modification */
	port_out(0x00, GRA_E0);
	port_out(0x01, GRA_E1);
    }
    /* update misc output register */
    __svgalib_outmisc(regs[MIS]);

    /* synchronous reset on */
    __svgalib_outseq(0x00,0x01);

    /* write sequencer registers */
    if (libdos_divideclock)
		__svgalib_outseq(0x01,regs[SEQ + 1] | 0x20 | 0x8);
	else
		__svgalib_outseq(0x01,regs[SEQ + 1] | 0x20);
    for (i = 2; i < SEQ_C; i++) {
       __svgalib_outseq(i,regs[SEQ + i]);
    }

    /* synchronous reset off */
    __svgalib_outseq(0x00,0x03);

    if (__svgalib_chipset != EGA) {
	/* deprotect CRT registers 0-7 */
        __svgalib_outcrtc(0x11,__svgalib_incrtc(0x11)&0x7f);
    }
    /* write CRT registers */
    for (i = 0; i < CRT_C; i++) {
        __svgalib_outcrtc(i,regs[CRT + i]);
    }

    /* write graphics controller registers */
    for (i = 0; i < GRA_C; i++) {
        __svgalib_outgra(i,regs[GRA+i]);
    }

    /* write attribute controller registers */
    for (i = 0; i < ATT_C; i++) {
        __svgalib_outatt(i,regs[ATT+i]);
    }

    return 0;
}

void __svgalib_read_options(char **commands, char *(*func) (int ind, int mode, char **nptr)) {
	/* TODO */
	/* no configuration supported */
	(void)commands;
	(void)func;
}

int vga_lastmodenumber(void) {
	return TEXT;
}

int vga_screenoff(void)
{
    int tmp = 0;

    SCREENON = 0;

    if(__svgalib_novga) return 0; 

    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->screenoff) {
	tmp = __svgalib_driverspecs->emul->screenoff();
    } else {
	/* turn off screen for faster VGA memory acces */
	if (CHIPSET != EGA) {
            __svgalib_outseq(0x01,__svgalib_inseq(0x01) | 0x20);
	}
	/* Disable video output */
#ifdef DISABLE_VIDEO_OUTPUT
	__svgalib_attscreen(0);
#endif
    }

    return tmp;
}


int vga_screenon(void)
{
    int tmp = 0;

    SCREENON = 1;
    if(__svgalib_novga) return 0; 
    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->screenon) {
	tmp = __svgalib_driverspecs->emul->screenon();
    } else {
	/* turn screen back on */
	if (CHIPSET != EGA) {
            __svgalib_outseq(0x01,__svgalib_inseq(0x01) & 0xdf);
	}
/* #ifdef DISABLE_VIDEO_OUTPUT */
	/* enable video output */
	__svgalib_attscreen(0x20);
/* #endif */
    }

    return 0;
}

int vga_getxdim(void) {
	return __svgalib_cur_info.xdim;
}

int vga_getydim(void) {
	return __svgalib_cur_info.ydim;
}

int vga_getcolors(void) {
	return __svgalib_cur_info.colors;
}

int vga_getcurrentmode(void) {
	return __svgalib_cur_mode;
}

vga_modeinfo *vga_getmodeinfo(int mode)
{
	static vga_modeinfo modeinfo;
	memset(&modeinfo,0,sizeof(modeinfo));

	modeinfo.linewidth = infotable[mode].xbytes;
	modeinfo.width = infotable[mode].xdim;
	modeinfo.height = infotable[mode].ydim;
	modeinfo.bytesperpixel = infotable[mode].bytesperpixel;
	modeinfo.colors = infotable[mode].colors;

	if (mode == TEXT) {
		modeinfo.flags = HAVE_EXT_SET;
		return &modeinfo;
	}

	modeinfo.flags = 0;

	chipset_getmodeinfo(mode, &modeinfo);

	/* If all needed info is here, signal if linear support has been enabled */
	if ((modeinfo.flags & (CAPABLE_LINEAR | EXT_INFO_AVAILABLE)) ==
		(CAPABLE_LINEAR | EXT_INFO_AVAILABLE)) {
		modeinfo.flags |= __svgalib_modeinfo_linearset;
	}

	return &modeinfo;
}

void __svgalib_emul_setpage(int page) {
	/* used only for banked modes */
}

void map_mmio() {
    unsigned long offset;

    if(mmio_mapped) return;

#ifdef __alpha__
    offset = 0x300000000;
#else
    offset = 0;
#endif

    if(__svgalib_mmio_size) {
        mmio_mapped=1;
        MMIO_POINTER=mmap( 0, __svgalib_mmio_size, PROT_READ | PROT_WRITE,
            		   MAP_SHARED, __svgalib_mem_fd, __svgalib_mmio_base + offset);
    } else {
        MMIO_POINTER=NULL;
    }
}

void map_linear(unsigned long base, unsigned long size) {
    unsigned long offset;
    
#ifdef __alpha__
    offset = 0x300000000;
#else
    offset = 0;
#endif
    
    LINEAR_POINTER=mmap(0, size,
  	                PROT_READ | PROT_WRITE, MAP_SHARED,
			__svgalib_mem_fd,
                        base + offset);
}

void unmap_linear(unsigned long size) {
    munmap(LINEAR_POINTER, size);
}

/***************************************************************************/
/* io */

#if 0 /* Actually they are define */
uint8_t v_readb(unsigned long addr) {
	return *(volatile uint8_t*)(__svgalib_mmio_pointer+addr);
}

uint16_t v_readw(unsigned long addr) {
	return *(volatile uint16_t*)(__svgalib_mmio_pointer+addr);
}

uint32_t v_readl(unsigned long addr) {
	return *(volatile uint32_t*)(__svgalib_mmio_pointer+addr);
}

void v_writeb(uint8_t b, unsigned long addr) {
	*(volatile uint8_t*)(__svgalib_mmio_pointer+addr) = b;
}

void v_writew(uint16_t b, unsigned long addr) {
	*(volatile uint16_t*)(__svgalib_mmio_pointer+addr) = b;
}

void v_writel(uint32_t b, unsigned long addr) {
	*(volatile uint32_t*)(__svgalib_mmio_pointer+addr) = b;
}
#endif

/***************************************************************************/
/* vgamisc */

void vga_waitretrace(void)
{
	if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->waitretrace) {
		__svgalib_driverspecs->emul->waitretrace();
	} else {
		while ((__svgalib_inis1() & 8)==0)
			;
		while ((__svgalib_inis1() & 8)!=0)
			;
	}
}

/***************************************************************************/
/* vgadraw */

int vga_drawscansegment(unsigned char *colors, int x, int y, int length) {
	(void)colors;
	(void)x;
	(void)y;
	(void)length;
	/* used only for cursor */
	assert(0);
	return 0;
}

/***************************************************************************/
/* vgapal */

int vga_getpalvec(int start, int num, int *pal) {
	(void)start;
	(void)num;
	(void)pal;
	/* used only for cursor */
	assert(0);
	return num;
}

int vga_setpalette(int index, int red, int green, int blue) {

	/* correct wrong RGB values */
	red &= 0x3F;
	green &= 0x3F;
	blue &= 0x3F;

	if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->setpalette) {
		return __svgalib_driverspecs->emul->setpalette(index, red, green, blue);
	} else {
		__svgalib_outpal(index,red,green,blue);
	}
	return 0;
}

/***************************************************************************/
/* vgadrv */

int vgadrv_saveregs(unsigned char regs[]) {
	(void)regs;
	assert(0);
	return 0;
}

void vgadrv_setregs(const unsigned char regs[], int mode) {
	(void)regs;
	(void)mode;
	assert(0);
}

void vgadrv_unlock(void) {
	assert(0);
}

void vgadrv_lock(void) {
	assert(0);
}

int vgadrv_test(void) {
	assert(0);
	return 0;
}

int vgadrv_init(int force, int par1, int par2) {
	(void)force;
	(void)par1;
	(void)par2;
	assert(0);
	return 0;
}

void vgadrv_setpage(int page) {
	(void)page;
	assert(0);
}

void vgadrv_setrdpage(int page) {
	(void)page;
	assert(0);
}

void vgadrv_setwrpage(int page) {
	(void)page;
	assert(0);
}

int vgadrv_setmode(int mode, int prv_mode) {
	(void)mode;
	(void)prv_mode;
	assert(0);
	return 0;
}

int vgadrv_modeavailable(int mode) {
	(void)mode;
	assert(0);
	return 0;
}

void vgadrv_setdisplaystart(int address) {
	(void)address;
	assert(0);
}

void vgadrv_setlogicalwidth(int width) {
	(void)width;
	assert(0);
}

void vgadrv_getmodeinfo(int mode, vga_modeinfo * modeinfo) {
	(void)mode;
	(void)modeinfo;
	assert(0);
}

DriverSpecs __svgalib_vga_driverspecs = {
	vgadrv_saveregs,
	vgadrv_setregs,
	vgadrv_unlock,
	vgadrv_lock,
	vgadrv_test,
	vgadrv_init,
	vgadrv_setpage,
	vgadrv_setrdpage,
	vgadrv_setwrpage,
	vgadrv_setmode,
	vgadrv_modeavailable,
	vgadrv_setdisplaystart,
	vgadrv_setlogicalwidth,
	vgadrv_getmodeinfo,
	0, /* bitblt */
	0, /* imageblt */
	0, /* fillblt */
	0, /* hlinelistblt */
	0, /* bltwait */
	0, /* extset */
	0, /* accel */
	0, /* linear */
	NULL, /* accelspecs */
	NULL, /* emulation */
	0 /* cursor */
};

/***************************************************************************/
/* interface */

void __svgalib_clear_accelspecs(AccelSpecs * accelspecs)
{
    memset(accelspecs, 0, sizeof(AccelSpecs));
}

ModeInfo* __svgalib_createModeInfoStructureForSvgalibMode(int mode)
{
	ModeInfo* modeinfo;

	if (mode != libdos_mode_number)
		return 0;

	modeinfo = malloc(sizeof(ModeInfo));
	assert(modeinfo);
	memset(modeinfo,0,sizeof(ModeInfo));

	modeinfo->width = libdos_mode.width;
	modeinfo->height = libdos_mode.height;
	modeinfo->bytesPerPixel = libdos_mode.bytes_per_pixel;


	switch (libdos_mode.bits_per_pixel) {
		case 8 :
			modeinfo->colorBits = 8;
			modeinfo->bitsPerPixel = 8;
			break;
		case 15 :
			modeinfo->colorBits = 15;
			modeinfo->bitsPerPixel = 16;
			break;
		case 16 :
			modeinfo->colorBits = 16;
			modeinfo->bitsPerPixel = 16;
			break;
		case 24 :
			modeinfo->colorBits = 24;
			modeinfo->bitsPerPixel = 24;
			break;
		case 32 :
			modeinfo->colorBits = 24;
			modeinfo->bitsPerPixel = 32;
			break;
	}

	modeinfo->redWeight = libdos_mode.red_len;
	modeinfo->greenWeight = libdos_mode.green_len;
	modeinfo->blueWeight = libdos_mode.blue_len;
	modeinfo->redOffset = libdos_mode.red_pos;
	modeinfo->greenOffset = libdos_mode.green_pos;
	modeinfo->blueOffset = libdos_mode.blue_pos;
	modeinfo->redMask = 0; /* TODO */
	modeinfo->blueMask = 0; /* TODO */
	modeinfo->greenMask = 0; /* TODO */

	modeinfo->lineWidth = libdos_mode.bytes_per_scanline;
	modeinfo->realWidth = libdos_mode.width;
	modeinfo->realHeight = libdos_mode.height;
	modeinfo->flags = 0; /* TODO */

	return modeinfo;
}

/*
 * This function converts a number of significant color bits to a matching
 * DAC mode type as defined in the RAMDAC interface.
 */
int __svgalib_colorbits_to_colormode(int bpp, int colorbits)
{
	if (colorbits == 8)
		return CLUT8_6;
	if (colorbits == 15)
		return RGB16_555;
	if (colorbits == 16)
		return RGB16_565;
	if (colorbits == 24) {
		if (bpp == 24)
			return RGB24_888_B;
		else
			return RGB32_888_B;
	}
	return CLUT8_6;
}

/*
 * findclock is an auxilliary function that checks if a close enough
 * pixel clock is provided by the card. Returns clock number if
 * succesful (a special number if a programmable clock must be used), -1
 * otherwise.
 */

/*
 * Clock allowance in 1/1000ths. 10 (1%) corresponds to a 250 kHz
 * deviation at 25 MHz, 1 MHz at 100 MHz
 */
#define CLOCK_ALLOWANCE 10

#define PROGRAMMABLE_CLOCK_MAGIC_NUMBER 0x1234

static int findclock(int clock, CardSpecs * cardspecs)
{
    int i;
    /* Find a clock that is close enough. */
    for (i = 0; i < cardspecs->nClocks; i++) {
	int diff;
	diff = cardspecs->clocks[i] - clock;
	if (diff < 0)
	    diff = -diff;
	if (diff * 1000 / clock < CLOCK_ALLOWANCE)
	    return i;
    }
    /* Try programmable clocks if available. */
    if (cardspecs->flags & CLOCK_PROGRAMMABLE) {
	int diff;
	diff = cardspecs->matchProgrammableClock(clock) - clock;
	if (diff < 0)
	    diff = -diff;
	if (diff * 1000 / clock < CLOCK_ALLOWANCE)
	    return PROGRAMMABLE_CLOCK_MAGIC_NUMBER;
    }
    /* No close enough clock found. */
    return -1;
}

int __svgalib_getmodetiming(ModeTiming* modetiming, ModeInfo* modeinfo, CardSpecs* cardspecs)
{
	int maxclock, desiredclock;

	/* Get the maximum pixel clock for the depth of the requested mode. */
	if (modeinfo->bitsPerPixel == 4)
		maxclock = cardspecs->maxPixelClock4bpp;
	else if (modeinfo->bitsPerPixel == 8)
		maxclock = cardspecs->maxPixelClock8bpp;
	else if (modeinfo->bitsPerPixel == 16) {
		/* rgb 16 bit mode check */
		if ((cardspecs->flags & NO_RGB16_565)!=0 && modeinfo->greenWeight==6)
			return 1;
		maxclock = cardspecs->maxPixelClock16bpp;
	} else if (modeinfo->bitsPerPixel == 24)
		maxclock = cardspecs->maxPixelClock24bpp;
	else if (modeinfo->bitsPerPixel == 32)
		maxclock = cardspecs->maxPixelClock32bpp;
	else
		return 1;

	/* clock check */
	if (libdos_crtc.dotclockHz / 1000 > maxclock)
		return 1;

	/* interlace check */
	if ((cardspecs->flags & NO_INTERLACE)!=0 && libdos_crtc.interlace)
		return 1;

	/*
	 * Copy the selected timings into the result, which may
	 * be adjusted for the chipset.
	 */
	modetiming->flags = 0;
	if (libdos_crtc.doublescan)
		modetiming->flags |= DOUBLESCAN;
	if (libdos_crtc.interlace)
		modetiming->flags |= INTERLACED;
	if (libdos_crtc.hpolarity)
		modetiming->flags |= NHSYNC;
	else
		modetiming->flags |= PHSYNC;
	if (libdos_crtc.vpolarity)
		modetiming->flags |= NVSYNC;
	else
		modetiming->flags |= PVSYNC;
	if (libdos_tvpal)
		modetiming->flags |= TVMODE | TVPAL;
	if (libdos_tvntsc)
		modetiming->flags |= TVMODE | TVNTSC;

	modetiming->pixelClock = libdos_crtc.dotclockHz / 1000;

	/*
	 * We know a close enough clock is available; the following is the
	 * exact clock that fits the mode. This is probably different
	 * from the best matching clock that will be programmed.
	 */
	desiredclock = cardspecs->mapClock(modeinfo->bitsPerPixel,libdos_crtc.dotclockHz / 1000);

	/* Fill in the best-matching clock that will be programmed. */
	modetiming->selectedClockNo = findclock(desiredclock, cardspecs);
	if (modetiming->selectedClockNo < 0)
		return 1;
	if (modetiming->selectedClockNo == PROGRAMMABLE_CLOCK_MAGIC_NUMBER) {
		modetiming->programmedClock = cardspecs->matchProgrammableClock(desiredclock);
		if (!modetiming->programmedClock)
			return 1;
		modetiming->flags |= USEPROGRCLOCK;
	} else
		modetiming->programmedClock = cardspecs->clocks[modetiming->selectedClockNo];

	modetiming->HDisplay = libdos_crtc.HDisp;
	modetiming->HSyncStart = libdos_crtc.HSStart;
	modetiming->HSyncEnd = libdos_crtc.HSEnd;
	modetiming->HTotal = libdos_crtc.HTotal;
	if (cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel, modetiming->programmedClock, libdos_crtc.HTotal) != libdos_crtc.HTotal) {
		/* Horizontal CRTC timings are scaled in some way. */
		modetiming->CrtcHDisplay = cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel, modetiming->programmedClock, libdos_crtc.HDisp);
		modetiming->CrtcHSyncStart = cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel, modetiming->programmedClock, libdos_crtc.HSStart);
		modetiming->CrtcHSyncEnd = cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel, modetiming->programmedClock, libdos_crtc.HSEnd);
		modetiming->CrtcHTotal = cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel, modetiming->programmedClock, libdos_crtc.HTotal);
		modetiming->flags |= HADJUSTED;
	} else {
		modetiming->CrtcHDisplay = libdos_crtc.HDisp;
		modetiming->CrtcHSyncStart = libdos_crtc.HSStart;
		modetiming->CrtcHSyncEnd = libdos_crtc.HSEnd;
		modetiming->CrtcHTotal = libdos_crtc.HTotal;
	}
	modetiming->VDisplay = libdos_crtc.VDisp;
	modetiming->VSyncStart = libdos_crtc.VSStart;
	modetiming->VSyncEnd = libdos_crtc.VSEnd;
	modetiming->VTotal = libdos_crtc.VTotal;
	if (modetiming->flags & DOUBLESCAN){
		modetiming->VDisplay <<= 1;
		modetiming->VSyncStart <<= 1;
		modetiming->VSyncEnd <<= 1;
		modetiming->VTotal <<= 1;
	}
	modetiming->CrtcVDisplay = modetiming->VDisplay;
	modetiming->CrtcVSyncStart = modetiming->VSyncStart;
	modetiming->CrtcVSyncEnd = modetiming->VSyncEnd;
	modetiming->CrtcVTotal = modetiming->VTotal;
	if (((modetiming->flags & INTERLACED) && (cardspecs->flags & INTERLACE_DIVIDE_VERT))
		|| (modetiming->VTotal >= 1024 && (cardspecs->flags & GREATER_1024_DIVIDE_VERT))) {
		/*
		 * Card requires vertical CRTC timing to be halved for
		 * interlaced modes, or for all modes with vertical
		 * timing >= 1024.
		 */
		modetiming->CrtcVDisplay /= 2;
		modetiming->CrtcVSyncStart /= 2;
		modetiming->CrtcVSyncEnd /= 2;
		modetiming->CrtcVTotal /= 2;
		modetiming->flags |= VADJUSTED;
	}

	return 0;
}

/***************************************************************************/
/* libdos */

void libdos_init(int divide_clock_with_sequencer)
{
	libdos_divideclock = divide_clock_with_sequencer;

	__svgalib_chipset = UNDEFINED;
	__svgalib_driverspecs = &__svgalib_vga_driverspecs;
	__svgalib_inmisc=__svgalib_vga_inmisc;
	__svgalib_outmisc=__svgalib_vga_outmisc;
	__svgalib_incrtc=__svgalib_vga_incrtc;
	__svgalib_outcrtc=__svgalib_vga_outcrtc;
	__svgalib_inseq=__svgalib_vga_inseq;
	__svgalib_outseq=__svgalib_vga_outseq;
	__svgalib_ingra=__svgalib_vga_ingra;
	__svgalib_outgra=__svgalib_vga_outgra;
	__svgalib_inatt=__svgalib_vga_inatt;
	__svgalib_outatt=__svgalib_vga_outatt;
	__svgalib_attscreen=__svgalib_vga_attscreen;
	__svgalib_inpal=__svgalib_vga_inpal;
	__svgalib_outpal=__svgalib_vga_outpal;
	__svgalib_inis1=__svgalib_vga_inis1;
}

void libdos_mode_init(unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, int doublescan, int interlace, int hsync, int vsync, unsigned bits_per_pixel, int tvpal, int tvntsc) {

	/* mode */
	memset(&libdos_mode,0,sizeof(card_mode));
	libdos_mode.width = hde;
	libdos_mode.height = vde;
	libdos_mode.bits_per_pixel = bits_per_pixel;
	libdos_mode.bytes_per_pixel = (libdos_mode.bits_per_pixel + 7) / 8;
	libdos_mode.bytes_per_scanline = (libdos_mode.bytes_per_pixel * libdos_mode.width + 0x3) & ~0x3;

	switch (bits_per_pixel) {
		case 15 :
			libdos_mode.red_len = 5;
			libdos_mode.red_pos = 10;
			libdos_mode.green_len = 5;
			libdos_mode.green_pos = 5;
			libdos_mode.blue_len = 5;
			libdos_mode.blue_pos = 0;
			break;
		case 16 :
			libdos_mode.red_len = 5;
			libdos_mode.red_pos = 11;
			libdos_mode.green_len = 6;
			libdos_mode.green_pos = 5;
			libdos_mode.blue_len = 5;
			libdos_mode.blue_pos = 0;
			break;
		case 24 :
		case 32 :
			libdos_mode.red_len = 8;
			libdos_mode.red_pos = 16;
			libdos_mode.green_len = 8;
			libdos_mode.green_pos = 8;
			libdos_mode.blue_len = 8;
			libdos_mode.blue_pos = 0;
			break;
	}

	/* mode */
	libdos_tvpal = tvpal;
	libdos_tvntsc = tvntsc;

	/* crtc */
	libdos_crtc.HDisp = hde;
	libdos_crtc.HSStart = hrs;
	libdos_crtc.HSEnd = hre;
	libdos_crtc.HTotal = ht;

	libdos_crtc.VDisp = vde;
	libdos_crtc.VSStart = vrs;
	libdos_crtc.VSEnd = vre;
	libdos_crtc.VTotal = vt;

	/* blank (not used) */
	libdos_crtc.HBStart = 0;
	libdos_crtc.HBEnd = 0;
	libdos_crtc.VBStart = 0;
	libdos_crtc.VBEnd = 0;

	/* the SVGALIB interface already divide and double the vertical value for doublescan and interlace */

	libdos_crtc.hpolarity = hsync;
	libdos_crtc.vpolarity = vsync;
	libdos_crtc.doublescan = doublescan;
	libdos_crtc.interlace = interlace;
	libdos_crtc.interlaceratio = 50;

	libdos_crtc.dotclockHz = pixelclock;
	libdos_mode_number = 15;

	__svgalib_infotable[libdos_mode_number].xdim = libdos_mode.width;
	__svgalib_infotable[libdos_mode_number].ydim = libdos_mode.height;
	__svgalib_infotable[libdos_mode_number].colors = libdos_mode.bits_per_pixel;
	__svgalib_infotable[libdos_mode_number].xbytes = libdos_mode.bytes_per_scanline;
	__svgalib_infotable[libdos_mode_number].bytesperpixel = libdos_mode.bytes_per_pixel;

	__svgalib_cur_mode = libdos_mode_number;
	__svgalib_cur_info = __svgalib_infotable[__svgalib_cur_mode];
}

void libdos_mode_done(void) {
	__svgalib_inmisc=__svgalib_vga_inmisc;
	__svgalib_outmisc=__svgalib_vga_outmisc;
	__svgalib_incrtc=__svgalib_vga_incrtc;
	__svgalib_outcrtc=__svgalib_vga_outcrtc;
	__svgalib_inseq=__svgalib_vga_inseq;
	__svgalib_outseq=__svgalib_vga_outseq;
	__svgalib_ingra=__svgalib_vga_ingra;
	__svgalib_outgra=__svgalib_vga_outgra;
	__svgalib_inatt=__svgalib_vga_inatt;
	__svgalib_outatt=__svgalib_vga_outatt;
	__svgalib_attscreen=__svgalib_vga_attscreen;
	__svgalib_inpal=__svgalib_vga_inpal;
	__svgalib_outpal=__svgalib_vga_outpal;
	__svgalib_inis1=__svgalib_vga_inis1;

	__svgalib_cur_mode = 0;
	__svgalib_cur_info = __svgalib_infotable[__svgalib_cur_mode];
}

void libdos_done(void) {
}


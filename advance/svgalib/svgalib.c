#include "svgaint.h"

#include "libvga.h"
#include "timing.h"
#include "driver.h"
#include "vga.h"
#include "vgaio.h"
#include "ramdac/ramdac.h"

#include <assert.h>

struct adv_svgalib_state_struct adv_svgalib_state;

/**************************************************************************/
/* heap */

#define HEAP_SIZE 8192

static unsigned char heap[HEAP_SIZE];

struct heap_slot {
	struct heap_slot* prev;
	struct heap_slot* next;
	int used;
	unsigned size;
};

#define SLOT_SIZE sizeof(struct heap_slot)

struct heap_slot* heap_list;

static struct heap_slot* heap_slot_from(void* p) {
	unsigned char* raw = (unsigned char*)p - SLOT_SIZE;
	return (struct heap_slot*)raw;
}

static unsigned char* heap_slot_to(struct heap_slot* h) {
	unsigned char* raw = (unsigned char*)h + SLOT_SIZE;
	return raw;
}

static unsigned heap_slot_begin(struct heap_slot* h) {
	return (unsigned char*)h - (unsigned char*)heap;
}

static unsigned heap_slot_size(struct heap_slot* h) {
	return h->size + SLOT_SIZE;
}

static unsigned heap_slot_end(struct heap_slot* h) {
	return heap_slot_begin(h) + heap_slot_size(h);
}

static void heap_init(void) {
	heap_list = (struct heap_slot*)heap;
	heap_list->prev = heap_list;
	heap_list->next = heap_list;
	heap_list->used = 0;
	heap_list->size = HEAP_SIZE - SLOT_SIZE;
}

void* adv_svgalib_malloc(unsigned size) {
	struct heap_slot* h = heap_list;
	struct heap_slot* n;

	if (size % SLOT_SIZE)
		size = size + SLOT_SIZE - size % SLOT_SIZE;

	while (h->used || h->size <= size + SLOT_SIZE) {
		h = h->next;
		if (h == heap_list)
			return 0;
	}

	n = (struct heap_slot*)((unsigned char*)h + size + SLOT_SIZE);
	n->prev = h;
	n->next = h->next;
	n->next->prev = n;
	n->prev->next = n;
	n->used = 0;
	n->size = h->size - size - SLOT_SIZE;
	h->used = 1;
	h->size = size;

	return heap_slot_to(h);
}

void adv_svgalib_free(void* ptr) {
	struct heap_slot* h = heap_slot_from(ptr);

	if (h->prev->used == 0 && heap_slot_end(h->prev) == heap_slot_begin(h)
		&& h->next->used == 0 && heap_slot_end(h) == heap_slot_begin(h->next)
	) {
		/* cat to prev and next */
		h->prev->size += heap_slot_size(h) + heap_slot_size(h->next);
		h->next->next->prev = h->prev;
		h->prev->next = h->next->next;
	} else if (h->prev->used == 0 && heap_slot_end(h->prev) == heap_slot_begin(h)) {
		/* cat to prev */
		h->prev->size += heap_slot_size(h);
		h->prev->next = h->next;
		h->next->prev = h->prev;
	} else if (h->next->used == 0 && heap_slot_end(h) == heap_slot_begin(h->next)) {
		/* cat to next */
		h->used = 0;
		h->size += heap_slot_size(h->next);
		h->next->next->prev = h;
		h->next = h->next->next;
	} else {
		/* keep in list */
		h->used = 0;
	}
}

void* adv_svgalib_calloc(unsigned n, unsigned size) {
	void* r = adv_svgalib_malloc(n * size);
	if (r)
		memset(r, 0, n*size);
	return r;
}

/**************************************************************************/

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
    if (adv_svgalib_state.divide_clock)
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

	if (mode != adv_svgalib_state.mode_number)
		return 0;

	modeinfo = malloc(sizeof(ModeInfo));
	assert(modeinfo);
	memset(modeinfo,0,sizeof(ModeInfo));

	modeinfo->width = adv_svgalib_state.mode.width;
	modeinfo->height = adv_svgalib_state.mode.height;
	modeinfo->bytesPerPixel = adv_svgalib_state.mode.bytes_per_pixel;


	switch (adv_svgalib_state.mode.bits_per_pixel) {
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

	modeinfo->redWeight = adv_svgalib_state.mode.red_len;
	modeinfo->greenWeight = adv_svgalib_state.mode.green_len;
	modeinfo->blueWeight = adv_svgalib_state.mode.blue_len;
	modeinfo->redOffset = adv_svgalib_state.mode.red_pos;
	modeinfo->greenOffset = adv_svgalib_state.mode.green_pos;
	modeinfo->blueOffset = adv_svgalib_state.mode.blue_pos;
	modeinfo->redMask = 0; /* TODO */
	modeinfo->blueMask = 0; /* TODO */
	modeinfo->greenMask = 0; /* TODO */

	modeinfo->lineWidth = adv_svgalib_state.mode.bytes_per_scanline;
	modeinfo->realWidth = adv_svgalib_state.mode.width;
	modeinfo->realHeight = adv_svgalib_state.mode.height;
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
	if (adv_svgalib_state.crtc.pixelclock / 1000 > maxclock)
		return 1;

	/* interlace check */
	if ((cardspecs->flags & NO_INTERLACE)!=0 && adv_svgalib_state.crtc.interlace)
		return 1;

	/*
	 * Copy the selected timings into the result, which may
	 * be adjusted for the chipset.
	 */
	modetiming->flags = 0;
	if (adv_svgalib_state.crtc.doublescan)
		modetiming->flags |= DOUBLESCAN;
	if (adv_svgalib_state.crtc.interlace)
		modetiming->flags |= INTERLACED;
	if (adv_svgalib_state.crtc.nhsync)
		modetiming->flags |= NHSYNC;
	else
		modetiming->flags |= PHSYNC;
	if (adv_svgalib_state.crtc.nvsync)
		modetiming->flags |= NVSYNC;
	else
		modetiming->flags |= PVSYNC;
	if (adv_svgalib_state.mode_tvpal)
		modetiming->flags |= TVMODE | TVPAL;
	if (adv_svgalib_state.mode_tvntsc)
		modetiming->flags |= TVMODE | TVNTSC;

	modetiming->pixelClock = adv_svgalib_state.crtc.pixelclock / 1000;

	/*
	 * We know a close enough clock is available; the following is the
	 * exact clock that fits the mode. This is probably different
	 * from the best matching clock that will be programmed.
	 */
	desiredclock = cardspecs->mapClock(modeinfo->bitsPerPixel,adv_svgalib_state.crtc.pixelclock / 1000);

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

	modetiming->HDisplay = adv_svgalib_state.crtc.hde;
	modetiming->HSyncStart = adv_svgalib_state.crtc.hrs;
	modetiming->HSyncEnd = adv_svgalib_state.crtc.hre;
	modetiming->HTotal = adv_svgalib_state.crtc.ht;
	if (cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel, modetiming->programmedClock, adv_svgalib_state.crtc.ht) != adv_svgalib_state.crtc.ht) {
		/* Horizontal CRTC timings are scaled in some way. */
		modetiming->CrtcHDisplay = cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel, modetiming->programmedClock, adv_svgalib_state.crtc.hde);
		modetiming->CrtcHSyncStart = cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel, modetiming->programmedClock, adv_svgalib_state.crtc.hrs);
		modetiming->CrtcHSyncEnd = cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel, modetiming->programmedClock, adv_svgalib_state.crtc.hre);
		modetiming->CrtcHTotal = cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel, modetiming->programmedClock, adv_svgalib_state.crtc.ht);
		modetiming->flags |= HADJUSTED;
	} else {
		modetiming->CrtcHDisplay = adv_svgalib_state.crtc.hde;
		modetiming->CrtcHSyncStart = adv_svgalib_state.crtc.hrs;
		modetiming->CrtcHSyncEnd = adv_svgalib_state.crtc.hre;
		modetiming->CrtcHTotal = adv_svgalib_state.crtc.ht;
	}
	modetiming->VDisplay = adv_svgalib_state.crtc.vde;
	modetiming->VSyncStart = adv_svgalib_state.crtc.vrs;
	modetiming->VSyncEnd = adv_svgalib_state.crtc.vre;
	modetiming->VTotal = adv_svgalib_state.crtc.vt;
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
/* adv_svgalib */

/* Short names for the most common flags */
#define FLAGS_NONE 0
#define FLAGS_INTERLACE 1
#define FLAGS_TV 2

static struct adv_svgalib_chipset_struct cards[] = {
#ifdef INCLUDE_NV3_DRIVER
	{ &__svgalib_nv3_driverspecs, NV3, "nv3", FLAGS_INTERLACE | FLAGS_TV },
#endif
#ifdef INCLUDE_TRIDENT_DRIVER
	{ &__svgalib_trident_driverspecs, TRIDENT, "trident", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_RENDITION_DRIVER
	/* The driver doesn't check the INTERLACED flags */
	{ &__svgalib_rendition_driverspecs, RENDITION, "rendition", FLAGS_NONE },
#endif
#ifdef INCLUDE_G400_DRIVER
	{ &__svgalib_g400_driverspecs, G400, "g400", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_PM2_DRIVER
	/* The driver doesn't check the INTERLACED flags */
	{ &__svgalib_pm2_driverspecs, PM2, "pm2", FLAGS_NONE },
#endif
#ifdef INCLUDE_SAVAGE_DRIVER
	{ &__svgalib_savage_driverspecs, SAVAGE, "savage", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_MILLENNIUM_DRIVER
	{ &__svgalib_mil_driverspecs, MILLENNIUM, "millenium", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_R128_DRIVER
	{ &__svgalib_r128_driverspecs, R128, "r128", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_BANSHEE_DRIVER
	{ &__svgalib_banshee_driverspecs, BANSHEE, "banshee", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_SIS_DRIVER
	{ &__svgalib_sis_driverspecs, SIS, "sis", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_I740_DRIVER
	/* A comment in the driver report that interlaced modes don't work  */
	{ &__svgalib_i740_driverspecs, I740, "i740", FLAGS_NONE },
#endif
#ifdef INCLUDE_I810_DRIVER
	/* A comment in the driver report that interlaced modes don't work  */
	{ &__svgalib_i810_driverspecs, I810, "i810", FLAGS_NONE },
#endif
#ifdef INCLUDE_LAGUNA_DRIVER
	{ &__svgalib_laguna_driverspecs, LAGUNA, "laguna", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_RAGE_DRIVER
	{ &__svgalib_rage_driverspecs, RAGE, "rage", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_MX_DRIVER
	{ &__svgalib_mx_driverspecs, MX, "mx", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_NEO_DRIVER
	{ &__svgalib_neo_driverspecs, NEOMAGIC, "neomagic", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_CHIPS_DRIVER
	{ &__svgalib_chips_driverspecs, CHIPS, "chips", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_MACH64_DRIVER
	{ &__svgalib_mach64_driverspecs, MACH64, "mach64", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_MACH32_DRIVER
	{ &__svgalib_mach32_driverspecs, MACH32, "mach32", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_EGA_DRIVER
	{ &__svgalib_ega_driverspecs, EGA, "ega", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_ET6000_DRIVER
	/* This must be before ET4000 */
	{ &__svgalib_et6000_driverspecs, ET6000, "et6000", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_ET4000_DRIVER
	{ &__svgalib_et4000_driverspecs, ET4000, "et4000", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_TVGA_DRIVER
	{ &__svgalib_tvga8900_driverspecs, TVGA8900, "tvga8900", FLAGS_INTERLACE }
#endif
#ifdef INCLUDE_CIRRUS_DRIVER
	{ &__svgalib_cirrus_driverspecs, CIRRUS, "cirrus", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_OAK_DRIVER
	{ &__svgalib_oak_driverspecs, OAK, "oak", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_PARADISE_DRIVER
	{ &__svgalib_paradise_driverspecs, PARADISE, "paradise", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_S3_DRIVER
	{ &__svgalib_s3_driverspecs, S3, "s3", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_ET3000_DRIVER
	{ &__svgalib_et3000_driverspecs, ET3000, "et3000", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_ARK_DRIVER
	{ &__svgalib_ark_driverspecs, ARK, "ark", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_GVGA6400_DRIVER
	{ &__svgalib_gvga6400_driverspecs, GVGA6400, "gvga6400", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_ATI_DRIVER
	{ &__svgalib_ati_driverspecs, ATI, "ati", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_ALI_DRIVER
	{ &__svgalib_ali_driverspecs, ALI, "ali", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_APM_DRIVER
	/* The driver doesn't check the INTERLACED flags */
	/* On certain cards this may toggle the video signal on/off which is ugly. Hence we test this last. */
	{ &__svgalib_apm_driverspecs, APM, "apm", FLAGS_NONE },
#endif
	{ 0, 0, 0, 0 }
};

int adv_svgalib_init(int divide_clock_with_sequencer)
{
	if (adv_svgalib_open() != 0)
		return -1;

	heap_init();

	adv_svgalib_state.divide_clock = divide_clock_with_sequencer;

	__svgalib_chipset = UNDEFINED;
	__svgalib_driverspecs = &__svgalib_vga_driverspecs;
	__svgalib_inmisc = __svgalib_vga_inmisc;
	__svgalib_outmisc = __svgalib_vga_outmisc;
	__svgalib_incrtc = __svgalib_vga_incrtc;
	__svgalib_outcrtc = __svgalib_vga_outcrtc;
	__svgalib_inseq = __svgalib_vga_inseq;
	__svgalib_outseq = __svgalib_vga_outseq;
	__svgalib_ingra = __svgalib_vga_ingra;
	__svgalib_outgra = __svgalib_vga_outgra;
	__svgalib_inatt = __svgalib_vga_inatt;
	__svgalib_outatt = __svgalib_vga_outatt;
	__svgalib_attscreen = __svgalib_vga_attscreen;
	__svgalib_inpal = __svgalib_vga_inpal;
	__svgalib_outpal = __svgalib_vga_outpal;
	__svgalib_inis1 = __svgalib_vga_inis1;

	return 0;
}

void adv_svgalib_mode_init(unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, int doublescan, int interlace, int hsync, int vsync, unsigned bits_per_pixel, int tvpal, int tvntsc)
{
	/* mode */
	memset(&adv_svgalib_state.mode,0,sizeof(adv_svgalib_state.mode));
	adv_svgalib_state.mode.width = hde;
	adv_svgalib_state.mode.height = vde;
	adv_svgalib_state.mode.bits_per_pixel = bits_per_pixel;
	adv_svgalib_state.mode.bytes_per_pixel = (adv_svgalib_state.mode.bits_per_pixel + 7) / 8;
	adv_svgalib_state.mode.bytes_per_scanline = (adv_svgalib_state.mode.bytes_per_pixel * adv_svgalib_state.mode.width + 0x3) & ~0x3;

	switch (bits_per_pixel) {
		case 15 :
			adv_svgalib_state.mode.red_len = 5;
			adv_svgalib_state.mode.red_pos = 10;
			adv_svgalib_state.mode.green_len = 5;
			adv_svgalib_state.mode.green_pos = 5;
			adv_svgalib_state.mode.blue_len = 5;
			adv_svgalib_state.mode.blue_pos = 0;
			break;
		case 16 :
			adv_svgalib_state.mode.red_len = 5;
			adv_svgalib_state.mode.red_pos = 11;
			adv_svgalib_state.mode.green_len = 6;
			adv_svgalib_state.mode.green_pos = 5;
			adv_svgalib_state.mode.blue_len = 5;
			adv_svgalib_state.mode.blue_pos = 0;
			break;
		case 24 :
		case 32 :
			adv_svgalib_state.mode.red_len = 8;
			adv_svgalib_state.mode.red_pos = 16;
			adv_svgalib_state.mode.green_len = 8;
			adv_svgalib_state.mode.green_pos = 8;
			adv_svgalib_state.mode.blue_len = 8;
			adv_svgalib_state.mode.blue_pos = 0;
			break;
	}

	/* mode */
	adv_svgalib_state.mode_tvpal = tvpal;
	adv_svgalib_state.mode_tvntsc = tvntsc;

	/* crtc */
	adv_svgalib_state.crtc.hde = hde;
	adv_svgalib_state.crtc.hrs = hrs;
	adv_svgalib_state.crtc.hre = hre;
	adv_svgalib_state.crtc.ht = ht;

	adv_svgalib_state.crtc.vde = vde;
	adv_svgalib_state.crtc.vrs = vrs;
	adv_svgalib_state.crtc.vre = vre;
	adv_svgalib_state.crtc.vt = vt;

	/* the SVGALIB interface already divide and double the vertical value for doublescan and interlace */

	adv_svgalib_state.crtc.nhsync = hsync;
	adv_svgalib_state.crtc.nvsync = vsync;
	adv_svgalib_state.crtc.doublescan = doublescan;
	adv_svgalib_state.crtc.interlace = interlace;

	adv_svgalib_state.crtc.pixelclock = pixelclock;
	adv_svgalib_state.mode_number = 15;

	__svgalib_infotable[adv_svgalib_state.mode_number].xdim = adv_svgalib_state.mode.width;
	__svgalib_infotable[adv_svgalib_state.mode_number].ydim = adv_svgalib_state.mode.height;
	__svgalib_infotable[adv_svgalib_state.mode_number].colors = adv_svgalib_state.mode.bits_per_pixel;
	__svgalib_infotable[adv_svgalib_state.mode_number].xbytes = adv_svgalib_state.mode.bytes_per_scanline;
	__svgalib_infotable[adv_svgalib_state.mode_number].bytesperpixel = adv_svgalib_state.mode.bytes_per_pixel;

	__svgalib_cur_mode = adv_svgalib_state.mode_number;
	__svgalib_cur_info = __svgalib_infotable[__svgalib_cur_mode];
}

void adv_svgalib_mode_done(void)
{
	__svgalib_cur_mode = 0;
	__svgalib_cur_info = __svgalib_infotable[__svgalib_cur_mode];
}

void adv_svgalib_done(void)
{
	adv_svgalib_close();
}

int adv_svgalib_detect(const char* name) {
	unsigned bit_map[5] = { 8,15,16,24,32 };
	unsigned i;

	adv_svgalib_state.driver = 0;
	for(i=0;cards[i].name;++i) {
		if (strcmp(name,"auto")==0 || strcmp(name,cards[i].name)==0) {
			if (cards[i].drv->test()) {
				adv_svgalib_state.driver = &cards[i];
				__svgalib_chipset = adv_svgalib_state.driver->chipset;
				break;
			}
		}
	}

	if (adv_svgalib_state.driver == 0) {
		return -1;
	}

	if (__svgalib_linear_mem_size == 0) {
		return -1;
	}

	if (__svgalib_linear_mem_base == 0) {
		return -1;
	}

	if (!adv_svgalib_state.driver->drv->saveregs
		|| !adv_svgalib_state.driver->drv->setregs
		|| !adv_svgalib_state.driver->drv->test
		|| !adv_svgalib_state.driver->drv->init
		|| !adv_svgalib_state.driver->drv->setmode
		|| !adv_svgalib_state.driver->drv->modeavailable
		|| !adv_svgalib_state.driver->drv->linear) {
		return -1;
	}

	adv_svgalib_state.has_bit8 = 1;
	adv_svgalib_state.has_bit15 = 1;
	adv_svgalib_state.has_bit16 = 1;
	adv_svgalib_state.has_bit24 = 1;
	adv_svgalib_state.has_bit32 = 1;

	/* bit depth */
	for(i=0;i<5;++i) {
		unsigned bit = bit_map[i];

		adv_svgalib_mode_init(25200000/2,640/2,656/2,752/2,800/2,480,490,492,525,0,0,1,1,bit,0,0);

		if (adv_svgalib_state.driver->drv->modeavailable(adv_svgalib_state.mode_number) == 0) {
			switch (bit) {
				case 8 : adv_svgalib_state.has_bit8 = 0; break;
				case 15 : adv_svgalib_state.has_bit15 = 0; break;
				case 16 : adv_svgalib_state.has_bit16 = 0; break;
				case 24 : adv_svgalib_state.has_bit24 = 0; break;
				case 32 : adv_svgalib_state.has_bit32 = 0; break;
			}
		}

		adv_svgalib_mode_done();
	}

	if (adv_svgalib_state.has_bit8 == 0 && adv_svgalib_state.has_bit15 == 0 && adv_svgalib_state.has_bit16 == 0 && adv_svgalib_state.has_bit24 == 0 && adv_svgalib_state.has_bit32 == 0) {
		return -1;
	}

	/* interlace */
	adv_svgalib_state.has_interlace = 0;
	if ((adv_svgalib_state.driver->cap & FLAGS_INTERLACE) != 0) {
		adv_svgalib_state.has_interlace = 1;
		adv_svgalib_mode_init(40280300,1024,1048,1200,1280,768,784,787,840,0,1,1,1,8,0,0);

		if (adv_svgalib_state.driver->drv->modeavailable(adv_svgalib_state.mode_number) == 0) {
			adv_svgalib_state.has_interlace = 0;
		}

		adv_svgalib_mode_done();
	}

	if ((adv_svgalib_state.driver->cap & FLAGS_TV) != 0) {
		adv_svgalib_state.has_tvntsc = 1;
		adv_svgalib_state.has_tvpal = 1;
	} else {
		adv_svgalib_state.has_tvntsc = 0;
		adv_svgalib_state.has_tvpal = 0;
	}

	return 0;
}

int adv_svgalib_set(unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, int doublescan, int interlace, int hsync, int vsync, unsigned bits_per_pixel, int tvpal, int tvntsc)
{
	adv_svgalib_mode_init(pixelclock, hde, hrs, hre, ht, vde, vrs, vre, vt, doublescan, interlace, hsync, vsync, bits_per_pixel, tvpal, tvntsc);

	if (adv_svgalib_state.driver->drv->unlock)
		adv_svgalib_state.driver->drv->unlock();

#ifdef NDEBUG
	adv_svgalib_disable();
	vga_screenoff();
#endif

	if (adv_svgalib_state.driver->drv->setmode(adv_svgalib_state.mode_number, TEXT)) {
		adv_svgalib_enable();
		vga_screenon();
		adv_svgalib_mode_done();
		return -1;
	}
	adv_svgalib_enable();

	adv_svgalib_usleep(10000);

	vga_screenon();

	if (adv_svgalib_state.driver->drv->linear(LINEAR_ENABLE, __svgalib_linear_mem_base)!=0) {
		adv_svgalib_mode_done();
		return -1;
	}

	return 0;
}

void adv_svgalib_unset(void) {
	if (adv_svgalib_state.driver->drv->unlock)
		adv_svgalib_state.driver->drv->unlock();

	adv_svgalib_state.driver->drv->linear(LINEAR_DISABLE, __svgalib_linear_mem_base);

	adv_svgalib_mode_done();
}

void adv_svgalib_save(unsigned char* regs) {
	if (adv_svgalib_state.driver->drv->unlock)
		adv_svgalib_state.driver->drv->unlock();

	adv_svgalib_disable();
	__svgalib_saveregs(regs);
	adv_svgalib_enable();
}

void adv_svgalib_restore(unsigned char* regs) {
	if (adv_svgalib_state.driver->drv->unlock)
		adv_svgalib_state.driver->drv->unlock();

#ifdef NDEBUG
	adv_svgalib_disable();
	vga_screenoff();
#endif
	__svgalib_setregs(regs);
	adv_svgalib_state.driver->drv->setregs(regs, TEXT);
	adv_svgalib_enable();

	adv_svgalib_usleep(10000);

	vga_screenon();
}

void adv_svgalib_mmio_map(void) {
	if (__svgalib_mmio_size) {
		__svgalib_mmio_pointer = adv_svgalib_mmap(0, __svgalib_mmio_size, PROT_READ | PROT_WRITE, MAP_SHARED, __svgalib_mem_fd, __svgalib_mmio_base);
	} else {
		__svgalib_mmio_pointer = 0;
	}
}

void adv_svgalib_mmio_unmap(void) {
	if (__svgalib_mmio_size) {
		adv_svgalib_munmap(__svgalib_mmio_pointer, __svgalib_mmio_size);
		__svgalib_mmio_pointer = 0;
	}
}

void adv_svgalib_linear_map(void) {
	if (__svgalib_linear_mem_size) {
		__svgalib_linear_pointer = adv_svgalib_mmap(0, __svgalib_linear_mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, __svgalib_mem_fd, __svgalib_linear_mem_base);
	} else {
		__svgalib_linear_pointer = 0;
	}
}

void adv_svgalib_linear_unmap(void) {
	if (__svgalib_linear_mem_size) {
		adv_svgalib_munmap(__svgalib_linear_pointer, __svgalib_linear_mem_size);
	}
}

void adv_svgalib_scroll_set(unsigned offset) {
	if (adv_svgalib_state.driver->drv->setdisplaystart)
		adv_svgalib_state.driver->drv->setdisplaystart(offset);
}

void adv_svgalib_scanline_set(unsigned byte_length) {
	adv_svgalib_state.mode.bytes_per_scanline = byte_length;

	if (adv_svgalib_state.driver->drv->setlogicalwidth)
		adv_svgalib_state.driver->drv->setlogicalwidth(byte_length);
}

void adv_svgalib_palette_set(unsigned index, unsigned r, unsigned g, unsigned b) {
	vga_setpalette(index, r, g, b);
}

void adv_svgalib_wait_vsync(void) {
	vga_waitretrace();
}

void adv_svgalib_on(void) {
	vga_screenon();
}

void adv_svgalib_off(void) {
	vga_screenoff();
}

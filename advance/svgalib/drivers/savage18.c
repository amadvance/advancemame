/*
Savage chipset driver 

Written by Matan Ziv-Av (matan@svgalib.org)

Based on XFree 3.3.6 driver by S. Marineau and Tim Roberts.
And XFree 4.1.0 driver by Kevin Brosius.

*/

#include <stdlib.h>
#include <stdio.h>		
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "vga.h"
#include "libvga.h"
#include "svgadriv.h"
#include "timing.h"
#include "vgaregs.h"
#include "interfac.h"
#include "vgapci.h"
#include "endianes.h"
#include "vgammvga.h"

#define SAVAGEREG_SAVE(i) (VGA_TOTAL_REGS+i)
#define TOTAL_REGS (VGA_TOTAL_REGS + 64)

typedef struct {     

   unsigned char SR08, SR0A, SR0E, SR0F, SR10, SR11, SR12, SR13;
   unsigned char SR15, SR18, SR1B, SR29, SR30;
   unsigned char SR54, SR55, SR56, SR57;
   unsigned char Clock;
//   unsigned char s3DacRegs[0x101];
   unsigned char CR31, CR33, CR34, CR36, CR3A, CR3B, CR3C;
   unsigned char CR40, CR41, CR42, CR43, CR45;
   unsigned char CR50, CR51, CR53, CR54, CR55, CR58, CR5B, CR5D, CR5E;
   unsigned char CR63, CR65, CR66, CR67, CR68, CR69, CR6D, CR6F; /* Video attrib. */
   unsigned char CR7B, CR7D;
   unsigned char CR85, CR86, CR87, CR88;
   unsigned char CR90, CR91, CR92, CR93, CRB0;
} vgaS3VRec, *vgaS3VPtr;

static int init(int, int, int);
static void unlock(void);
static void lock(void);

enum { UNKNOWN, TRIO64, TRIO3D, TRIO3D2X, 
                VIRGE, VIRGEVX, VIRGEDX, VIRGEGX2, VIRGEMX,
                SAVAGE3D, SAVAGEMX, SAVAGE4, SAVAGEPRO, SAVAGE2000 };

static int memory, chipset;
static int is_linear, linear_base;

static CardSpecs *cardspecs;

static void setpage(int page)
{
    __svgalib_outcrtc(0x6a, page);
}

static int inlinearmode(void)
{
return is_linear;
}

/* Fill in chipset specific mode information */

static void getmodeinfo(int mode, vga_modeinfo *modeinfo)
{

    if(modeinfo->colors==16)return;

    modeinfo->maxpixels = memory*1024/modeinfo->bytesperpixel;
    modeinfo->maxlogicalwidth = 4088;
    modeinfo->startaddressrange = memory * 1024 - 1;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;

    if (modeinfo->bytesperpixel >= 1) {
	if(linear_base)modeinfo->flags |= CAPABLE_LINEAR;
        if (inlinearmode())
	    modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
    }
}

/* Read and save chipset-specific registers */

static int saveregs(unsigned char regs[])
{ 
    unsigned char cr3a, cr66;
    vgaS3VPtr save = (vgaS3VPtr)(regs+VGA_TOTAL_REGS);

    unlock();		

    cr66 = __svgalib_incrtc(0x66);
    __svgalib_outcrtc(0x66, cr66 | 0x80);
    cr3a = __svgalib_incrtc(0x3a);
    __svgalib_outcrtc(0x3a, cr3a | 0x80);

    cr66 = __svgalib_incrtc(0x66);
    __svgalib_outcrtc(0x66, cr66 | 0x80);
    cr3a = __svgalib_incrtc(0x3a);
    __svgalib_outcrtc(0x3a, cr3a | 0x80);

#if 0
   save = (vgaS3VPtr)vgaHWSave((vgaHWPtr)save, sizeof(vgaS3VRec));
#endif

    __svgalib_outcrtc(0x66, cr66);
    __svgalib_outcrtc(0x3a, cr3a);
    __svgalib_outcrtc(0x66, cr66);
    __svgalib_outcrtc(0x3a, cr3a);

   /* First unlock extended sequencer regs */
    save->SR08 = __svgalib_inseq(0x08);
    __svgalib_outseq(0x08, 0x06);

   /* Now we save all the s3 extended regs we need */
    save->CR31 = __svgalib_incrtc(0x31);
    save->CR34 = __svgalib_incrtc(0x34);
    save->CR36 = __svgalib_incrtc(0x36);
    save->CR3A = __svgalib_incrtc(0x3a);

    if(chipset>TRIO3D) 
        save->CR40 = __svgalib_incrtc(0x40);

    if(chipset==VIRGEMX) 
        save->CR41 = __svgalib_incrtc(0x41);

    save->CR42 = __svgalib_incrtc(0x42);
    save->CR45 = __svgalib_incrtc(0x45);

    if(chipset>=SAVAGE3D) 
        save->CR50 = __svgalib_incrtc(0x50);

    save->CR51 = __svgalib_incrtc(0x51);
    save->CR53 = __svgalib_incrtc(0x53);

    if(chipset<SAVAGE3D){
        save->CR54 = __svgalib_incrtc(0x54);
        save->CR55 = __svgalib_incrtc(0x55);
    }

    save->CR58 = __svgalib_incrtc(0x58);

    if(chipset<SAVAGE3D)
        save->CR63 = __svgalib_incrtc(0x63);

    save->CR66 = __svgalib_incrtc(0x66);
    save->CR67 = __svgalib_incrtc(0x67);
    save->CR68 = __svgalib_incrtc(0x68);
    save->CR69 = __svgalib_incrtc(0x69);

    if(chipset>=SAVAGE3D) 
        save->CR6F = __svgalib_incrtc(0x6f);

    save->CR33 = __svgalib_incrtc(0x33);

    if((chipset==TRIO3D2X)||(chipset==VIRGEGX2)||(chipset==VIRGEMX))
        save->CR85 = __svgalib_incrtc(0x85);

    if((chipset==VIRGEDX)||(chipset>=SAVAGE3D))
        save->CR86 = __svgalib_incrtc(0x86);
    
    if((chipset==TRIO3D2X)||(chipset==VIRGEGX2)||(chipset==VIRGEMX)) {
        save->CR7B = __svgalib_incrtc(0x7b);
        save->CR7D = __svgalib_incrtc(0x7d);
        save->CR87 = __svgalib_incrtc(0x87);
        save->CR92 = __svgalib_incrtc(0x92);
        save->CR93 = __svgalib_incrtc(0x93);
    }

    if((chipset==TRIO3D2X)||(chipset==VIRGEDX)||(chipset==VIRGEGX2)||
       (chipset==VIRGEMX)) {
        save->CR90 = __svgalib_incrtc(0x90);
        save->CR91 = __svgalib_incrtc(0x91);
    }
    
    if(chipset>=SAVAGE3D) {
        save->CR88 = __svgalib_incrtc(0x88);
        save->CR90 = __svgalib_incrtc(0x90);
        save->CR91 = __svgalib_incrtc(0x91);
        save->CRB0 = __svgalib_incrtc(0xb0) | 0x80;
    }
    
    save->CR3B = __svgalib_incrtc(0x3b);
    save->CR3C = __svgalib_incrtc(0x3c);
    save->CR43 = __svgalib_incrtc(0x43);
    save->CR5D = __svgalib_incrtc(0x5d);
    save->CR5E = __svgalib_incrtc(0x5e);
    save->CR65 = __svgalib_incrtc(0x65);

    if(chipset<SAVAGE3D) 
        save->CR6D = __svgalib_incrtc(0x6d);

   /* Save sequencer extended regs for DCLK PLL programming */
    save->SR0E = __svgalib_inseq(0x0E);
    save->SR10 = __svgalib_inseq(0x10);
    save->SR11 = __svgalib_inseq(0x11);
    save->SR12 = __svgalib_inseq(0x12);
    save->SR13 = __svgalib_inseq(0x13);

    if(chipset>=SAVAGE3D) 
        save->SR29 = __svgalib_inseq(0x29);

    if((chipset==TRIO3D2X)||(chipset==VIRGEGX2)||(chipset==VIRGEMX)) {
        save->SR29 = __svgalib_inseq(0x29);
        save->SR54 = __svgalib_inseq(0x54);
        save->SR55 = __svgalib_inseq(0x55);
        save->SR56 = __svgalib_inseq(0x56);
        save->SR57 = __svgalib_inseq(0x57);
    }
        
    save->SR15 = __svgalib_inseq(0x15);

	if(chipset>=SAVAGE3D) 
    	save->SR30 = __svgalib_inseq(0x30);

	save->SR18 = __svgalib_inseq(0x18);

	if(chipset>=SAVAGE3D) 
    	save->SR1B = __svgalib_inseq(0x1B);

    if(chipset<=TRIO3D) {
        save->SR0A = __svgalib_inseq(0x0a);
        save->SR0F = __svgalib_inseq(0x0f);
    }

    __svgalib_outcrtc(0x3a, cr3a);
    __svgalib_outcrtc(0x66, cr66);

    return TOTAL_REGS - VGA_TOTAL_REGS;
}

/* Set chipset-specific registers */

static void setregs(const unsigned char regs[], int mode)
{  
    int tmp;
    vgaS3VPtr restore = (vgaS3VPtr)(regs+VGA_TOTAL_REGS);
    
    unlock();

#if 0
   /* Are we going to reenable STREAMS in this new mode? */
   s3vPriv.STREAMSRunning = restore->CR67 & 0x0c; 

   /* First reset GE to make sure nothing is going on */
   outb(vgaCRIndex, 0x66);
   if(port_in(vgaCRReg) & 0x01) S3SAVGEReset(0,__LINE__,__FILE__);
#endif
   
   /* As per databook, always disable STREAMS before changing modes */
    __svgalib_outcrtc(0x67, __svgalib_incrtc(0x67)&0xf3);

    if(chipset<SAVAGE3D)
        __svgalib_outcrtc(0x63, restore->CR63);

    __svgalib_outcrtc(0x66, restore->CR66);
    __svgalib_outcrtc(0x3a, restore->CR3A);
    __svgalib_outcrtc(0x31, restore->CR31);
    __svgalib_outcrtc(0x58, restore->CR58);
    
    if(chipset<SAVAGE3D)
        __svgalib_outcrtc(0x55, restore->CR55);
    
#if 0 /* why is it done twice? */
    __svgalib_outseq(0x08, 0x06);
    __svgalib_outseq(0x12, restore->SR12);    
    __svgalib_outseq(0x13, restore->SR13);    
    __svgalib_outseq(0x29, restore->SR29);    
    __svgalib_outseq(0x15, restore->SR15);
#endif

    __svgalib_outcrtc(0x53, restore->CR53);
    __svgalib_outcrtc(0x5d, restore->CR5D);
    __svgalib_outcrtc(0x5e, restore->CR5E);
    __svgalib_outcrtc(0x3b, restore->CR3B);
    __svgalib_outcrtc(0x3c, restore->CR3C);
    __svgalib_outcrtc(0x43, restore->CR43);
    __svgalib_outcrtc(0x65, restore->CR65);

    if(chipset<SAVAGE3D) 
        __svgalib_outcrtc(0x6d, restore->CR6D);

   /* Restore the desired video mode with CR67 */

    if(chipset<SAVAGE3D) 
        __svgalib_outcrtc(0x67, 0x50 | (__svgalib_incrtc(0x67)&0x0f));
    else
        __svgalib_outcrtc(0x67, 0x50 | (__svgalib_incrtc(0x67)&0xf3));
        
    usleep(10000);
    __svgalib_outcrtc(0x67, restore->CR67&0xf3);
        
    __svgalib_outcrtc(0x34, restore->CR34);

    if(chipset>TRIO3D) 
        __svgalib_outcrtc(0x40, restore->CR40);

    if(chipset==VIRGEMX) 
        __svgalib_outcrtc(0x41, restore->CR41);

    __svgalib_outcrtc(0x42, restore->CR42);
    __svgalib_outcrtc(0x45, restore->CR45);

    if(chipset>=SAVAGE3D) 
        __svgalib_outcrtc(0x50, restore->CR50);

    __svgalib_outcrtc(0x51, restore->CR51);

    if(chipset<SAVAGE3D) 
        __svgalib_outcrtc(0x54, restore->CR54);

    __svgalib_outcrtc(0x36, restore->CR36);
    __svgalib_outcrtc(0x68, restore->CR68);
    __svgalib_outcrtc(0x69, restore->CR69);

    if(chipset>=SAVAGE3D) 
        __svgalib_outcrtc(0x6f, restore->CR6F);

    __svgalib_outcrtc(0x33, restore->CR33);

    if(chipset>=SAVAGE3D) {
        __svgalib_outcrtc(0x86, restore->CR86);
        __svgalib_outcrtc(0x88, restore->CR88);
        __svgalib_outcrtc(0x90, restore->CR90);
        __svgalib_outcrtc(0x91, restore->CR91);
        __svgalib_outcrtc(0xb0, restore->CRB0);
    }

    if((chipset==TRIO3D2X)||(chipset==VIRGEGX2)||(chipset==VIRGEMX))
        __svgalib_outcrtc(0x85, restore->CR85);

    if(chipset==VIRGEDX) 
        __svgalib_outcrtc(0x86, restore->CR86);

    if(chipset==VIRGEGX2) {
        __svgalib_outcrtc(0x7b, restore->CR7B);
        __svgalib_outcrtc(0x7d, restore->CR7D);
        __svgalib_outcrtc(0x87, restore->CR87);
        __svgalib_outcrtc(0x92, restore->CR92);
        __svgalib_outcrtc(0x93, restore->CR93);
    }

    if((chipset==TRIO3D2X)||(chipset==VIRGEDX)||(chipset==VIRGEGX2)||(chipset==VIRGEMX)) {
        __svgalib_outcrtc(0x90, restore->CR90);
        __svgalib_outcrtc(0x91, restore->CR91);
    }

    __svgalib_outseq(0x08, 0x06);
   /* Restore extended sequencer regs for MCLK. SR10 == 255 indicates that 
    * we should leave the default SR10 and SR11 values there.
    */

    if (restore->SR10 != 255) {   
        __svgalib_outseq(0x10, restore->SR10);    
        __svgalib_outseq(0x11, restore->SR11);    
    }

   /* Restore extended sequencer regs for DCLK */

    __svgalib_outseq(0x12, restore->SR12);    
    __svgalib_outseq(0x13, restore->SR13);    

    if(chipset>=SAVAGE3D)
        __svgalib_outseq(0x29, restore->SR29);    

    if((chipset==TRIO3D2X)||(chipset==VIRGEGX2)||(chipset==VIRGEMX)) {
        __svgalib_outseq(0x29, restore->SR29);    
        __svgalib_outseq(0x54, restore->SR54);    
        __svgalib_outseq(0x55, restore->SR55);    
        __svgalib_outseq(0x56, restore->SR56);    
        __svgalib_outseq(0x57, restore->SR57);    
    }

    __svgalib_outseq(0x18, restore->SR18);

    if(chipset>=SAVAGE3D)
        __svgalib_outseq(0x1B, restore->SR1B);

    tmp = __svgalib_inseq(0x15) & ~0x21;
    __svgalib_outseq(0x15, tmp | 0x03);
    __svgalib_outseq(0x15, tmp | 0x23);
    __svgalib_outseq(0x15, tmp | 0x03);
    __svgalib_outseq(0x15, restore->SR15);
    
	usleep(100);
	
    if(chipset<=TRIO3D) {
        __svgalib_outseq(0x0a, restore->SR0A);
        __svgalib_outseq(0x0f, restore->SR0F);
    } else if (chipset >= SAVAGE3D) {
		__svgalib_outseq(0x30, restore->SR30);
	}

    __svgalib_outseq(0x08, restore->SR08);


   /* Now write out CR67 in full, possibly starting STREAMS */
   
    __svgalib_outcrtc(0x67, 0x50);
    usleep(10000);
    __svgalib_outcrtc(0x67, restore->CR67&0xf3);

    __svgalib_outcrtc(0x53, restore->CR53);
    __svgalib_outcrtc(0x66, restore->CR66);
    __svgalib_outcrtc(0x3a, restore->CR3A);
}


/* Return nonzero if mode is available */

static int modeavailable(int mode)
{
    struct vgainfo *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];
    
    if (memory * 1024 < info->ydim * info->xbytes)
	return 0;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    if(modeinfo->bytesPerPixel==3) return 0;

    modetiming = malloc(sizeof(ModeTiming));

    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 0;
    }

    free(modetiming);
    free(modeinfo);
    return SVGADRV;
}

/* Local, called by setmode(). */
#define BASE_FREQ 14.31818

static void savageCalcClock(long freq, int min_m, int min_n1, int max_n1, int min_n2, int max_n2, 
		long freq_min, long freq_max, unsigned int *mdiv, unsigned int *ndiv, unsigned int *r)
{
   double ffreq, ffreq_min, ffreq_max;
   double div, diff, best_diff;
   unsigned int m;
   unsigned char n1, n2;
   unsigned char best_n1=16+2, best_n2=2, best_m=125+2;

   ffreq     = freq     / 1000.0 / BASE_FREQ;
   ffreq_min = freq_min / 1000.0 / BASE_FREQ;
   ffreq_max = freq_max / 1000.0 / BASE_FREQ;

   if (ffreq < ffreq_min / (1<<max_n2)) {
      ffreq = ffreq_min / (1<<max_n2);
   }
   if (ffreq > ffreq_max / (1<<min_n2)) {
      ffreq = ffreq_max / (1<<min_n2);
   }

   /* work out suitable timings */

   best_diff = ffreq;
   
   for (n2=min_n2; n2<=max_n2; n2++) {
      for (n1 = min_n1+2; n1 <= max_n1+2; n1++) {
	 m = (int)(ffreq * n1 * (1<<n2) + 0.5) ;
	 if (m < min_m+2 || m > 127+2) 
	    continue;
	 div = (double)(m) / (double)(n1);	 
	 if ((div >= ffreq_min) &&
	     (div <= ffreq_max)) {
	    diff = ffreq - div / (1<<n2);
	    if (diff < 0.0) 
	       diff = -diff;
	    if (diff < best_diff) {
	       best_diff = diff;
	       best_m    = m;
	       best_n1   = n1;
	       best_n2   = n2;
	    }
	 }
      }
   }
   
   *ndiv = best_n1 - 2;
   *r = best_n2;
   *mdiv = best_m - 2;
}


static void initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{ 
#define NL (1<<30)
    int i, j, dclk, width, tmp;
    int clocklimits[14][2]={ 
        {},
        {86000,0}, /* Trio 64 - Guess */
        {115000,115000}, /* Trio 3D */
        {NL,0}, /* Trio 3D2X */
        {80000,0}, /* Virge */
        {110000,110000}, /* Virge VX */
        {80000,0}, /* Virge DX */
        {NL,0}, /* Virge GX2 */
        {NL,0}, /* Virge MX */
        {NL,NL}, /* Savage 3D */
        {0,0}, /* Savage MX */
        {NL,NL}, /* Savage 4 */
        {NL,NL}, /* Savage Pro */
        {230000,230000}, /* Savage 2000 */
    };
    int cargs[14][4]= {
        {},
        {31,  0, 3, 86000}, /* Trio 64 - Guess */
        {31,  0, 4, 230000},
        {31,  0, 4, 170000},
        {31,  0, 3, 135000},
        {31,  0, 4, 220000},
        {31,  0, 4, 135000},
        {31,  0, 4, 170000},
        {31,  0, 4, 170000},
        {127, 0, 4, 180000},
        {127, 0, 4, 180000},
        {127, 0, 4, 180000},
        {127, 0, 4, 180000},
        {127, 0, 4, 180000},
    };
#undef NL
    
    vgaS3VPtr new = (vgaS3VPtr)(moderegs+VGA_TOTAL_REGS);
   
    if(modeinfo->bitsPerPixel==16) {
        if((chipset==VIRGE)|| (chipset==TRIO64) || (chipset==VIRGEDX)) {
            modetiming->HDisplay *=2;
            modetiming->HSyncStart *=2;
            modetiming->HSyncEnd *=2;
            modetiming->HTotal *=2;
            modetiming->CrtcHDisplay =modetiming->HDisplay;
            modetiming->CrtcHSyncStart =modetiming->HSyncStart;
            modetiming->CrtcHSyncEnd =modetiming->HSyncEnd;
            modetiming->CrtcHTotal =modetiming->HTotal;
        }
    }

    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

    tmp = __svgalib_incrtc(0x3a);
    if((chipset==TRIO3D2X)||(chipset==VIRGEGX2)||(chipset==VIRGEMX)) {
        if(0) new->CR3A = tmp | 0x90;			/* ENH 256, no PCI burst! */
            else new->CR3A = (tmp & 0x38) | 0x10;	/* ENH 256, PCI burst */
    } else {
        if(0) new->CR3A = tmp | 0x95;			/* ENH 256, no PCI burst! */
            else new->CR3A = (tmp & 0x7f) | 0x15;	/* ENH 256, PCI burst */
    }

    new->CR53 |= 0x08;     	/* Disables MMIO */
    new->CR31 = 0x09;     	/* Enable 64k window */

    if(chipset==VIRGEVX) {
        new->CR66 = 0x90; 
        new->CR63 = 0x09; 
        new->CR58 = 0x40;
    } else {
        if((chipset==TRIO3D2X)||(chipset==VIRGEGX2)||(chipset==VIRGEMX)) {
            new->CR63 = 0x08;
        } else {
            new->CR63 = 0x00;
        }
        new->CR66 = 0x89; 
        new->CR58 = 0;
    }
    
/* Now do clock PLL programming. Use the s3gendac function to get m,n */
/* Also determine if we need doubling etc. */

    dclk = modetiming->pixelClock;
    new->CR67 = 0x00;             /* Defaults */
    if(chipset > TRIO3D) {
        new->SR15 = 0x03 | 0x80; 
        if(chipset==VIRGE)
           new->SR15 = (__svgalib_inseq(0x15) & 0x80) | 3; 
    } else {
        new->SR15 = (__svgalib_inseq(0x15) & 0x80) | 3; 
        new->SR0A = __svgalib_inseq(0x0a); 
    }
    new->SR18 = 0x00;
    new->CR43 = 0x00;
    new->CR45 = 0x00;
    new->CR65 = 0x00;
    new->CR54 = 0x00;
    
    if(chipset > TRIO3D)
        new->CR40 = __svgalib_incrtc(0x40) & 0xfe ;
   
    new->SR10 = 255; /* This is a reserved value, so we use as flag */
    new->SR11 = 255;

	new->SR1B = 0;
	new->SR30 = __svgalib_inseq(0x30);

    switch( modeinfo->colorBits ) {
        case 8:
            new->CR67 = 0x00;   /* 8bpp, 1 pixel/clock */
            if(dclk >= clocklimits[chipset][0]) new->CR67 |= 0x10;
            break;
        case 15:
            new->CR67 = 0x20;
            if(dclk >= clocklimits[chipset][1]) new->CR67 |= 0x10;
            break;
        case 16:
            new->CR67 = 0x40;
            if(dclk >= clocklimits[chipset][1]) new->CR67 |= 0x10;
            break;
        case 24:
            new->CR67 = 0xd0;
            break;
    }
    
    /* Now the special cases */ 
    if(chipset==VIRGE) {
        if(new->CR67 == 0x10) {
            new->SR15 |= 0x10;
            new->SR18 =  0x80;
        }
    }
    
    {
        unsigned int m, n, r;

        savageCalcClock(dclk, 1, 1, cargs[chipset][0], cargs[chipset][1], 
          cargs[chipset][2], cargs[chipset][3], cargs[chipset][3]*2,
          &m, &n, &r);
        if(chipset < SAVAGE3D) 
            new->SR12 = (r << 5) | (n & 0x1F);
            else new->SR12 = (r << 6) | (n & 0x3F);
        new->SR13 = m & 0xFF;
        new->SR29 = (r & 4) | (m & 0x100) >> 5 | (n & 0x40) >> 2;
    }


   /* If we have an interlace mode, set the interlace bit. Note that mode
    * vertical timings are already adjusted by the standard VGA code 
    */
    if (modetiming->flags & INTERLACED) {
        new->CR42 = 0x20; /* Set interlace mode */
    } else {
        new->CR42 = 0x00;
    }

    /* Set display fifo */
    if((chipset==TRIO3D2X)||(chipset==VIRGEGX2)||(chipset==VIRGEMX)) {
        new->CR34 = 0;
    } else {
        new->CR34 = 0x10;
    }

    /* Now we adjust registers for extended mode timings */
    /* This is taken without change from the accel/s3_virge code */

    i = ((((modetiming->CrtcHTotal >> 3) - 5) & 0x100) >> 8) |
        ((((modetiming->CrtcHDisplay >> 3) - 1) & 0x100) >> 7) |
        ((((modetiming->CrtcHSyncStart >> 3) - 1) & 0x100) >> 6) |
        ((modetiming->CrtcHSyncStart & 0x800) >> 7);

    if ((modetiming->CrtcHSyncEnd >> 3) - (modetiming->CrtcHSyncStart >> 3) > 64)
        i |= 0x08;   /* add another 64 DCLKs to blank pulse width */

    if ((modetiming->CrtcHSyncEnd >> 3) - (modetiming->CrtcHSyncStart >> 3) > 32)
        i |= 0x20;   /* add another 32 DCLKs to hsync pulse width */

    j = (  moderegs[0] + ((i&0x01)<<8)
         + moderegs[4] + ((i&0x10)<<4) + 1) / 2;

    if (j-(moderegs[4] + ((i&0x10)<<4)) < 4) {
        if (moderegs[4] + ((i&0x10)<<4) + 4 <= moderegs[0]+ ((i&0x01)<<8))
            j = moderegs[4] + ((i&0x10)<<4) + 4;
        else
            j = moderegs[0]+ ((i&0x01)<<8) + 1;
    }
    
    new->CR3B = j & 0xFF;
    i |= (j & 0x100) >> 2;
    new->CR3C = (moderegs[0] + ((i&0x01)<<8))/2;

    new->CR5D = i;

    new->CR5E = (((modetiming->CrtcVTotal - 2) & 0x400) >> 10)  |
                (((modetiming->CrtcVDisplay - 1) & 0x400) >> 9) |
                (((modetiming->CrtcVSyncStart) & 0x400) >> 8)   |
                (((modetiming->CrtcVSyncStart) & 0x400) >> 6)   | 0x40;
   
    width = modeinfo->lineWidth >> 3;
    moderegs[19] = 0xFF & width;
    new->CR51 = (0x300 & width) >> 4; /* Extension bits */
   
    /* And finally, select clock source 2 for programmable PLL */
    moderegs[VGA_MISCOUTPUT] |= 0x0c;      

    if(chipset>=SAVAGE3D) {
        /* Set frame buffer description */
        if (modeinfo->colorBits <= 8) {
            new->CR50 = 0;
        } else {
            if (modeinfo->colorBits <= 16) {
                new->CR50 = 0x10;
            } else {
                new->CR50 = 0x30;
            }
        }
    
        if (modeinfo->width == 640)
            new->CR50 |= 0x40;
        else if (modeinfo->width == 800)
            new->CR50 |= 0x80;
        else if (modeinfo->width == 1024);
        else if (modeinfo->width == 1152)
            new->CR50 |= 0x01;
        else if (modeinfo->width == 1280)
            new->CR50 |= 0x41;
        else if (modeinfo->width == 2048 && new->CR31 & 2);
        else if (modeinfo->width == 1600)
            new->CR50 |= 0x81; /* TODO: need to consider bpp=4 */
        else
            new->CR50 |= 0xC1; /* default to use GlobalBD */

        new->CR33 = 0x08;
        new->CR6F = __svgalib_incrtc(0x6f);
        new->CR86 = __svgalib_incrtc(0x86);
        new->CR88 = __svgalib_incrtc(0x88);
        new->CRB0 = __svgalib_incrtc(0xb0) | 0x80;
    } else /* trio, virge */ {
        new->CR33 = 0x20;
        if((chipset==VIRGEDX)||(chipset<=TRIO3D)) {
            new->CR86 = 0x80;
        }

        if((chipset!=VIRGEVX)&&(chipset!=VIRGE)) {
            new->CR91 = (modeinfo->lineWidth+7) >> 3;
            new->CR90 = 0x80 | ((modeinfo->lineWidth+7) >> 11);
        }    

        if(chipset == VIRGEVX) {
            if(modeinfo->colorBits>16) new->CR6D = 0x51 ; else new->CR6D=0;
        } else {
            new->CR6D = __svgalib_incrtc(0x6d);
            new->CR65 &= ~0x38;
            switch(modeinfo->colorBits) {
                case 8:
                    break;
                case 15:
                case 16:
                    new->CR65 |= 2<<3;
                    break;
                default:
                    new->CR65 |= 4<<3;
                    break;
            }
        }

        if(chipset == VIRGEMX) {
            new->SR54=0x10;
            new->SR55=0x80;
            new->SR56=0x10;
            new->SR57=0x80;
        } else {
            new->SR54=0x1f;
            new->SR55=0x9f;
            new->SR56=0x1f;
            new->SR57=0x9f;
        }
    }

   /* Now we handle various XConfig memory options and others */

    new->CR36 = __svgalib_incrtc(0x36);
   
#if 0
   if (mode->Private) {
      new->CR67 &= ~1;
      if( 
        (s3vPriv.chip != S3_SAVAGE2000) &&
        (mode->Private[0] & (1 << S3_INVERT_VCLK)) &&
	(mode->Private[S3_INVERT_VCLK])
      )
	 new->CR67 |= 1;

      if (mode->Private[0] & (1 << S3_BLANK_DELAY)) {
	 new->CR65 = (new->CR65 & ~0x38) 
	    | (mode->Private[S3_BLANK_DELAY] & 0x07) << 3;
      }
   }
#endif

    if(__svgalib_emulatepage || is_linear) new->CR58 |= 0x13;
    new->CR68 = __svgalib_incrtc(0x68);
    new->CR69 = 0;

    return ;
}


static int local_setmode(int mode, int prv_mode)
{
    unsigned char *moderegs;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {
        __svgalib_outcrtc(0x34, 0);
	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    }
    if (!modeavailable(mode))
	return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 1;
    }

    moderegs = malloc(TOTAL_REGS);

    initializemode(moderegs, modetiming, modeinfo, mode);
    free(modetiming);

    __svgalib_setregs(moderegs);	/* Set standard regs. */
    setregs(moderegs, mode);		/* Set extended regs. */
    free(moderegs);

    free(modeinfo);
    return 0;
}


/* Unlock chipset-specific registers */

static void unlock(void)
{
    __svgalib_outcrtc(0x11,__svgalib_incrtc(0x11)&0x7f);
    __svgalib_outcrtc(0x38, 0x48);
    __svgalib_outcrtc(0x39, 0xa5);
    __svgalib_outcrtc(0x40,__svgalib_incrtc(0x40)&0xfe);   
}

static void lock(void)
{
}


#define VENDOR_ID 0x5333

/* Indentify chipset, initialize and return non-zero if detected */

static int test(void)
{
    int found;
    int id;
    unsigned long buf[64];
    
    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    id=(buf[0]>>16)&0xffff;
    if(found)return 0;
    switch(id) {
        case 0x8811:
        case 0x8903:
        case 0x8904:
        case 0x8a13:
        case 0x5631:
        case 0x883d:
        case 0x8a01:
        case 0x8a10:
        case 0x8c00:
        case 0x8c01:
        case 0x8c02:
        case 0x8c03:
        case 0x8a20:
        case 0x8a21:
        case 0x8a22:
        case 0x8a23:
        case 0x8c10:
        case 0x8c11:
        case 0x8c12:
        case 0x8c13:
        case 0x9102:
		case 0x8d03:
		case 0x8d04:
            init(0,0,0);
            return 1;
            break;
        default:
            return 0;
    }
}


/* Set display start address (not for 16 color modes) */

static void setdisplaystart(int address)
{ 
  address=address >> 2;
  __svgalib_outcrtc(0x0d,address&0xff);
  __svgalib_outcrtc(0x0c,(address>>8)&0xff);
  __svgalib_outcrtc(0x69,(address>>16)&0xff);
  
}

/* Set logical scanline length (usually multiple of 8) */
/* Cirrus supports multiples of 8, up to 4088 */

static void setlogicalwidth(int width)
{   
    int offset = width >> 3;
 
    __svgalib_outcrtc(0x13,offset&0xff);
}

static int linear(int op, int param)
{
    if (op==LINEAR_ENABLE){
       __svgalib_outcrtc(0x58,__svgalib_incrtc(0x58)|0x13);
       is_linear=1; 
       return 0;
    };
    if (op==LINEAR_DISABLE) {
       __svgalib_outcrtc(0x58,__svgalib_incrtc(0x58)&~0x13);
       is_linear=0; 
       return 0;
    };
    if (op==LINEAR_QUERY_BASE) return linear_base;
    if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY) return 0;		/* No granularity or range. */
        else return -1;		/* Unknown function. */
}

static int match_programmable_clock(int clock)
{
return clock ;
}

static int map_clock(int bpp, int clock)
{
return clock ;
}

static int map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
return htiming;
}

static struct {
    unsigned char c8;
    unsigned short c15;
    unsigned short c16;
    unsigned int c32;
} cursor_colors[16*2];

static int pal=1, palette[768];

static int findcolor(int rgb) {
   int i,j,k,l=0;

   if(pal)vga_getpalvec(0,256,palette);
   pal=0;
   k=0xffffff;
   for(i=0;i<256;i++) {
      j=((rgb&0xff)-(palette[i*3+2]<<2))*((rgb&0xff)-(palette[i*3+2]<<2))+
        (((rgb>>8)&0xff)-(palette[i*3+1]<<2))*(((rgb>>8)&0xff)-(palette[i*3+1]<<2))+
        (((rgb>>16)&0xff)-(palette[i*3]<<2))*(((rgb>>16)&0xff)-(palette[i*3]<<2));
      if(j==0) {
         return i;
      }
      if(j<k) {
         k=j;
         l=i;
      }
   }
   return l;
}

static int cursor( int cmd, int p1, int p2, int p3, int p4, void *p5) {
    unsigned long *b3;
    unsigned char *buf;
    int i, j; 
    unsigned int l1, l2;
    
    switch(cmd){
        case CURSOR_INIT:
            return 1;
        case CURSOR_HIDE:
            __svgalib_outcrtc(0x45,__svgalib_incrtc(0x45)&0xfe);
            break;
        case CURSOR_SHOW:
            __svgalib_outcrtc(0x45,__svgalib_incrtc(0x45)|0x01); /* Windows */
            break;
        case CURSOR_POSITION:
            __svgalib_outcrtc(0x46,p1>>8);
            __svgalib_outcrtc(0x47,p1&0xff);
            __svgalib_outcrtc(0x49,p2&0xff);
            __svgalib_outcrtc(0x4e,0);
            __svgalib_outcrtc(0x4f,0);
            __svgalib_outcrtc(0x48,p2>>8);
            break;
        case CURSOR_SELECT:
            i=memory-(16-p1);
            switch(CI.colors) {
                case 256:
                    __svgalib_incrtc(0x45);
                    __svgalib_outcrtc(0x4b,cursor_colors[p1*2].c8);
                    __svgalib_outcrtc(0x4b,cursor_colors[p1*2].c8);
                    __svgalib_incrtc(0x45);
                    __svgalib_outcrtc(0x4a,cursor_colors[p1*2+1].c8);
                    __svgalib_outcrtc(0x4a,cursor_colors[p1*2+1].c8);
                   break;
                case 32768:
                    __svgalib_incrtc(0x45);
                    __svgalib_outcrtc(0x4b,cursor_colors[p1*2].c15&0xff);
                    __svgalib_outcrtc(0x4b,cursor_colors[p1*2].c15>>8);
                    __svgalib_outcrtc(0x4b,cursor_colors[p1*2].c15&0xff);
                    __svgalib_outcrtc(0x4b,cursor_colors[p1*2].c15>>8);
                    __svgalib_incrtc(0x45);
                    __svgalib_outcrtc(0x4a,cursor_colors[p1*2+1].c15&0xff);
                    __svgalib_outcrtc(0x4a,cursor_colors[p1*2+1].c15>>8);
                    __svgalib_outcrtc(0x4a,cursor_colors[p1*2+1].c15&0xff);
                    __svgalib_outcrtc(0x4a,cursor_colors[p1*2+1].c15>>8);
                   break;
                case 65536:
                    __svgalib_incrtc(0x45);
                    __svgalib_outcrtc(0x4b,cursor_colors[p1*2].c16&0xff);
                    __svgalib_outcrtc(0x4b,cursor_colors[p1*2].c16>>8);
                    __svgalib_outcrtc(0x4b,cursor_colors[p1*2].c16&0xff);
                    __svgalib_outcrtc(0x4b,cursor_colors[p1*2].c16>>8);
                    __svgalib_incrtc(0x45);
                    __svgalib_outcrtc(0x4a,cursor_colors[p1*2+1].c16&0xff);
                    __svgalib_outcrtc(0x4a,cursor_colors[p1*2+1].c16>>8);
                    __svgalib_outcrtc(0x4a,cursor_colors[p1*2+1].c16&0xff);
                    __svgalib_outcrtc(0x4a,cursor_colors[p1*2+1].c16>>8);
                   break;
                case (1<<24):
                    __svgalib_incrtc(0x45);
                    __svgalib_outcrtc(0x4b,cursor_colors[p1*2].c32&0xff);
                    __svgalib_outcrtc(0x4b,(cursor_colors[p1*2].c32>>8)&0xff);
                    __svgalib_outcrtc(0x4b,(cursor_colors[p1*2].c32>>16)&0xff);
                    __svgalib_incrtc(0x45);
                    __svgalib_outcrtc(0x4a,cursor_colors[p1*2+1].c32&0xff);
                    __svgalib_outcrtc(0x4a,(cursor_colors[p1*2+1].c32>>8)&0xff);
                    __svgalib_outcrtc(0x4a,(cursor_colors[p1*2+1].c32>>16)&0xff);
                   break;
            }
            __svgalib_outcrtc(0x4d, i&0xff);
            __svgalib_outcrtc(0x4c, i>>8);
            break;
        case CURSOR_IMAGE:
            buf=malloc(1024);
            cursor_colors[p1*2].c8=findcolor(p3);
            cursor_colors[p1*2].c32=p3;
            cursor_colors[p1*2].c16=((p3&0xf80000)>>8)|((p3&0xfc00)>>5)|((p3&0xf8)>>3);
            cursor_colors[p1*2].c15=((p3&0xf80000)>>9)|((p3&0xf800)>>5)|((p3&0xf8)>>3);
            cursor_colors[p1*2+1].c8=findcolor(p4);
            cursor_colors[p1*2+1].c32=p4;
            cursor_colors[p1*2+1].c16=((p4&0xf80000)>>8)|((p4&0xfc00)>>5)|((p4&0xf8)>>3);
            cursor_colors[p1*2+1].c15=((p4&0xf80000)>>9)|((p4&0xf800)>>5)|((p4&0xf8)>>3);
            i=memory*1024-(16-p1)*1024;
            b3=(unsigned long *)p5;
            switch(p2) {
                case 0:
                    for(j=0;j<32;j++) {
                        l2=*(b3+j);
                        l1=*(b3+32+j);
                        l1=BE32(l1);
                        l2=BE32(l2);
                        l2=l2&l1;
                        l1=~l1;
                        *(unsigned short *)(buf+16*j)=l1&0xffff;
                        *(unsigned short *)(buf+16*j+2)=l2&0xffff;
                        *(unsigned short *)(buf+16*j+4)=(l1>>16)&0xffff;
                        *(unsigned short *)(buf+16*j+6)=(l2>>16)&0xffff;
                        *(unsigned short *)(buf+16*j+8)=0xffff;
                        *(unsigned short *)(buf+16*j+10)=0;
                        *(unsigned short *)(buf+16*j+12)=0xffff;
                        *(unsigned short *)(buf+16*j+14)=0;
                    }
                    for(j=32;j<64;j++) {
                        *(unsigned short *)(buf+16*j)=0xffff;
                        *(unsigned short *)(buf+16*j+2)=0;
                        *(unsigned short *)(buf+16*j+4)=0xffff;
                        *(unsigned short *)(buf+16*j+6)=0;
                        *(unsigned short *)(buf+16*j+8)=0xffff;
                        *(unsigned short *)(buf+16*j+10)=0;
                        *(unsigned short *)(buf+16*j+12)=0xffff;
                        *(unsigned short *)(buf+16*j+14)=0;
                    }
                    break;
            }
            vga_drawscansegment(buf, i/CI.bytesperpixel,0,1024);
            break;
    }
    return 0;
}       

/* Function table (exported) */
DriverSpecs __svgalib_savage_18_driverspecs =
{
    saveregs,
    setregs,
    unlock,
    lock,
    test,
    init,
    setpage,
    NULL,
    NULL,
    local_setmode,
    modeavailable,
    setdisplaystart,
    setlogicalwidth,
    getmodeinfo,
    0,				/* old blit funcs */
    0,
    0,
    0,
    0,
    0,				/* ext_set */
    0,				/* accel */
    linear,
    0,				/* accelspecs, filled in during init. */
    NULL,                       /* Emulation */
    cursor,
};

/* Initialize chipset (called after detection) */

static int init(int force, int par1, int par2)
{
    unsigned long buf[64];
    unsigned long mmio_base;
    int found=0, config1;
    int mems[8]={2,4,8,12,16,32,64,2};
    char *chipnames[] = {"Unknown", "Trio64", "Trio 3D", "Trio 3d/2X", "Virge", "Virge VX", 
        		 "Virge DX", "Virge GX2", "Virge MX",
        		 "Savage3D", "SavageMX", "Savage4", "SavagePro", "Savage2000"};
    int vmems[9][8]= { {0},
        {0,0,0,1,0,0,1,0},
        {4,0,4,0,2,0,0,0},
        {4,0,4,0,0,0,2,0},
        {4,0,0,0,2,0,1,0},
        {2,4,6,8,2,4,6,8},
        {4,0,0,0,2,0,1,0},
        {0,0,4,4,0,0,2,2},
        {0,0,4,4,0,0,2,2}
    };
    int id;
    
//    unlock();
    if (force) {
	memory = par1;
        chipset = par2;
    } else {

    };

    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);

    id=(buf[0]>>16)&0xffff;

    switch(id) {
        case 0x8811:
            chipset = TRIO64;
            break;
        case 0x8903:
        case 0x8904:
            chipset = TRIO3D;
            break;
        case 0x8a13:
            chipset = TRIO3D2X;
            break;
        case 0x5631:
            chipset = VIRGE;
            break;
        case 0x883d:
            chipset = VIRGEVX;
            break;
        case 0x8a01:
            chipset = VIRGEDX;
            break;
        case 0x8a10:
            chipset = VIRGEGX2;
            break;
        case 0x8c00:
        case 0x8c01:
        case 0x8c02:
        case 0x8c03:
            chipset = VIRGEMX;
            break;
        case 0x8a20:
        case 0x8a21:
            chipset = SAVAGE3D;
            break;
        case 0x8c10:
        case 0x8c11:
        case 0x8c12:
        case 0x8c13:
            chipset = SAVAGEMX;
            break;
        case 0x8a22:
        case 0x8a23:

		case 0x8d03:
		case 0x8d04:
            chipset = SAVAGE4;
            break;
        case 0x9102:
            chipset = SAVAGE2000;
            break;
        default:
            chipset = UNKNOWN;
    }
    
    if(chipset<SAVAGE3D) {
        linear_base=buf[4]&0xffffff00;
        mmio_base = linear_base + 0x1000000;
#if 1 /* You need to write linear address to CR59 5A, and enable MMIO in CR53 - 
         But how to do it if it's a secondary card??? */
        if(__svgalib_secondary) {
            __svgalib_mmio_base = mmio_base;
            __svgalib_mmio_size = 0x10000;
            map_mmio();
            __svgalib_vgammbase=0x8000; 
            __svgalib_mm_io_mapio();
        }
#endif
        unlock();
        config1=__svgalib_incrtc(0x36);
        memory = 1024 * vmems[chipset][(config1&0xe0)>>5];
    } else {
        linear_base=buf[5]&0xffffff00;
        mmio_base  =buf[4]&0xffffff00;
        __svgalib_mmio_base = mmio_base;
        __svgalib_mmio_size = 0x10000;
        map_mmio();
    
        __svgalib_vgammbase=0x8000;
        __svgalib_mm_io_mapio();
        unlock();
    
        config1=__svgalib_incrtc(0x36);
        if(chipset >= SAVAGE4) {
            memory=mems[config1>>5]*1024;
        } else {
            switch(config1>>6) {
                case 0:
                    memory=8192;
                    break;
                case 0x40:
                case 0x80:
                    memory=4096;
                    break;
                case 0xC0:
                    memory=2048;
                    break;
            }
        }
    }

    if (__svgalib_driver_report) {
	fprintf(stderr,"Using SAVAGE driver, %iKB. Chipset: %s\n",memory, chipnames[chipset]);
    };
    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = memory;
    cardspecs->maxPixelClock4bpp = 0;	
    cardspecs->maxPixelClock8bpp = 250000;	
    cardspecs->maxPixelClock16bpp = 250000;	
    cardspecs->maxPixelClock24bpp = 220000;
    cardspecs->maxPixelClock32bpp = 220000;
    cardspecs->flags = INTERLACE_DIVIDE_VERT | CLOCK_PROGRAMMABLE;
    cardspecs->maxHorizontalCrtc = 4088;
    cardspecs->nClocks = 0;
    cardspecs->mapClock = map_clock;
    cardspecs->mapHorizontalCrtc = map_horizontal_crtc;
    cardspecs->matchProgrammableClock=match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_savage_18_driverspecs;
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=linear_base;
    __svgalib_linear_mem_size=memory*0x400;
    return 0;
}

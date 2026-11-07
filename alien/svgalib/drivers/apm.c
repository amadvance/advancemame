/*
At3d (at25) driver - Matan Ziv-Av, matan@svgalib.org
please report problems to me,

If you have an alliance promotion 6422, it should
be easy to make the driver work on it.

This driver is based on the XFREE86 apm driver, developed by
Kent Hamilton and by Henrik Harmsen.

I used the file cirrus.c in this directory as a skeleton.

there are still the following problems:
  * pageflipping (in threeDKit) does not work. 
  * no acceleration (is there a program that uses it anyway?).
  * The clocks for 320x200 modes are _not easy_ for the driver.
    if you have a problem with those modes, please contact me.
  * 24 bits modes don't work on AT24.
  
*/


#include <stdlib.h>
#include <stdio.h>		/* for printf */
#include <string.h>		/* for memset */
#include <unistd.h>
#include <sys/mman.h>
#include "vga.h"
#include "libvga.h"
#include "svgadriv.h"
#include "vgapci.h"

/* New style driver interface. */
#include "timing.h"
#include "vgaregs.h"
#include "vgammvga.h"
#include "interfac.h"

#define APMREG_SAVE(i) (VGA_TOTAL_REGS+i)
#define APM_TOTAL_REGS (VGA_TOTAL_REGS + 38)

static int apm_init(int, int, int);
static void apm_unlock(void);

static int apm_memory,apm_chiptype;
static int apm_is_linear,apm_xbase;

static CardSpecs *cardspecs;

static void outXR(int index,unsigned char d)
{
    __svgalib_outseq(0x1d,index>>2);
    port_out_r(apm_xbase,d);
}

static void outwXR(int index,unsigned short d)
{
    __svgalib_outseq(0x1d,index>>2);
    port_outw_r(apm_xbase,d);
}

static void outlXR(int index,unsigned long d)
{
    __svgalib_outseq(0x1d,index>>2);
    port_outl_r(apm_xbase,d);
}

static unsigned char inXR(int index)
{
    __svgalib_outseq(0x1d,index>>2);
    return port_in(apm_xbase);
}

#if 0
/* currently not used */
static unsigned short inwXR(int index)
{
    __svgalib_outseq(0x1d,index>>2);
    return inw(apm_xbase);
}
#endif

static unsigned long inlXR(int index)
{
    __svgalib_outseq(0x1d,index>>2);
    return port_inl(apm_xbase);
}

enum {
    AT6420 = 0, AT6422, AT24, AT3D
};

static void apm_setpage(int page)
{
    /* default 4K granularity */
    outwXR(0xc0,page << 4);
}

static int __svgalib_apm_inlinearmode(void)
{
    return apm_is_linear;
}

/* Fill in chipset specific mode information */

static void apm_getmodeinfo(int mode, vga_modeinfo *modeinfo)
{
    if(modeinfo->colors==16)return;

    modeinfo->maxpixels = apm_memory*1024/modeinfo->bytesperpixel;
    modeinfo->maxlogicalwidth = 4088;
    modeinfo->startaddressrange = apm_memory * 1024 - 1;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;

    if (modeinfo->bytesperpixel >= 1) {
	modeinfo->flags |= CAPABLE_LINEAR;
        if (__svgalib_apm_inlinearmode())
	    modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
    }
}

/* Read and save chipset-specific registers */

static int apm_saveregs(unsigned char regs[])
{ 
	unsigned long k;
	unsigned long *p;
	unsigned short *q;

    apm_unlock();		/* May be locked again by other programs (e.g. X) */
    regs[APMREG_SAVE(2)] = __svgalib_incrtc(0x19);
    regs[APMREG_SAVE(3)] = __svgalib_incrtc(0x1A);
    regs[APMREG_SAVE(4)] = __svgalib_incrtc(0x1B);
    regs[APMREG_SAVE(5)] = __svgalib_incrtc(0x1C);
    regs[APMREG_SAVE(6)] = __svgalib_incrtc(0x1D);
    regs[APMREG_SAVE(7)] = __svgalib_incrtc(0x1E);

    regs[APMREG_SAVE(0)] = __svgalib_inseq(0x1B);
    regs[APMREG_SAVE(1)] = __svgalib_inseq(0x1C);

   regs[APMREG_SAVE(8)] = inXR(0x80) ;
   regs[APMREG_SAVE(9)] = inXR(0xc0) ;
   k = inlXR(0xe8) ;
   regs[APMREG_SAVE(10)] = k&0xff;
   regs[APMREG_SAVE(11)] = (k >> 8)&0xff;
   regs[APMREG_SAVE(12)] = (k >> 16) & 0xff ;
   regs[APMREG_SAVE(13)] = ( k >> 24 ) & 0xff ;
   k = inlXR(0xec) ;
   regs[APMREG_SAVE(14)] = k&0xff;
   regs[APMREG_SAVE(15)] = (k >> 8)&0xff;
   regs[APMREG_SAVE(16)] = (k >> 16) & 0xff ;
   regs[APMREG_SAVE(17)] = ( k >> 24 ) & 0xff ;
   p=(unsigned long *) &regs[APMREG_SAVE(18)];
   *p = inlXR(0xf0);
   p=(unsigned long *) &regs[APMREG_SAVE(22)];
   *p = inlXR(0xf4);
   p=(unsigned long *) &regs[APMREG_SAVE(26)];
   *p = inlXR(0x140);
   q=(unsigned short *) &regs[APMREG_SAVE(30)];
   *q = inlXR(0x144);
   p=(unsigned long *) &regs[APMREG_SAVE(32)];
   *p = inXR(0x148);
   q=(unsigned short *) &regs[APMREG_SAVE(36)];
   *q = inXR(0x14c);

    return APM_TOTAL_REGS - VGA_TOTAL_REGS;
}

/* Set chipset-specific registers */

static void apm_setregs(const unsigned char regs[], int mode)
{  unsigned long k ; 

    apm_unlock();		/* May be locked again by other programs (eg. X) */
    apm_setpage(0);

    __svgalib_outseq(0x1b,regs[APMREG_SAVE(0)]);
    __svgalib_outseq(0x1c,regs[APMREG_SAVE(1)]);
    __svgalib_outcrtc(0x19,regs[APMREG_SAVE(2)]);
    __svgalib_outcrtc(0x1a,regs[APMREG_SAVE(3)]);
    __svgalib_outcrtc(0x1b,regs[APMREG_SAVE(4)]);
    __svgalib_outcrtc(0x1c,regs[APMREG_SAVE(5)]);
    __svgalib_outcrtc(0x1d,regs[APMREG_SAVE(6)]);
    __svgalib_outcrtc(0x1e,regs[APMREG_SAVE(7)]);

   if(apm_chiptype==AT3D) {
     k=regs[APMREG_SAVE(10)]+(regs[APMREG_SAVE(11)] << 8)+(regs[APMREG_SAVE(12)] << 16)+(regs[APMREG_SAVE(13)] << 24);
     outlXR(0xe8,k) ;
   };
   
   k=regs[APMREG_SAVE(14)]+(regs[APMREG_SAVE(15)] << 8)+(regs[APMREG_SAVE(16)] << 16)+(regs[APMREG_SAVE(17)] << 24);
   outlXR(0xec,k & ~(1 << 7)) ;
   outlXR(0xec,k | (1 << 7)) ;

   outXR(0x80,regs[APMREG_SAVE(8)]) ;

}


/* Return nonzero if mode is available */

static int apm_modeavailable(int mode)
{
    struct vgainfo *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

  
    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];
    if (apm_memory * 1024 < info->ydim * info->xbytes)
	return 0;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    if((modeinfo->bitsPerPixel==24)&&(apm_chiptype==AT24)) {
	free(modeinfo);
	return 0;
    }

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

static unsigned comp_lmn(unsigned clock) ;
/* Set a mode */

/* Local, called by apm_setmode(). */

static void apm_initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{
    long k;
  
    apm_saveregs(moderegs);
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

  {
    int offset;
    offset = modeinfo->lineWidth >> 3;
    moderegs[0x13] = offset;
    /* Bit 8 resides at CR1C bits 7:4. */
    moderegs[APMREG_SAVE(5)] = (offset & 0xf00) >> 4;
  }

  /* Set pixel depth. */
  switch(modeinfo->bitsPerPixel)
  {
    case 8:
         moderegs[APMREG_SAVE(8)] = 0x42;
         break;
    case 16:
         moderegs[APMREG_SAVE(8)] = modeinfo->greenWeight+7;
         break;
    case 24:
         moderegs[APMREG_SAVE(8)] = 0xe;
         break;
    case 32:
         moderegs[APMREG_SAVE(8)] = 0x0f;
         break;
    default:
         break;
  }

  /*
   * Enable VESA Super VGA memory organisation.
   * Also enable Linear Addressing.
   */
  moderegs[APMREG_SAVE(0)] = 0;
  moderegs[APMREG_SAVE(1)] = 21;
  /* Set banking register to zero. */
  moderegs[APMREG_SAVE(9)] = 0;

  /* Handle the CRTC overflow bits. */
  {
    unsigned char val;
    /* Vertical Overflow. */
    val = 0;
    if ((modetiming->CrtcVTotal - 2) & 0x400)
      val |= 0x01;
    if ((modetiming->CrtcVDisplay - 1) & 0x400)
      val |= 0x02;
    /* VBlankStart is equal to VSyncStart + 1. */
    if (modetiming->CrtcVSyncStart & 0x400)
      val |= 0x04;
    /* VRetraceStart is equal to VSyncStart + 1. */
    if (modetiming->CrtcVSyncStart & 0x400)
      val |= 0x08;
    moderegs[APMREG_SAVE(3)] = val;

    /* Horizontal Overflow. */
    val = 0;
    if ((modetiming->CrtcHTotal / 8 - 5) & 0x100)
      val |= 1;
    if ((modetiming->CrtcHDisplay / 8 - 1) & 0x100)
      val |= 2;
    /* HBlankStart is equal to HSyncStart - 1. */
    if ((modetiming->CrtcHSyncStart / 8 - 1) & 0x100)
      val |= 4;
    /* HRetraceStart is equal to HSyncStart. */
    if ((modetiming->CrtcHSyncStart / 8) & 0x100)
      val |= 8;
    moderegs[APMREG_SAVE(4)] = val;
  }
  moderegs[APMREG_SAVE(7)]= 1;          /* disable autoreset feature */

    k = comp_lmn(modetiming->pixelClock);
    if (k==0){       
       k = comp_lmn(modetiming->pixelClock*2);
       if(k>0)moderegs[VGA_SR1] |= 8 ; /* Sequencer1 - clock halving */
    };
    moderegs[APMREG_SAVE(14)]=k&0xff;
    moderegs[APMREG_SAVE(15)]=(k >> 8)&0xff;
    moderegs[APMREG_SAVE(16)]=(k >> 16)&0xff;
    moderegs[APMREG_SAVE(17)]=(k >> 24)&0xff;   
    moderegs[MIS] |= 0xc;
    moderegs[MIS]=0xef; 
  /* Set up the RAMDAC registers. */

  if (modeinfo->bitsPerPixel > 8) {
    /* Get rid of white border. */
      moderegs[VGA_AR11] = 0;
  } else { 
      int p,q,r;

      moderegs[VGA_AR11]=0xff;
      p=moderegs[1]+((moderegs[APMREG_SAVE(4)]&2)<<7); 
      moderegs[2]=p&0xff;
      moderegs[APMREG_SAVE(4)]&=0xfd;
      moderegs[APMREG_SAVE(4)]|=(p&0x100)>>7;
      q=moderegs[4]+((moderegs[APMREG_SAVE(4)]&8)<<5); 
      r=moderegs[0]+4;
      moderegs[3]&=0xe0;
      moderegs[3]|=(r&0x1f);
      moderegs[5]&=0x7f;
      moderegs[5]|=(r&0x20)<<2;
  
      p=moderegs[0x12]+((moderegs[0x7]&2)<<7)+((moderegs[7]&0x40)<<3)+((moderegs[APMREG_SAVE(3)]&2)<<9);
      moderegs[0x15]=p&0xff;
      moderegs[0x7]&=0xf7;
      moderegs[0x7]|=(p&0x100)>>5;
      moderegs[0x9]&=0xdf;
      moderegs[0x9]|=(p&0x200)>>4;
      moderegs[APMREG_SAVE(3)]&=0xfb;
      moderegs[APMREG_SAVE(3)]|=(p&0x400)>>7;
      moderegs[0x16]=moderegs[6]+1;
  }

    moderegs[APMREG_SAVE(10)] = 0xe8;  
    moderegs[APMREG_SAVE(11)] = 0x01;  
    moderegs[APMREG_SAVE(12)] = 0x1f;   
    moderegs[APMREG_SAVE(13)] = 0x07;  

    apm_is_linear=0;

return ;

}



static int apm_setmode(int mode, int prv_mode)
{
    unsigned char *moderegs;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {

	/* Let the standard VGA driver set standard VGA modes */
	/* But first reset an Cirrus extended register that */
	/* an old XFree86 Trident probe corrupts. */
	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    }
    if (!apm_modeavailable(mode))
	return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 1;
    }

    moderegs = malloc(APM_TOTAL_REGS);

    apm_initializemode(moderegs, modetiming, modeinfo, mode);
    free(modetiming);

    __svgalib_setregs(moderegs);	/* Set standard regs. */
    apm_setregs(moderegs, mode);	/* Set extended regs. */
    free(moderegs);

    free(modeinfo);
    return 0;
}


/* Unlock chipset-specific registers */

static void apm_unlock(void)
{
    __svgalib_outcrtc(0x11,__svgalib_incrtc(0x11)&0x7f);
    __svgalib_outseq(0x10, 0x12);
}


/* Relock chipset-specific registers */
/* (currently not used) */

static void apm_lock(void)
{
}


/* Indentify chipset, initialize and return non-zero if detected */

static int apm_test(void)
{
#ifdef OLD_TEST
    int     i;
    int	    oldSEQ10;

/* To remove annoying "unused..." warning
    char    id_ap6420[] = "Pro6420";
    char    id_ap6422[] = "Pro6422";
*/
    char    id_at24[]   = "Pro6424";
    char    id_at3d[]   = "ProAT3D"; /* Yeah, the manual could have been 
                                        correct... */
    char    idstring[]  = "       ";

    /*
     * Warning! A fully compliant VGA will most probably interprete this as
     * __svgalib_outbSR(0x00, 0x02), hence stop the sequencer, to play safe
     * we keep the old setting.
     */

    oldSEQ10 = __svgalib_inSR(0x10);
    __svgalib_outSR(0x10, 0x12);

    for (i = 0; i < 7; i++)
      idstring[i] = __svgalib_inSR(0x11+i);

    /*
     * Just in case, restore any old setting & select SEQ00:
     */
    __svgalib_outSR(0x10, oldSEQ10);
    __svgalib_inSR(0);

    if (!memcmp(id_at3d, idstring, 7) || !memcmp(id_at24,idstring,7)) 
      { apm_init(0,0,0) ; return 1; }
    return 0;
#else /* use PCI test */
   int found=0;
   unsigned long buf[64];
    
   found=__svgalib_pci_find_vendor_vga(0x1142,buf,0);
   if(found)return 0;
   if ((((buf[0]>>16)&0xffff)!=0x6424)&&
       (((buf[0]>>16)&0xffff)!=0x643d)
       )return 0;
   apm_init(0,0,0);
   return 1;
#endif

}


/* No r/w paging */
static void apm_setrdpage(int page)
{
}
static void apm_setwrpage(int page)
{
}


/* Set display start address (not for 16 color modes) */
/* Cirrus supports any address in video memory (up to 2Mb) */

static void apm_setdisplaystart(int address)
{  int i;
  port_outw_r(0x3d4, (address & 0x00FF00) | 0x0C);
  port_outw_r(0x3d4, ((address & 0x00FF) << 8) | 0x0D);

  /*
   * Here the high-order bits are masked and shifted, and put into
   * the appropriate extended registers.
   */
   port_out_r(0x3d4,0x1c);
   i=(port_in(0x3d5)&0xf0)|((address & 0x0f0000) >> 16);
   __svgalib_outCR(0x1c,i);
/*  modinx(vgaIOBase + 4, 0x1c, 0x0f, (address & 0x0f0000) >> 16); */

}


/* Set logical scanline length (usually multiple of 8) */
/* Cirrus supports multiples of 8, up to 4088 */

static void apm_setlogicalwidth(int width)
{
}

static int apm_linear(int op, int param)
{
if (op==LINEAR_ENABLE) { apm_is_linear=1; return 0;}
if (op==LINEAR_DISABLE){ apm_is_linear=0; return 0;}
if (op==LINEAR_QUERY_BASE) {__svgalib_outSR(0x1d,0x193>>2); return port_in(apm_xbase+3)<<24 ;}
if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY) return 0;		/* No granularity or range. */
    else return -1;		/* Unknown function. */
}

static int apm_match_programmable_clock(int clock)
{
return clock ;
}
static int apm_map_clock(int bpp, int clock)
{
return clock ;
}
static int apm_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
return htiming;
}
/* Function table (exported) */

DriverSpecs __svgalib_apm_driverspecs =
{
    apm_saveregs,
    apm_setregs,
    apm_unlock,
    apm_lock,
    apm_test,
    apm_init,
    apm_setpage,
    apm_setrdpage,
    apm_setwrpage,
    apm_setmode,
    apm_modeavailable,
    apm_setdisplaystart,
    apm_setlogicalwidth,
    apm_getmodeinfo,
    0,				/* old blit funcs */
    0,
    0,
    0,
    0,
    0,				/* ext_set */
    0,				/* accel */
    apm_linear,
    0,				/* accelspecs, filled in during init. */
    NULL,                       /* Emulation */
};

/* Initialize chipset (called after detection) */

static int apm_init(int force, int par1, int par2)
{
    char    idstring[]  = "       ";
    int found=0;
    unsigned long buf[64];
    int db, d9;

    apm_unlock();

    found=__svgalib_pci_find_vendor_vga(0x1142,buf,0);
    if(found)return 1;
    switch((buf[0]>>16)&0xffff) {
        case 0x6424: strcpy(idstring,"AT24"); break; 
        case 0x643d: strcpy(idstring,"AT25/3D"); break; 
        default: return 1; break;
    };

    __svgalib_linear_mem_base=buf[4]&0xffffff00;

    __svgalib_mmio_base=__svgalib_linear_mem_base+0xffc000;
    __svgalib_mmio_size=0x4000;
    map_mmio();
    db=v_readb(0x2cdb);
    d9=v_readb(0x2cd9);
    v_writeb((db & 0xf4) | 0x0a, 0x2cdb);
    v_writeb((d9 & 0xcf) | 0x20, 0x2cd9);
    __svgalib_vgammbase = 0x3000;
    __svgalib_mm_io_mapio();

    apm_memory=__svgalib_inseq(0x20)*64-34; /* maybe will support accel some day */
    if (__svgalib_driver_report) {
	fprintf(stderr,"Using Alliance driver, %.7s, %iKB.\n",idstring, apm_memory);
    }

    __svgalib_modeinfo_linearset = IS_LINEAR;

    apm_xbase= (__svgalib_inseq(0x1f) << 8 ) + __svgalib_inseq(0x1e);
    
    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = apm_memory;
    cardspecs->maxPixelClock4bpp = 75000;
    cardspecs->maxPixelClock8bpp = 175500;
    if(apm_chiptype==AT24)cardspecs->maxPixelClock8bpp = 160000;
    cardspecs->maxPixelClock16bpp = 144000;
    cardspecs->maxPixelClock24bpp = 75000;
    cardspecs->maxPixelClock32bpp = 94500;
    cardspecs->flags = CLOCK_PROGRAMMABLE | INTERLACE_DIVIDE_VERT;
    cardspecs->maxHorizontalCrtc = 4088;
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->nClocks =0;
    cardspecs->clocks = NULL;
    cardspecs->mapClock = apm_map_clock;
    cardspecs->mapHorizontalCrtc = apm_map_horizontal_crtc;
    cardspecs->matchProgrammableClock=apm_match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_apm_driverspecs;
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_size=apm_memory*1024;
    return 0;
}

#define WITHIN(v,c1,c2) (((v) >= (c1)) && ((v) <= (c2)))

static unsigned
comp_lmn(unsigned clock)
{
  int n, m, l, f;
  int fvco;
  int fout;
  int fmax;
  int fref;
  int fvco_goal;
  int c;

  if(apm_chiptype==AT3D)
      fmax = 400000; else fmax=265000;
/* The XFree86 driver says the max for AT24 is 250000,
   but there is a much used clock (65MHZ), that uses 260000 fvco,
   and the VESA bios on the card uses this clock, upped the max */   
  
  fref = 14318;

  for (m = 1; m <= 5; m++)
  {
    for (l = 3; l >= 0; l--)
    {
      for (n = 8; n <= 127; n++)
      {
        fout = ((n+1) * fref / (m+1) + (1<<(l-1))) >> l;
        fvco_goal = clock << l;
        fvco = (n+1) * fref / (m+1);
        if (!WITHIN(fvco, 125000, fmax))
          continue;
        if (!WITHIN(fvco, 995*fvco_goal/1000, 1005*fvco_goal/1000))
          continue;
        
        c=1000*(380*7)/(380-175);
        f = (c+ 500 - 34*fvco/1000)/1000;
        if (f > 7) f = 7;
        if (f < 0) f = 0;
/*fprintf(stderr,"clock=%i l=%i f=%i m=%i n=%i\n",clock,l,f,m,n);*/
        return (n << 16) | (m << 8) | (l << 2) | (f << 4);
      }
    }
  }
/*fprintf(stderr,"Can't do clock=%i\n",clock);*/
  return 0;
}



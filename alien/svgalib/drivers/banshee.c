/*
3dfx Voodoo Banshee driver 
*/

#include <stdlib.h>
#include <stdio.h>		/* for printf */
#include <string.h>		/* for memset */
#include <unistd.h>
#include "vga.h"
#include "libvga.h"
#include "svgadriv.h"
#include "vgarelvg.h"

/* New style driver interface. */
#include "timing.h"
#include "vgaregs.h"
#include "interfac.h"
#include "vgapci.h"

#define BANSHEEREG_SAVE(i) (VGA_TOTAL_REGS+i)
#define BANSHEE_TOTAL_REGS (VGA_TOTAL_REGS + 2 + 56)

static int banshee_init(int, int, int);
static void banshee_unlock(void);
static void banshee_lock(void);

static int banshee_memory, chiptype;
static int banshee_is_linear, banshee_linear_base, banshee_io_base;

static CardSpecs *cardspecs;

static void banshee_setpage(int page)
{
   page<<=1;
   port_outl_r(banshee_io_base+0x2c,(port_inl(banshee_io_base+0x2c)&0xfff00000)|(page)|(page<<10));
}

static int __svgalib_banshee_inlinearmode(void)
{
return banshee_is_linear;
}

/* Fill in chipset specific mode information */

static void banshee_getmodeinfo(int mode, vga_modeinfo *modeinfo)
{

    if(modeinfo->colors==16)return;

    modeinfo->maxpixels = banshee_memory*1024/modeinfo->bytesperpixel;
    modeinfo->maxlogicalwidth = 4088;
    modeinfo->startaddressrange = banshee_memory * 1024 - 1;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;

    if (modeinfo->bytesperpixel >= 1) {
	if(banshee_linear_base)modeinfo->flags |= CAPABLE_LINEAR;
        if (__svgalib_banshee_inlinearmode())
	    modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
    }
}

/* Read and save chipset-specific registers */

typedef struct {
   unsigned int pllCtrl0, pllCtrl1, dacMode, dacAddr,
      		vidProcCfg, vidScreenSize, vgaInit0,
                vgaInit1, vidDesktopStartAddr,vidDesktopOverlayStride,
                hwCurPatAddr, hwCurLoc, hwCurC0, hwCurC1;
} *HWRecPtr;

static int banshee_saveregs(uint8_t regs[])
{ 
  HWRecPtr save;

  banshee_unlock();		/* May be locked again by other programs (e.g. X) */
  
  save=(HWRecPtr)(regs+62);
  
  regs[BANSHEEREG_SAVE(0)]=__svgalib_incrtc(0x1a);
  regs[BANSHEEREG_SAVE(1)]=__svgalib_incrtc(0x1b);
  save->pllCtrl0=port_inl(banshee_io_base+0x40);
  save->pllCtrl1=port_inl(banshee_io_base+0x44);
  save->dacMode=port_inl(banshee_io_base+0x4c);
  save->dacAddr=port_inl(banshee_io_base+0x50);
  save->vidProcCfg=port_inl(banshee_io_base+0x5c);
  save->vidScreenSize=port_inl(banshee_io_base+0x98);
  save->vgaInit0=port_inl(banshee_io_base+0x28);
  save->vgaInit1=port_inl(banshee_io_base+0x2c);
  save->vidDesktopStartAddr=port_inl(banshee_io_base+0xe4);
  save->vidDesktopOverlayStride=port_inl(banshee_io_base+0xe8);
  save->hwCurPatAddr=port_inl(banshee_io_base+0x60);
  save->hwCurLoc=port_inl(banshee_io_base+0x64);
  save->hwCurC0=port_inl(banshee_io_base+0x68);
  save->hwCurC1=port_inl(banshee_io_base+0x6c);
  
  return BANSHEE_TOTAL_REGS - VGA_TOTAL_REGS;
}

/* Set chipset-specific registers */

static void banshee_setregs(const uint8_t regs[], int mode)
{  
    HWRecPtr restore;

    banshee_unlock();		/* May be locked again by other programs (eg. X) */
  
    restore=(HWRecPtr)(regs+62);
  
    __svgalib_outcrtc(0x1a,regs[BANSHEEREG_SAVE(0)]);
    __svgalib_outcrtc(0x1b,regs[BANSHEEREG_SAVE(1)]);
    port_outl_r(banshee_io_base+0x40,restore->pllCtrl0);
    port_outl_r(banshee_io_base+0x44,restore->pllCtrl1);
    port_outl_r(banshee_io_base+0x4c,restore->dacMode);
    port_outl_r(banshee_io_base+0x50,restore->dacAddr);
    port_outl_r(banshee_io_base+0x5c,restore->vidProcCfg);
    port_outl_r(banshee_io_base+0x98,restore->vidScreenSize);
    port_outl_r(banshee_io_base+0x28,restore->vgaInit0);
    port_outl_r(banshee_io_base+0x2c,restore->vgaInit1);
    port_outl_r(banshee_io_base+0xe4,restore->vidDesktopStartAddr);
    port_outl_r(banshee_io_base+0xe8,restore->vidDesktopOverlayStride);
    port_outl_r(banshee_io_base+0x60,restore->hwCurPatAddr);
    port_outl_r(banshee_io_base+0x64,restore->hwCurLoc);
    port_outl_r(banshee_io_base+0x68,restore->hwCurC0);
    port_outl_r(banshee_io_base+0x6c,restore->hwCurC1);
    port_outl_r(banshee_io_base+0x5c,restore->vidProcCfg&0xfffffffe);
    port_outl_r(banshee_io_base+0x5c,restore->vidProcCfg|1);
    port_outl_r(banshee_io_base+0x5c,restore->vidProcCfg);

}


/* Return nonzero if mode is available */

static int banshee_modeavailable(int mode)
{
    struct vgainfo *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;


    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];
    if (banshee_memory * 1024 < info->ydim * info->xbytes)
	return 0;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    if((modeinfo->bitsPerPixel==16)&&(modeinfo->greenWeight==5)) {
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

static int CalcPLL(int freq, int isBanshee);
/* Set a mode */

/* Local, called by banshee_setmode(). */

static void banshee_initializemode(uint8_t *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{ /* long k; */

    int vd,vt,vbs,vbe,ht,hd,hss,hse;

    HWRecPtr banshee_regs;

    banshee_regs=(HWRecPtr)(moderegs+62);
   
    banshee_saveregs(moderegs);

    if(modetiming->pixelClock > 135000) {
        modetiming->CrtcHDisplay>>=1;
        modetiming->CrtcHTotal>>=1;
        modetiming->CrtcHSyncStart>>=1;
        modetiming->CrtcHSyncEnd>>=1;
    }

    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

    hd  = (modetiming->CrtcHDisplay>>3)-1;
    hss = (modetiming->CrtcHSyncStart>>3);
    hse = (modetiming->CrtcHSyncEnd>>3);
    ht  = (modetiming->CrtcHTotal>>3)-5;
    moderegs[BANSHEEREG_SAVE(0)]=((ht&0x100)>>8) |
       			    ((hd&0x100)>>6) |
                            ((hd&0x100)>>4) |
                            ((ht&0x40)>>1) |
                            ((hss&0x100)>>2) |
                            ((hse&0x20)<<2) ; 


    vd   = modetiming->CrtcVDisplay - 1;
    vt   = modetiming->CrtcVTotal - 2;
    vbs  = vd;
    vbe  = vt; 
    moderegs[BANSHEEREG_SAVE(1)]=((vt & 0x400)>>10) | 
		            ((vd  & 0x400)>>8) |
		            ((vbs & 0x400)>>6) |
		            ((vbe & 0x400)>>4);

    banshee_regs->vidProcCfg&=0xf7e30000; /* disable HW cursor, scalling etc. */
    banshee_regs->vidProcCfg|=0x00000c81; /* bypass CLUT, disable VGA mode */
    banshee_regs->vidProcCfg |= 0x02; /* cursor in X11 mode */

    if(modeinfo->bitsPerPixel==8){
        banshee_regs->vidProcCfg &= ~0xc00; /* disable CLUT bypass */
    };

    if (modetiming->flags & INTERLACED) {
        banshee_regs->vidProcCfg |= 0x8;
    }

    if (modetiming->flags & DOUBLESCAN) {
        banshee_regs->vidProcCfg |= 0x10;
        banshee_regs->vidScreenSize=modeinfo->width|(modeinfo->height<<13);
    } else {
        banshee_regs->vidScreenSize=modeinfo->width|(modeinfo->height<<12);
    }
    
    switch (modeinfo->bitsPerPixel)
    {
	    case 8: 
               	    banshee_regs->vidProcCfg|=0<<18;
		    break;
	    case 15: 
	    case 16:if(modeinfo->greenWeight==5){
                        banshee_regs->vidProcCfg|=1<<18;
                    } else banshee_regs->vidProcCfg|=1<<18;
		    break;
	    case 24: 
               	    banshee_regs->vidProcCfg|=2<<18;
		    break;
	    case 32: 
               	    banshee_regs->vidProcCfg|=3<<18;
		    break;
	    default: 
		    break;
    }
    
    if(modetiming->pixelClock <= 135000) {
        banshee_regs->dacMode &= ~1;
        banshee_regs->vidProcCfg &= ~(1<<26);
    } else {
        banshee_regs->dacMode |= 1;
        banshee_regs->vidProcCfg |= 1<<26;
    }
    
    banshee_regs->pllCtrl0=CalcPLL(modetiming->pixelClock, 0);
    moderegs[VGA_MISCOUTPUT]|=0x0c; /* pixel clock = pllCtrl0 */

    banshee_regs->vidDesktopStartAddr=0;
    banshee_regs->vidDesktopOverlayStride=modeinfo->lineWidth;    

    banshee_regs->vgaInit0=0x1140;
    banshee_regs->vgaInit1=0x00100000;

    if(modetiming->pixelClock > 135000) {
        modetiming->CrtcHDisplay<<=1;
        modetiming->CrtcHTotal<<=1;
        modetiming->CrtcHSyncStart<<=1;
        modetiming->CrtcHSyncEnd<<=1;
    }

    banshee_is_linear=0;

return ;
}

static int banshee_setmode(int mode, int prv_mode)
{
    uint8_t *moderegs;
    ModeTiming modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {
        banshee_unlock();
        __svgalib_outcrtc(0x1a,0x00);
        __svgalib_outcrtc(0x1b,0x00);
        port_outl_r(banshee_io_base+0x28, port_inl(banshee_io_base+0x28) & ~0x1245 );
        port_outl_r(banshee_io_base+0x2c, port_inl(banshee_io_base+0x2c) & ~0x100000 );
        port_outl_r(banshee_io_base+0x5c, port_inl(banshee_io_base+0x5c) & ~1 ); /* VGA */
	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    }
    if (!banshee_modeavailable(mode))
	return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    if (__svgalib_getmodetiming(&modetiming, modeinfo, cardspecs)) {
	free(modeinfo);
	return 1;
    }

    moderegs = malloc(BANSHEE_TOTAL_REGS);

    banshee_initializemode(moderegs, &modetiming, modeinfo, mode);

    __svgalib_setregs(moderegs);	/* Set standard regs. */
    banshee_setregs(moderegs, mode);		/* Set extended regs. */
    free(moderegs);

    free(modeinfo);

    return 0;
}

/* Unlock chipset-specific registers */

static void banshee_unlock(void)
{
    __svgalib_outcrtc(0x11,__svgalib_incrtc(0x11)&0x7f);    
    port_outl_r(banshee_io_base+0x28,(port_inl(banshee_io_base+0x28)&0xffffffbf)|(1<<6));
}

static void banshee_lock(void)
{
    port_outl_r(banshee_io_base+0x28,(port_inl(banshee_io_base+0x28)&0xffffffbf));
}


/* Indentify chipset, initialize and return non-zero if detected */

static int banshee_test(void)
{
   int found;
   unsigned long buf[64];

   found=(!__svgalib_pci_find_vendor_vga(0x121a,buf,0))&&
          (((buf[0]>>16)==0x0003)||
          ((buf[0]>>16)==0x0009)||
          ((buf[0]>>16)==0x0005));
    
   if(found)banshee_init(0,0,0); 
   return found;
}

/* Set display start address (not for 16 color modes) */
/* Cirrus supports any address in video memory (up to 2Mb) */

static void banshee_setdisplaystart(int address)
{ 
  __svgalib_outcrtc(0x0c,((address>>2) & 0x00FF00)>>8);
  __svgalib_outcrtc(0x0d,(address>>2) & 0x00FF);
  port_outl_r(banshee_io_base+0xe4,address);
}


/* Set logical scanline length (usually multiple of 8) */

static void banshee_setlogicalwidth(int width)
{   
    int offset = width >> 3;
 
    __svgalib_outcrtc(0x13,offset&0xff);
    port_outl_r(banshee_io_base+0xe8,width);
}

static int banshee_linear(int op, int param)
{
    if (op==LINEAR_DISABLE) { banshee_is_linear=0; return 0;}
    if (op==LINEAR_ENABLE)  { banshee_is_linear=1; return 0;}
    if (op==LINEAR_QUERY_BASE) return banshee_linear_base;
    if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY) return 0;		/* No granularity or range. */
        else return -1;		/* Unknown function. */
}

static int banshee_match_programmable_clock(int clock)
{
return clock ;
}

static int banshee_map_clock(int bpp, int clock)
{
return clock ;
}

static int banshee_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
return htiming;
}

static unsigned int cur_colors[16*2];

static int banshee_cursor( int cmd, int p1, int p2, int p3, int p4, void *p5) {
    int i, j;
    unsigned long *b3;
    unsigned long l1, l2;
    
    switch(cmd){
        case CURSOR_INIT:
            return 1;
        case CURSOR_HIDE:
            port_outl_r(banshee_io_base+0x5c,port_inl(banshee_io_base+0x5c)&~(1<<27));
            break;
        case CURSOR_SHOW:
            port_outl_r(banshee_io_base+0x5c,port_inl(banshee_io_base+0x5c)|(1<<27));
            break;
        case CURSOR_POSITION:
            port_outl_r(banshee_io_base+0x64,((p2+64)<<16)|(p1+64));
            break;
        case CURSOR_SELECT:
            i=banshee_memory*1024-(p1+1)*4096;
            port_outl_r(banshee_io_base+0x68,cur_colors[p1*2]);
            port_outl_r(banshee_io_base+0x6c,cur_colors[p1*2+1]);
            port_outl_r(banshee_io_base+0x60,i);
            break;
        case CURSOR_IMAGE:
            i=banshee_memory*1024-(p1+1)*4096;
            b3=(unsigned long *)p5;
            switch(p2) {
                case 0:
                    cur_colors[p1*2]=p3;
                    cur_colors[p1*2+1]=p4;
                    for(j=0;j<32;j++) {
                        l2=*(b3+j);
                        l1=*(b3+32+j);
                        /*change endianess */
                        l1=(l1<<24)|(l1>>24)|((l1>>8)&0xff00)|((l1<<8)&0xff0000);
                        l2=(l2<<24)|(l2>>24)|((l2>>8)&0xff00)|((l2<<8)&0xff0000);
                        *(unsigned long *)(LINEAR_POINTER+i+16*j)=l1;
                        *(unsigned long *)(LINEAR_POINTER+i+16*j+4)=0;
                        *(unsigned long *)(LINEAR_POINTER+i+16*j+8)=l2;
                        *(unsigned long *)(LINEAR_POINTER+i+16*j+12)=0;
                    }
                    for(j=32;j<64;j++) {
                        *(unsigned long *)(LINEAR_POINTER+i+16*j)=0;
                        *(unsigned long *)(LINEAR_POINTER+i+16*j+4)=0;
                        *(unsigned long *)(LINEAR_POINTER+i+16*j+8)=0;
                        *(unsigned long *)(LINEAR_POINTER+i+16*j+12)=0;
                    }
                break;
            }
            break;
    }
    return 0;
}       

/* Function table (exported) */

DriverSpecs __svgalib_banshee_driverspecs =
{
    banshee_saveregs,
    banshee_setregs,
    banshee_unlock,
    banshee_lock,
    banshee_test,
    banshee_init,
    banshee_setpage,
    NULL,
    NULL,
    banshee_setmode,
    banshee_modeavailable,
    banshee_setdisplaystart,
    banshee_setlogicalwidth,
    banshee_getmodeinfo,
    0,				/* old blit funcs */
    0,
    0,
    0,
    0,
    0,				/* ext_set */
    0,				/* accel */
    banshee_linear,
    0,				/* accelspecs, filled in during init. */
    NULL,                       /* Emulation */
    banshee_cursor
};

/* Initialize chipset (called after detection) */

static int banshee_init(int force, int par1, int par2)
{
    unsigned long buf[64];
    int found=0;

    if (force) {
	banshee_memory = par1;
        chiptype = par2;
    } else {

    };
    
    found=(!__svgalib_pci_find_vendor_vga(0x121a,buf,0))&&
            (((buf[0]>>16)==0x0003)||
            ((buf[0]>>16)==0x0009)||
            ((buf[0]>>16)==0x0005));

    chiptype = buf[0]>>16;
    
    if (found){
       banshee_linear_base=buf[5]&0xffffff00;
       banshee_io_base=buf[6]&0xff00;
       __svgalib_io_reloc=banshee_io_base-0x300;
       __svgalib_rel_io_mapio();
    };

    if(banshee_memory==0) {
       unsigned int draminit0,draminit1;
       
       draminit0=port_inl(banshee_io_base+0x18);
       draminit1=port_inl(banshee_io_base+0x1c);
       if(chiptype == 9) {
           banshee_memory = 1024*
                            4*(1+((draminit0>>26)&1))*   	   /* chips */
	                    (1<<((draminit0 & 0x38000000) >> 28))* /* psize */
	                    2*(1+((draminit0>>30)&1)); 		   /* banks */
       } else {
           if(draminit1&0x40000000) {
              /* SDRAM */
              banshee_memory=16*1024;
           } else {
              /* SGRAM */
              banshee_memory=1024*4*
                 (1+((draminit0>>27)&1))* /* SGRAM type - 8MBIT or 16MBIT */
                 (1+((draminit0>>26)&1)); /* Number of sgram chips (4 or 8) */
           }
       }
    }
    
    banshee_unlock();
    
    if (__svgalib_driver_report) {
	fprintf(stderr,"Using Banshee / Voodoo3 driver, %iKB.\n",banshee_memory);
    }
    
	__svgalib_modeinfo_linearset |= IS_LINEAR;
	
    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = banshee_memory;
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->maxPixelClock8bpp = 270000;
    cardspecs->maxPixelClock16bpp = 270000;
    cardspecs->maxPixelClock24bpp = 270000;
    cardspecs->maxPixelClock32bpp = 270000;
    cardspecs->flags = CLOCK_PROGRAMMABLE | ((chiptype==0x03)?NO_INTERLACE:0);
    cardspecs->maxHorizontalCrtc = 4088;
    cardspecs->nClocks =0;
    cardspecs->mapClock = banshee_map_clock;
    cardspecs->mapHorizontalCrtc = banshee_map_horizontal_crtc;
    cardspecs->matchProgrammableClock=banshee_match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_banshee_driverspecs;
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=banshee_linear_base;
    __svgalib_linear_mem_size=banshee_memory*0x400;
    return 0;
}

#define REFFREQ 14318.18

static int
CalcPLL(int freq, int isBanshee) {
  int m, n, k, best_m, best_n, best_k, f_cur, best_error;
  int minm, maxm;

  best_error=freq;
  best_n=best_m=best_k=0;
  if (isBanshee) {
    minm=24;
    maxm=25;
  } else {
    minm=1;
    maxm=57; /* This used to be 64, alas it seems the last 8 (funny that ?)
              * values cause jittering at lower resolutions. I've not done
              * any calculations to what the adjustment affects clock ranges,
              * but I can still run at 1600x1200@75Hz */
  }
  for (n=1; n<256; n++) {
    f_cur=REFFREQ*(n+2);
    if (f_cur<freq) {
      f_cur=f_cur/3;
      if (freq-f_cur<best_error) {
	best_error=freq-f_cur;
	best_n=n;
	best_m=1;
	best_k=0;
	continue;
      }
    }
    for (m=minm; m<maxm; m++) {
      for (k=0; k<4; k++) {
	f_cur=REFFREQ*(n+2)/(m+2)/(1<<k);
	if (abs(f_cur-freq)<best_error) {
	  best_error=abs(f_cur-freq);
	  best_n=n;
	  best_m=m;
	  best_k=k;
	}
      }
    }
  }
  n=best_n;
  m=best_m;
  k=best_k;
  return (n<<8)|(m<<2)|k;
}


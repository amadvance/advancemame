/*
MX 86251 driver

chipset MX 0 1 # 86251 
chipset MX 0 0 # 86250 - not tested.

Unless I make sure I lock after every unlock, after the first
run, et4000 autodetects.
This slows down the bank switching and setting display start.
Fixes should be made either to vga.c (to run chipset_lock 
when entering text mode) or to the et4000 autodetection.
 
*/

#include <stdlib.h>
#include <stdio.h>		/* for printf */
#include <string.h>		/* for memset */
#include <unistd.h>
#include "vga.h"
#include "libvga.h"
#include "svgadriv.h"


/* New style driver interface. */
#include "timing.h"
#include "vgaregs.h"
#include "interfac.h"
#include "vgapci.h"

#define MXREG_SAVE(i) (VGA_TOTAL_REGS+i)
#define MX_TOTAL_REGS (VGA_TOTAL_REGS + 36)

static struct {
    unsigned char c8;
    unsigned short c15;
    unsigned short c16;
    unsigned int c32;
} cursor_colors[16*2];
static unsigned char *cursors[16];

static int mx_init(int, int, int);
static void mx_unlock(void);
static void mx_lock(void);

static int mx_memory,mx_chiptype;
static int mx_is_linear, mx_linear_base;
static int SysControl, BankIdx;

static CardSpecs *cardspecs;

enum {
   	MX86250 = 0, MX86251
};

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

static void mx_setpage(int page)
{
  __svgalib_outCR(BankIdx,page);
}

static int __svgalib_mx_inlinearmode(void)
{
return mx_is_linear;
}

/* Fill in chipset specific mode information */

static void mx_getmodeinfo(int mode, vga_modeinfo *modeinfo)
{

    if(modeinfo->colors==16)return;

	modeinfo->maxpixels = mx_memory*1024/modeinfo->bytesperpixel;
    modeinfo->maxlogicalwidth = 4088;
	modeinfo->startaddressrange = mx_memory * 1024 - 1;
	modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;

    if (modeinfo->bytesperpixel >= 1) {
	if(mx_linear_base)modeinfo->flags |= CAPABLE_LINEAR;
        if (__svgalib_mx_inlinearmode())
	    modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
    }
}

/* Read and save chipset-specific registers */

static int mx_saveregs(unsigned char regs[])
{ 
  int i;

    mx_unlock();		/* May be locked again by other programs (e.g. X) */
    regs[MXREG_SAVE(0)] = __svgalib_inCR(0x21);
    regs[MXREG_SAVE(1)] = __svgalib_inCR(0x22);
    regs[MXREG_SAVE(2)] = __svgalib_inCR(0x23);
    regs[MXREG_SAVE(3)] = __svgalib_inCR(0x24);
    regs[MXREG_SAVE(4)] = __svgalib_inSR(0x18);
    regs[MXREG_SAVE(5)] = __svgalib_inSR(0x19);
    regs[MXREG_SAVE(6)] = __svgalib_inSR(0x1D);
    regs[MXREG_SAVE(7)] = __svgalib_inCR(SysControl);
    regs[MXREG_SAVE(21)] = __svgalib_inSR(0x11);
    regs[MXREG_SAVE(22)] = __svgalib_inCR(0x66);

    switch(mx_chiptype){
      case MX86250:
        for(i=0x33;i<0x3d;i++)regs[MXREG_SAVE(i-0x33+8)]=__svgalib_inCR(i);
      break;
      case MX86251:
        regs[MXREG_SAVE(8)]=__svgalib_inCR(0x1f);
        regs[MXREG_SAVE(9)]=__svgalib_inCR(0x20);
      break;
    }

    for(i=0x60;i<0x6d;i++) if(i!=0x66)
        regs[MXREG_SAVE(i-0x60+23)]=__svgalib_inCR(i);
    
    return MX_TOTAL_REGS - VGA_TOTAL_REGS;
}

/* Set chipset-specific registers */

static void mx_setregs(const unsigned char regs[], int mode)
{  
    int i;

    mx_unlock();		/* May be locked again by other programs (eg. X) */

    __svgalib_outSR(0,1);
    __svgalib_outCR(0x21,regs[MXREG_SAVE(0)]);
    __svgalib_outCR(0x22,regs[MXREG_SAVE(1)]);
    __svgalib_outCR(0x23,regs[MXREG_SAVE(2)]);
    __svgalib_outCR(0x24,regs[MXREG_SAVE(3)]);
    __svgalib_outCR(SysControl,regs[MXREG_SAVE(7)]);
    __svgalib_outSR(0x1D,regs[MXREG_SAVE(6)]);
    __svgalib_outSR(0x18,regs[MXREG_SAVE(4)]);
    __svgalib_outSR(0x19,regs[MXREG_SAVE(5)]);
    __svgalib_outSR(0x11,regs[MXREG_SAVE(21)]);
    __svgalib_outCR(0x66,regs[MXREG_SAVE(22)]);
    __svgalib_outSR(0x1c,0x10);
    __svgalib_outSR(0x1c,0x00);

    switch(mx_chiptype){
      case MX86250:
        for(i=0x33;i<0x3d;i++)__svgalib_outCR(i,regs[MXREG_SAVE(i-0x33+8)]);
      break;
      case MX86251:
        for(i=0x1f;i<0x21;i++)__svgalib_outCR(i,regs[MXREG_SAVE(i-0x1f+8)]);
      break;
    }

    for(i=0x60;i<0x6d;i++) if(i!=0x66)
        __svgalib_outCR(i,regs[MXREG_SAVE(i-0x60+23)]);

    __svgalib_outSR(0,3);
}


/* Return nonzero if mode is available */

static int mx_modeavailable(int mode)
{
    struct vgainfo *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];
    if (mx_memory * 1024 < info->ydim * info->xbytes)
	return 0;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

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

/* Local, called by mx_setmode(). */

static void mx_initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{ /* long k; */
    int tmp, tmptot, tmpss, tmpse, tmpbs, tmpbe, k;
    int offset;
   
    mx_saveregs(moderegs);
    moderegs[MXREG_SAVE(22)]&=0xfe;		/* disable hardware cursor */
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

    offset = modeinfo->lineWidth >> 3;
    moderegs[0x13] = offset&0xff;
    moderegs[0x14]=0x40;
    moderegs[0x17]=0xA3;

    moderegs[MXREG_SAVE(0)] = 0x00;
    tmp    = (modetiming->CrtcHDisplay >> 3) - 1;
    tmptot = (modetiming->CrtcHTotal >> 3) - 5;
    tmpss  = (modetiming->CrtcHSyncStart >> 3);
    tmpse  = (modetiming->CrtcHSyncEnd >> 3);
    tmpbs  = (modetiming->CrtcHSyncStart >> 3) - 1;
    tmpbe  = (modetiming->CrtcHSyncEnd >> 3);
    moderegs[MXREG_SAVE(1)]=((tmptot & 0x100)>>8) | 
       			    ((tmp & 0x100)>>7) |
                            ((tmpss & 0x100)>>6) |
                            ((tmpbs & 0x100)>>5) |
                            ((tmpse & 0x20)>>1) |
                            ((tmpbe & 0x40));
    tmp    = modetiming->CrtcVDisplay - 1;
    tmptot = modetiming->CrtcVTotal - 2;
    tmpss  = modetiming->CrtcVSyncStart;
    tmpse  = modetiming->CrtcVSyncEnd;
    tmpbs  = modetiming->CrtcVSyncStart - 1;
    tmpbe  = offset; 	/* borrow for offset */
    moderegs[MXREG_SAVE(2)]=((tmptot & 0x400)>>10) | 
		            ((tmp & 0x400)>>9) |
		            ((tmpss & 0x400)>>8) |
		            ((tmpbs & 0x400)>>7) |
		            ((tmpse & 0x10)<<1) |
		            ((tmpbe & 0x300)>>2);
    moderegs[MXREG_SAVE(3)] = 0x60 ;
    if (modetiming->flags & INTERLACED)
	    moderegs[MXREG_SAVE(3)] |= 0x8;
    switch (modeinfo->bitsPerPixel)
    {
	    case 8: 
		    moderegs[MXREG_SAVE(3)] |= 0x3;
                    moderegs[MXREG_SAVE(21)]=0x0;
		    break;
	    case 15: 
	    case 16: if(modeinfo->greenWeight==5){
                         moderegs[MXREG_SAVE(21)]=0x20;
                         moderegs[MXREG_SAVE(3)] |= 0x7;}
                     else { 
                         moderegs[MXREG_SAVE(3)] |= 0x4;
                         moderegs[MXREG_SAVE(21)]=0x40;};
		    break;
	    case 24: 
		    moderegs[MXREG_SAVE(3)] |= 0x5;
                    moderegs[MXREG_SAVE(21)]=0x60;
		    break;
	    case 32: 
		    moderegs[MXREG_SAVE(3)] |= 0x6;
                    moderegs[MXREG_SAVE(21)]=0x60;
		    break;
	    default: 
		    moderegs[MXREG_SAVE(3)] |= 0x3;
       moderegs[MXREG_SAVE(21)]=0x20;
		    break;
    }
    if(modeinfo->bitsPerPixel!=8){
       moderegs[0x28]&=0xbf;
    };
    moderegs[MXREG_SAVE(7)] = 0x77;
    k=comp_lmn(modetiming->pixelClock);
    moderegs[MXREG_SAVE(4)] = k&0xff;
    moderegs[MXREG_SAVE(5)] = k>>8;
    moderegs[MXREG_SAVE(6)] = 0x2f;
    if (mx_chiptype == MX86251)
    {
	    moderegs[MXREG_SAVE(8)] = offset & 0xFF;
	    moderegs[MXREG_SAVE(9)] = (offset >> 8) | 0x50;
    }
    else if (mx_chiptype == MX86250)
    {
            moderegs[MXREG_SAVE(8)] = 0x1f;
	    moderegs[MXREG_SAVE(9)] = 0xf7;
	    moderegs[MXREG_SAVE(10)] = 0x77;
	    moderegs[MXREG_SAVE(11)] = 0x1e;
	    moderegs[MXREG_SAVE(12)] = 0x01;
	    moderegs[MXREG_SAVE(13)] = 0x1e;
	    moderegs[MXREG_SAVE(14)] = 0x5d;
	    moderegs[MXREG_SAVE(15)] = 0x00;

	    tmp = (modeinfo->bitsPerPixel + 7) >> 3;

	    if (mx_memory == 1024)
	    {
		    tmp = (modetiming->CrtcHDisplay * tmp) >> 2;
		    moderegs[MXREG_SAVE(16)] = tmp & 0x00ff;
/*		    moderegs[MXREG_SAVE(17)] = (tmp & 0x0300);*/
	    }
	    else	
	    {
		    tmp = (modetiming->CrtcHDisplay * tmp) >> 3;
		    moderegs[MXREG_SAVE(16)] = tmp & 0x00ff;
/*                    moderegs[MXREG_SAVE(17)] = tmp & 0x0300;*/
	    }
    }

    { int p,q,r;
    
    p=moderegs[1]+((moderegs[MXREG_SAVE(1)]&2)<<7);
    moderegs[2]=p&0xff;
    q=moderegs[4]+((moderegs[MXREG_SAVE(1)]&8)<<5);
    r=moderegs[0]+4;
    moderegs[3]&=0xe0;
    moderegs[3]|=(r&0x1f);
    moderegs[5]&=0x7f;
    moderegs[5]|=(r&0x20)<<2;
    
    p=moderegs[0x12]+((moderegs[0x7]&2)<<7)+((moderegs[7]&0x40)<<3)+((moderegs[MXREG_SAVE(2)]&2)<<9);
    moderegs[0x15]=p&0xff;
    moderegs[0x7]&=0xf7;
    moderegs[0x7]|=(p&0x100)>>5;
    moderegs[0x9]&=0xdf;
    moderegs[0x9]|=(p&0x200)>>4;
    moderegs[0x16]=moderegs[6]+1;
    }
    
    mx_is_linear=0;
    
    return ;

}


static int mx_setmode(int mode, int prv_mode)
{
    unsigned char *moderegs;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {

	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    }
    if (!mx_modeavailable(mode))
	return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 1;
    }

    moderegs = malloc(MX_TOTAL_REGS);

    mx_initializemode(moderegs, modetiming, modeinfo, mode);
    free(modetiming);

    __svgalib_setregs(moderegs);	/* Set standard regs. */
    mx_setregs(moderegs, mode);		/* Set extended regs. */
    free(moderegs);

    free(modeinfo);
    return 0;
}


/* Unlock chipset-specific registers */

static void mx_unlock(void)
{
    int vgaIOBase, temp;

    vgaIOBase = (port_in(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
    port_out_r(vgaIOBase + 4, 0x11);
    temp = port_in(vgaIOBase + 5);
    port_out_r(vgaIOBase + 5, temp & 0x7F);
    
    __svgalib_outCR(0x19, 0x88);
}

static void mx_lock(void)
{
    int vgaIOBase, temp;

    vgaIOBase = (port_in(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
    port_out_r(vgaIOBase + 4, 0x11);
    temp = port_in(vgaIOBase + 5);
    port_out_r(vgaIOBase + 5, temp & 0x7F);
    
    __svgalib_outCR(0x19, 0x0);
}


/* Indentify chipset, initialize and return non-zero if detected */

static int mx_test(void)
{
    int     i;

    i=(__svgalib_inCR(0xf2)<< 8) | __svgalib_inCR(0xf1);
    if((i&0xfff0)!=0x8620)return 0;
    
    mx_chiptype=-1;
    if((i&0xff)==0x25)mx_chiptype=MX86250;
    if((i&0xff)==0x26)mx_chiptype=MX86251;
    if(mx_chiptype>=0){ mx_init(0,0,0); return 1;};
    return 0;
}


/* Set display start address (not for 16 color modes) */
/* Cirrus supports any address in video memory (up to 2Mb) */

static void mx_setdisplaystart(int address)
{ 
  address=address >> 2;
  port_outw_r(CRT_IC, (address & 0x00FF00) | 0x0C);
  port_outw_r(CRT_IC, ((address & 0x00FF) << 8) | 0x0D);
  port_outw_r(CRT_IC, ((address & 0xFF0000) >> 8) | 0x21);

}


/* Set logical scanline length (usually multiple of 8) */
/* Cirrus supports multiples of 8, up to 4088 */

static void mx_setlogicalwidth(int width)
{   int i;
    int offset = width >> 3;
 
    __svgalib_outCR(0x13,offset&0xff);
    i=__svgalib_inCR(0x23);
    i&=0x3f;
    i|=(offset&0x300)>>2;
    __svgalib_outCR(0x23,i);
}

static int mx_linear(int op, int param)
{
if (op==LINEAR_ENABLE){ mx_is_linear=1; return 0;}
if (op==LINEAR_DISABLE){ mx_is_linear=0; return 0;}
if (op==LINEAR_QUERY_BASE) return mx_linear_base;
if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY) return 0;		/* No granularity or range. */
    else return -1;		/* Unknown function. */
}

static int mx_match_programmable_clock(int clock)
{
return clock ;
}

static int mx_map_clock(int bpp, int clock)
{
return clock ;
}

static int mx_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
return htiming;
}

static int mx_cursor( int cmd, int p1, int p2, int p3, int p4, void *p5) {
    unsigned long *b3;
    int i, j, k, l; 
    unsigned int l1, l2;
    unsigned char c;
    
    switch(cmd){
        case CURSOR_INIT:
            for (i=0;i<16;i++)cursors[i]=NULL;
            return 1;
        case CURSOR_HIDE:
            __svgalib_outcrtc(0x66,__svgalib_incrtc(0x66)&0xfe);
            break;
        case CURSOR_SHOW:
            __svgalib_outcrtc(0x66,__svgalib_incrtc(0x66)|1);
            break;
        case CURSOR_POSITION:
            if(CI.colors>65536)p1&= ~1; /* the X server does this */
            __svgalib_outcrtc(0x61,p1>>8);
            __svgalib_outcrtc(0x60,p1&0xff);
            __svgalib_outcrtc(0x63,p2&0xff);
            __svgalib_outcrtc(0x62,0);
            __svgalib_outcrtc(0x65,0);
            __svgalib_outcrtc(0x64,p2>>8);
            break;
        case CURSOR_IMAGE:
            switch(p2){
                case 0:
                    if(cursors[p1]!=NULL) {
                        free(cursors[p1]);
                    }
                    cursors[p1]=malloc(256);
                    memcpy(cursors[p1],p5,256);
                    cursor_colors[p1*2].c8=findcolor(p3);
                    cursor_colors[p1*2].c32=p3;
                    cursor_colors[p1*2].c16=((p3&0xf80000)>>8)|((p3&0xfc00)>>5)|((p3&0xf8)>>3);
                    cursor_colors[p1*2].c15=((p3&0xf80000)>>9)|((p3&0xf800)>>5)|((p3&0xf8)>>3);
                    cursor_colors[p1*2+1].c8=findcolor(p4);
                    cursor_colors[p1*2+1].c32=p4;
                    cursor_colors[p1*2+1].c16=((p4&0xf80000)>>8)|((p4&0xfc00)>>5)|((p4&0xf8)>>3);
                    cursor_colors[p1*2+1].c15=((p4&0xf80000)>>9)|((p4&0xf800)>>5)|((p4&0xf8)>>3);
                    break;
            }
        case CURSOR_SELECT:
            i=mx_memory*1024-256;
            b3=(unsigned long *)cursors[p1];
            switch(p2) {
                case 0:
                    switch(CI.colors) {
                        case 256:
                           __svgalib_outcrtc(0x6a,cursor_colors[p1*2+1].c8);
                           __svgalib_outcrtc(0x67,cursor_colors[p1*2].c8);
                           break;
                        case 32768:
                           __svgalib_outcrtc(0x6a,cursor_colors[p1*2+1].c15>>8);
                           __svgalib_outcrtc(0x6b,cursor_colors[p1*2+1].c15&0xff);
                           __svgalib_outcrtc(0x67,cursor_colors[p1*2].c15>>8);
                           __svgalib_outcrtc(0x68,cursor_colors[p1*2].c15&0xff);
                           break;
                        case 65536:
                           __svgalib_outcrtc(0x6a,cursor_colors[p1*2+1].c16>>8);
                           __svgalib_outcrtc(0x6b,cursor_colors[p1*2+1].c16&0xff);
                           __svgalib_outcrtc(0x67,cursor_colors[p1*2].c16>>8);
                           __svgalib_outcrtc(0x68,cursor_colors[p1*2].c16&0xff);
                           break;
                        case (1<<24):
                           __svgalib_outcrtc(0x6a,cursor_colors[p1*2+1].c32>>16);
                           __svgalib_outcrtc(0x6b,(cursor_colors[p1*2+1].c32>>8)&0xff);
                           __svgalib_outcrtc(0x6c,cursor_colors[p1*2+1].c32&0xff);
                           __svgalib_outcrtc(0x67,cursor_colors[p1*2].c32>>16);
                           __svgalib_outcrtc(0x68,(cursor_colors[p1*2].c32>>8)&0xff);
                           __svgalib_outcrtc(0x69,cursor_colors[p1*2].c32&0xff);
                           break;
                    }
                        
                    for(j=0;j<32;j++) {
                        l1=*(b3+j);
                        l2=~(*(b3+32+j));
                        for(k=0;k<8;k++) {
                            c=0;
                            for(l=0;l<4;l++) {
                                c<<=1;
                                c|=(l2>>31);
                                c<<=1;
                                c|=(l1>>31);
                                if(c&2)c&=0xfe;
                                l1<<=1;
                                l2<<=1;
                            }
                            *(LINEAR_POINTER+i+8*j+k)=c;
                        }
                    }
                    break;
            }
            break;
    }
    return 0;
}       

/* Function table (exported) */

DriverSpecs __svgalib_mx_driverspecs =
{
    mx_saveregs,
    mx_setregs,
    mx_unlock,
    mx_lock,
    mx_test,
    mx_init,
    mx_setpage,
    NULL,
    NULL,
    mx_setmode,
    mx_modeavailable,
    mx_setdisplaystart,
    mx_setlogicalwidth,
    mx_getmodeinfo,
    0,				/* old blit funcs */
    0,
    0,
    0,
    0,
    0,				/* ext_set */
    0,				/* accel */
    mx_linear,
    0,				/* accelspecs, filled in during init. */
    NULL,                       /* Emulation */
    mx_cursor,
};

/* Initialize chipset (called after detection) */

static int mx_init(int force, int par1, int par2)
{
   unsigned long buf[64];
   int found=0;

    mx_unlock();
    if (force) {
	mx_memory = par1;
        mx_chiptype = par2;
    } else {

    };
    if(mx_memory==0)mx_memory=1024<<((__svgalib_inCR(0x57)>>3)& 3);

    found=__svgalib_pci_find_vendor_vga(0x10d9,buf,0);
    mx_linear_base=0;
    if (!found){
       mx_linear_base=buf[4]&0xffffff00;
    };

    if (__svgalib_driver_report) {
	fprintf(stderr,"Using MX driver, %iKB.\n",mx_memory);
        switch(mx_chiptype){
           case 1:
              fprintf(stderr,"82651 chipset.\n");
           break;
           case 0:
              fprintf(stderr,"82650 chipset.\n");
           break;
           default:
              fprintf(stderr,"unknown chipset, using 82650.\n");
           break;
        };
    }
    switch(mx_chiptype){
       case 1:
          SysControl=0x32;
          BankIdx=0x34;
       break;
       case 0:
       default:
          SysControl=0x25;
          BankIdx=0x2e;
       break;
    };
    __svgalib_modeinfo_linearset |= IS_LINEAR;
    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = mx_memory;
    cardspecs->maxPixelClock4bpp = 75000;	
    cardspecs->maxPixelClock8bpp = 160000;	
    cardspecs->maxPixelClock16bpp = 160000;	
    cardspecs->maxPixelClock24bpp = 160000;
    cardspecs->maxPixelClock32bpp = 160000;
    cardspecs->flags = INTERLACE_DIVIDE_VERT | CLOCK_PROGRAMMABLE;
    cardspecs->maxHorizontalCrtc = 4088;
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->nClocks =0;
    cardspecs->mapClock = mx_map_clock;
    cardspecs->mapHorizontalCrtc = mx_map_horizontal_crtc;
    cardspecs->matchProgrammableClock=mx_match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_mx_driverspecs;
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=mx_linear_base;
    __svgalib_linear_mem_size=mx_memory*0x400;
    return 0;
}

#define WITHIN(v,c1,c2) (((v) >= (c1)) && ((v) <= (c2)))

static unsigned
comp_lmn(unsigned clock)
{
  int     n, m, l, f, b;
  double  fvco;
  double  fout;
  double  fmax;
  double  fref;
  double  fvco_goal;

    fmax = 162000.0;

  fref = 28636.5; /* I'm not sure */

  for (m = 1; m <= 30; m++)
  {
    for (l = 2; l >= 0; l--)
    {
      for (n = 6; n <= 127; n++)
      {
        fout = ((double)(n + 1) * fref)/((double)(m + 1) * (1 << l));
        fvco_goal = (double)clock * (double)(1 << l);
        fvco = fout * (double)(1 << l);
        if (!WITHIN(fvco, 0.995*fvco_goal, 1.005*fvco_goal))
          continue;
        if (!WITHIN(fvco, 32000.0, fmax))
          continue;
	f=0;
	if((l==2)||(fvco>100000.0))f=1;		
	b=0;
	if((fvco>150000.0))b=1;		
#if 0
fprintf(stderr,"clock=%i l=%i m=%i n=%i f=%i b=%i\n",clock,n,m,l,f,b);
#endif
   	if(l==2)l=3;
        return (n << 8) | m | (l << 5) | (f<<7) | (b<<15);
      }
    }
  }
fprintf(stderr,"MX driver: Can't do clock=%i\n",clock);
  return 0;
}


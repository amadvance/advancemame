/*
Laguna chipset driver for cirrus logic 5462, 5464, 5465

Only tested on 5464.
Problems:
    mouse cursor does not work.
    secondary (mmio only) not tested.
    fifo threshold needs more tuning.
*/


#include <stdlib.h>
#include <stdio.h>		
#include <string.h>
#include <unistd.h>
#include "vga.h"
#include "libvga.h"
#include "svgadriv.h"
#include "timing.h"
#include "vgaregs.h"
#include "interfac.h"
#include "vgapci.h"

#include "lagunaio.h"

struct lagunahw {
    unsigned char crt[5];
    unsigned char gra[3];
    unsigned char seq[3];
    unsigned char pal_state;
    unsigned char bclk;
    unsigned char tile; 
    unsigned short tiling_control;
    unsigned short control;
    unsigned short format;
    unsigned short cur_x;
    unsigned short cur_y;
    unsigned short cur_preset;
    unsigned short cur_misc;
    unsigned short cur_address;
    unsigned short disp_thresh;
    unsigned int pcifc;
};

#define LAGUNAREG_SAVE(i) (VGA_TOTAL_REGS+i)
#define LAGUNA_TOTAL_REGS (VGA_TOTAL_REGS + sizeof(struct lagunahw))

enum {
    L5462=0, L5464, L5465
};

static int pagemul=4;

static int laguna_init(int, int, int);
static void laguna_unlock(void);
static void laguna_lock(void);

static int laguna_memory, laguna_chiptype;
static int laguna_is_linear, laguna_linear_base, laguna_mmio_base;

static CardSpecs *cardspecs;

static void laguna_setpage(int page)
{
    __svgalib_outgra(0x09,page*pagemul);
}

static int __svgalib_laguna_inlinearmode(void)
{
return laguna_is_linear;
}

/* Fill in chipset specific mode information */

static void laguna_getmodeinfo(int mode, vga_modeinfo *modeinfo)
{

    if(modeinfo->colors==16)return;

    modeinfo->maxpixels = laguna_memory*1024/modeinfo->bytesperpixel;
    modeinfo->maxlogicalwidth = 4088;
    modeinfo->startaddressrange = laguna_memory * 1024 - 1;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;

    if (modeinfo->bytesperpixel >= 1) {
	if(laguna_linear_base)modeinfo->flags |= CAPABLE_LINEAR;
        if (__svgalib_laguna_inlinearmode())
	    modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
    }
}

/* Read and save chipset-specific registers */

static int laguna_saveregs(unsigned char regs[])
{ 
    struct lagunahw *l=(struct lagunahw *)(regs+VGA_TOTAL_REGS);

    laguna_unlock();		
    
    l->crt[0]= __svgalib_incrtc(0x19);
    l->crt[1]= __svgalib_incrtc(0x1a);
    l->crt[2]= __svgalib_incrtc(0x1b);
    l->crt[3]= __svgalib_incrtc(0x1d);
    l->crt[4]= __svgalib_incrtc(0x1e);

    l->gra[0]= __svgalib_ingra(0x09);
    l->gra[1]= __svgalib_ingra(0x0a);
    l->gra[2]= __svgalib_ingra(0x0b);

    l->seq[0]= __svgalib_inseq(0x0e);
    l->seq[1]= __svgalib_inseq(0x1e);
    l->seq[2]= __svgalib_inseq(0x07);

    if(laguna_chiptype==L5465)
        l->tiling_control=laguna_readw(0x2c4);
    l->control=laguna_readw(0x402);
    if(laguna_chiptype==L5465)
        l->bclk=laguna_readb(0x2c0);
        else l->bclk=laguna_readb(0x8c);
    l->tile=laguna_readb(0x407);
    l->pal_state=laguna_readb(0xb0);
    l->format=laguna_readw(0xc0);
    l->cur_x=laguna_readw(0xe0);
    l->cur_y=laguna_readw(0xe2);
    l->cur_preset=laguna_readw(0xe4);
    l->cur_misc=laguna_readw(0xe6);
    l->cur_address=laguna_readw(0xe8);
    l->disp_thresh=laguna_readw(0xea);
    l->pcifc=laguna_readl(0x3fc);
    
    return LAGUNA_TOTAL_REGS - VGA_TOTAL_REGS;
}

/* Set chipset-specific registers */

static void laguna_setregs(const unsigned char regs[], int mode)
{  
    struct lagunahw *l=(struct lagunahw *)(regs+VGA_TOTAL_REGS);

    laguna_unlock();		

    __svgalib_outcrtc(0x19,l->crt[0]);
    __svgalib_outcrtc(0x1a,l->crt[1]);
    __svgalib_outcrtc(0x1b,l->crt[2]);
    __svgalib_outcrtc(0x1d,l->crt[3]);
    __svgalib_outcrtc(0x1e,l->crt[4]);

    __svgalib_outseq(0x07,l->seq[2]);
    __svgalib_outseq(0x0e,l->seq[0]);
    __svgalib_outseq(0x1e,l->seq[1]);

    laguna_writew(0xc0,l->format);

    laguna_writel(0x3fc,l->pcifc);

    laguna_writew(0xea,l->disp_thresh);

    if(laguna_chiptype==L5465)
        laguna_writew(0x2c4,l->tiling_control);

    laguna_writew(0x407,l->tile);

    if(laguna_chiptype==L5465)
        laguna_writeb(0x2c0,l->bclk);
        else laguna_writeb(0x8c,l->bclk);

    laguna_writew(0x402,l->control);

    laguna_writew(0xb0,l->pal_state);
    laguna_writew(0xe0,l->cur_x);
    laguna_writew(0xe2,l->cur_y);
    laguna_writew(0xe4,l->cur_preset);
    laguna_writew(0xe6,l->cur_misc);
    laguna_writew(0xe8,l->cur_address);

    __svgalib_outgra(0x09,l->gra[0]);
    __svgalib_outgra(0x0a,l->gra[1]);
    __svgalib_outgra(0x0b,l->gra[2]);
}


/* Return nonzero if mode is available */

static int laguna_modeavailable(int mode)
{
    struct vgainfo *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];
    if (laguna_memory * 1024 < info->ydim * info->xbytes)
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

/* Local, called by laguna_setmode(). */

#define REF 14318

static int laguna_findclock(int clock, int *on, int *om, int *op, int *opp)
{
    int n, m, diff, best, bestm;
    
    *opp=0;

//    while(clock<20000) {
//        clock*=2;
//        *opp=+1;
//    }
    
    if(clock<40000) {
        clock *= 2;
        *op=1;
    } else *op=0;
    
    best=clock;
    bestm=5;
    for(m=6; m<30;m++) {
        n=clock*m/REF;
        if((n<32)||(n>127)) continue;
        diff=clock-(n*REF/m);
        if(diff<0)diff=-diff;
        if(diff<best) {
            best=diff;
            bestm=m;
        }
    }
    
    if((best==0) || (clock/best>100)) {
        *on=clock*bestm/REF;
        *om=bestm;
        return 0;
    } else {
        fprintf(stderr,"Can't find clock %iKHz.\n",clock);
        return -1;
    }        
}

static void laguna_initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{
    struct lagunahw *l=(struct lagunahw *)(moderegs+VGA_TOTAL_REGS);
    int hd, htot, hss, hse, hbs, hbe;
    int vd, vtot, vss, vse, vbs, vbe;
    int offset;
    int n,m,p,pp;
   
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);
    
    
    if(laguna_chiptype==L5465) {
        l->tiling_control=laguna_readw(0x2c4);
        l->tiling_control &= ~0x80; /* disable tiling */
    }

    l->control=laguna_readw(0x402) & 0x87ff; /* narrow tiles */
    l->control |= 0x1000; /* disable tiling */

    l->disp_thresh=laguna_readw(0xea)&0xffe0;
    l->disp_thresh &= 0x3fbf; /* 1-way interleave and narrow tiles */
    switch(modeinfo->bitsPerPixel) {
        case 8:
        case 15:
        case 16:
            l->disp_thresh |= 0x10;
            break;
        case 24:
        case 32:
            l->disp_thresh |= 0x20;
            break;
        default:
            l->disp_thresh |= 0x18;
    }
            
    l->pcifc=laguna_readl(0x3fc);
    l->pcifc |= 0x10000000;

    if(laguna_chiptype==L5465)
        l->bclk=laguna_readb(0x2c0);
        else l->bclk=laguna_readb(0x8c);

    l->bclk=18;

    l->tile=laguna_readb(0x407) & 0x3f; /* no interleave */

    offset = modeinfo->lineWidth >> 3;
    
    moderegs[0x13]=offset&0xff;
    
    hd    = (modetiming->CrtcHDisplay >> 3) - 1;
    htot = (modetiming->CrtcHTotal >> 3) - 5;
    hss  = (modetiming->CrtcHSyncStart >> 3);
    hse  = (modetiming->CrtcHSyncEnd >> 3);
    hbs  = (modetiming->CrtcHSyncStart >> 3) - 1;
    hbe  = (modetiming->CrtcHSyncEnd >> 3);
    vd    = modetiming->CrtcVDisplay - 1;
    vtot = modetiming->CrtcVTotal - 2;
    vss  = modetiming->CrtcVSyncStart;
    vse  = modetiming->CrtcVSyncEnd;
    vbs  = modetiming->CrtcVSyncStart - 1;
    vbe  = modetiming->CrtcVSyncEnd;

    l->crt[0] = htot>>1;
    l->crt[1] = ((vbe>>2)&0xf0) | /* vertical blank end overflow */
        	2; /* double buffered display start */
    if (modetiming->flags & INTERLACED) l->crt[1] |= 0x1;
    l->crt[2] = 0x22 | /* extended address and no border*/
        	((offset&0x100)>>4);
    l->crt[3]=(offset&0x200)>>9;
    l->crt[4] = ((htot&0x100)>>1) |
        	((hd&0x100)>>2) |
                ((hbs&0x100)>>3) |
                ((hss&0x100)>>4) |
                ((vtot&0x400)>>7) |
        	((vd&0x400)>>8) |
                ((vbs&0x400)>>9) |
                ((vss&0x400)>>10);
    
    l->gra[0]=0;
    l->gra[2]=32; /* use 16KB granularity, only one bank */
    
    if(laguna_findclock(modetiming->pixelClock,&n,&m,&p,&pp)) {
        /* should not happen */
        exit(4);
    }
    l->seq[0]=(m<<1)|p;
    l->seq[1]=n;
    l->seq[2]=1;

    l->pal_state=0;
    l->format=0;
    switch(modeinfo->bitsPerPixel) {
        case 8:
            l->format|=0;
            l->control |= 0; 
            break;
	case 15: 
	case 16: if(modeinfo->greenWeight==5)
            l->format|=0x1600; else
            l->format|=0x1400;
            l->control |= 0x2000;
            break;
        case 24:
            l->format|=0x2400;
            l->control |= 0x4000;
            break;
        case 32:
            l->format|=0x3400;
            l->control |= 0x6000;
            break;
    }
    
    l->cur_misc=0; /* disable cursor */
    l->cur_preset=0;

    moderegs[59] |= 0x0c;

    return;
}


static int laguna_setmode(int mode, int prv_mode)
{
    unsigned char *moderegs;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {
        laguna_writeb(0xe6,0);        
        __svgalib_outseq(0x07,0);
	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    }
    if (!laguna_modeavailable(mode))
	return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 1;
    }

    moderegs = malloc(LAGUNA_TOTAL_REGS);

    laguna_initializemode(moderegs, modetiming, modeinfo, mode);
    free(modetiming);

    __svgalib_setregs(moderegs);	/* Set standard regs. */
    laguna_setregs(moderegs, mode);		/* Set extended regs. */
    free(moderegs);

    free(modeinfo);
    return 0;
}


/* Unlock chipset-specific registers */

static void laguna_unlock(void)
{
    __svgalib_outcrtc(0x11,__svgalib_incrtc(0x11)&0x7f);
}

static void laguna_lock(void)
{
}

#define VENDOR_ID 0x1013

/* Indentify chipset, initialize and return non-zero if detected */

static int laguna_test(void)
{
    int found;
    unsigned long buf[64];
    
    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    
    if(!found&&
        ((((buf[0]>>16)&0xffff)==0x00d0)||
         (((buf[0]>>16)&0xffff)==0x00d4)||
         (((buf[0]>>16)&0xffff)==0x00d6))){
       laguna_init(0,0,0);
       return 1;
    };
    return 0;
}


/* Set display start address (not for 16 color modes) */
static void laguna_setdisplaystart(int address)
{ 
    address=address >> 2;
    __svgalib_outcrtc(0x0c,(address & 0xFF00)>>8);
    __svgalib_outcrtc(0x0d, address & 0xFF);
    __svgalib_outcrtc(0x1b, (__svgalib_incrtc(0x1b)&0xf2) | ((address&0x100)>>8) | ((address&0x600)>>7));
    __svgalib_outcrtc(0x1d, (__svgalib_incrtc(0x1b)&0xe7) | ((address&0x1800)>>8) );
 
}


/* Set logical scanline length (usually multiple of 8) */
static void laguna_setlogicalwidth(int width)
{   
    int offset = width >> 3;
 
    __svgalib_outcrtc(0x13,offset&0xff);
    __svgalib_outcrtc(0x1b, (__svgalib_incrtc(0x1b)&0xef)| ((offset&0x100)>>4));
    __svgalib_outcrtc(0x1d, (__svgalib_incrtc(0x1b)&0xfe)| ((offset&0x200)>>9));
}

static int laguna_linear(int op, int param)
{
    if (op==LINEAR_ENABLE){laguna_is_linear=1; return 0;};
    if (op==LINEAR_DISABLE){laguna_is_linear=0; return 0;};
    if (op==LINEAR_QUERY_BASE) return laguna_linear_base;
    if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY) return 0;		/* No granularity or range. */
        else return -1;		/* Unknown function. */
}

static int laguna_match_programmable_clock(int clock)
{
return clock ;
}

static int laguna_map_clock(int bpp, int clock)
{
return clock ;
}

static int laguna_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
return htiming;
}

/* Function table (exported) */

static unsigned char cur_colors[16*6];

static int laguna_cursor( int cmd, int p1, int p2, int p3, int p4, void *p5) {
    unsigned long *b3;
    int i, j; 
    unsigned int l1, l2;
    
    switch(cmd){
        case CURSOR_INIT:
            return 1;
        case CURSOR_HIDE:
            laguna_writew(0xe6,laguna_readw(0xe6)&0xfffe);
            break;
        case CURSOR_SHOW:
            laguna_writew(0xe6,laguna_readw(0xe6)|1);
            break;
        case CURSOR_POSITION:
            laguna_writew(0xe0,p1);
            laguna_writew(0xe2,p2);
            break;
        case CURSOR_SELECT:
            i=laguna_memory*1024-(16-p1)*1024;
            laguna_writew(0xb0,laguna_readw(0xb0)|8);
            laguna_writeb(0xa8,15);
            laguna_writeb(0xac,cur_colors[p1*6]>>2);
            laguna_writeb(0xac,cur_colors[p1*6+1]>>2);
            laguna_writeb(0xac,cur_colors[p1*6+2]>>2);
            laguna_writeb(0xa8,0);
            laguna_writeb(0xac,cur_colors[p1*6+3]>>2);
            laguna_writeb(0xac,cur_colors[p1*6+4]>>2);
            laguna_writeb(0xac,cur_colors[p1*6+5]>>2);
            laguna_writew(0xb0,laguna_readw(0xb0)&~8);            
            laguna_writew(0xe8,i>>8);
            break;
        case CURSOR_IMAGE:
            i=laguna_memory*1024-(16-p1)*1024;
            b3=(unsigned long *)p5;
            switch(p2) {
                case 0:
                    cur_colors[p1*6]=(p3&0xff0000)>>16;
                    cur_colors[p1*6+1]=(p3&0xff00)>>8;
                    cur_colors[p1*6+2]=p3&0xff;
                    cur_colors[p1*6+3]=(p4&0xff0000)>>16;
                    cur_colors[p1*6+4]=(p4&0xff00)>>8;
                    cur_colors[p1*6+5]=p4&0xff;
                    for(j=0;j<32;j++) {
                        l1=*(b3+j);
                        l2=*(b3+32+j);
                        *(unsigned long *)(LINEAR_POINTER+i+j*16)=l1&l2;
                        *(unsigned long *)(LINEAR_POINTER+i+j*16+4)=0;
                        *(unsigned long *)(LINEAR_POINTER+i+j*16+8)=l1;
                        *(unsigned long *)(LINEAR_POINTER+i+j*16+12)=0;
                    }
                    memset(LINEAR_POINTER+i+512,0,512);
                    break;
            }
            break;
    }
    return 0;
}       

DriverSpecs __svgalib_laguna_driverspecs =
{
    laguna_saveregs,
    laguna_setregs,
    laguna_unlock,
    laguna_lock,
    laguna_test,
    laguna_init,
    laguna_setpage,
    NULL,
    NULL,
    laguna_setmode,
    laguna_modeavailable,
    laguna_setdisplaystart,
    laguna_setlogicalwidth,
    laguna_getmodeinfo,
    0,				/* old blit funcs */
    0,
    0,
    0,
    0,
    0,				/* ext_set */
    0,				/* accel */
    laguna_linear,
    0,				/* accelspecs, filled in during init. */
    NULL,                       /* Emulation */
    laguna_cursor,
};

/* Initialize chipset (called after detection) */

static int laguna_init(int force, int par1, int par2)
{
    unsigned long buf[64];
    int found=0;
    char *chipnames[]={"5462", "5464", "5465"};
    
    laguna_unlock();
    if (force) {
	laguna_memory = par1;
        laguna_chiptype = par2;
    } else {

    };

    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    laguna_linear_base=0;

    if (!found){
       int i,j;
       switch(buf[0]>>16) {
           case 0xd0:
               laguna_chiptype=L5462;
               break;
           case 0xd4:
               laguna_chiptype=L5464;
               break;
           case 0xd6:
               laguna_chiptype=L5465;
               break;
       }
       if(laguna_chiptype==L5465) {
		   laguna_mmio_base=buf[5]&0xffffff00;
		   laguna_linear_base=buf[4]&0xffffff00;
	   } else {
		   laguna_mmio_base=buf[4]&0xffffff00;
		   laguna_linear_base=buf[5]&0xffffff00;
	   }
       if(__svgalib_secondary)laguna_mapio();

       __svgalib_mmio_base=laguna_mmio_base;
       __svgalib_mmio_size=4096;
       map_mmio();

       i=*(MMIO_POINTER + 0x200);
       j=*(MMIO_POINTER + 0x201);
    
       laguna_memory = 512*(1<<((j&0xc0)>>6))*((i&7)+1);
    } else return -1;

    if(laguna_memory<4096) pagemul<<=2;

    if (__svgalib_driver_report) {
	fprintf(stderr,"Using LAGUNA driver, %s with %iKB video ram found.\n",
            	chipnames[laguna_chiptype],laguna_memory);
    };

    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = laguna_memory;
    cardspecs->maxPixelClock4bpp = 170000;	
    switch(laguna_chiptype) {
        case L5462:
            cardspecs->maxPixelClock8bpp = 170000;	
            cardspecs->maxPixelClock16bpp = 135100;	
            cardspecs->maxPixelClock24bpp = 135100;
            cardspecs->maxPixelClock32bpp = 85500;
        case L5464:
            cardspecs->maxPixelClock8bpp = 230000;	
            cardspecs->maxPixelClock16bpp = 170000;	
            cardspecs->maxPixelClock24bpp = 170000;
            cardspecs->maxPixelClock32bpp = 135100;
        case L5465:
            cardspecs->maxPixelClock8bpp = 250000;	
            cardspecs->maxPixelClock16bpp = 170000;	
            cardspecs->maxPixelClock24bpp = 170000;
            cardspecs->maxPixelClock32bpp = 135100;
    }
    __svgalib_modeinfo_linearset |= IS_LINEAR;
	cardspecs->flags = INTERLACE_DIVIDE_VERT | CLOCK_PROGRAMMABLE;
    cardspecs->maxHorizontalCrtc = 4088;
    cardspecs->nClocks =0;
    cardspecs->mapClock = laguna_map_clock;
    cardspecs->mapHorizontalCrtc = laguna_map_horizontal_crtc;
    cardspecs->matchProgrammableClock=laguna_match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_laguna_driverspecs;
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=laguna_linear_base;
    __svgalib_linear_mem_size=laguna_memory*0x400;
    return 0;
}

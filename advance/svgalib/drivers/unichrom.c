/*
VIA Unichrome driver 
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

static int unichrome_init(int, int, int);
static void unichrome_unlock(void);
static void unichrome_lock(void);

static int unichrome_memory;
static int unichrome_is_linear, unichrome_linear_base, unichrome_io_base;

static CardSpecs *cardspecs;
typedef struct {
	unsigned char seq[0x50];
    unsigned char crt[0xc0];
} UNIRegRec, *UNIRegPtr;

enum { CLE266=1, CLE266CX, KM400, KM800, PM800} chiptype;

//static void unichrome_setpage(int page) {}

static int __svgalib_unichrome_inlinearmode(void)
{
return unichrome_is_linear;
}

/* Fill in chipset specific mode information */

static void unichrome_getmodeinfo(int mode, vga_modeinfo *modeinfo)
{

    if(modeinfo->colors==16)return;

    modeinfo->maxpixels = unichrome_memory*1024/modeinfo->bytesperpixel;
    modeinfo->maxlogicalwidth = 4088;
    modeinfo->startaddressrange = unichrome_memory * 1024 - 1;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;

    if (modeinfo->bytesperpixel >= 1) {
	if(unichrome_linear_base)modeinfo->flags |= CAPABLE_LINEAR;
        if (__svgalib_unichrome_inlinearmode())
	    modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
    }
}

static int unichrome_saveregs(uint8_t regs[])
{ 
  UNIRegPtr save;
  int i;

  unichrome_unlock();		/* May be locked again by other programs (e.g. X) */
  
  save=(UNIRegPtr)(regs+60);

  for(i=0x10; i<0x50; i++)save->seq[i]=__svgalib_inseq(i);
  for(i=0x18; i<0xc0; i++)save->crt[i]=__svgalib_incrtc(i);
  
 
  return sizeof(UNIRegRec);
}

/* Set chipset-specific registers */
void ViaSeqMask(int index, int value, int mask) {
	int tmp;
	
	tmp = __svgalib_inseq(index);
	tmp &= ~mask;
	tmp |= (value & mask);
	
	__svgalib_outseq(index, tmp);
}

static void unichrome_setregs(const uint8_t regs[], int mode)
{  
	UNIRegPtr restore;
	int i;

	unichrome_unlock();		/* May be locked again by other programs (eg. X) */
  
	restore=(UNIRegPtr)(regs+60);

	__svgalib_outcrtc(0x6a, 0);
	__svgalib_outcrtc(0x6b, 0);
	__svgalib_outcrtc(0x6c, 0);
  
	for(i=0x14; i<=0x1f;i++) __svgalib_outseq(i, restore->seq[i]);
	for(i=0x22; i<=0x2b;i++) __svgalib_outseq(i, restore->seq[i]);
	__svgalib_outseq(0x2e, restore->seq[0x2e]);
	for(i=0x44; i<=0x47;i++) __svgalib_outseq(i, restore->seq[i]);

	ViaSeqMask( 0x40, 0x06, 0x06);
	ViaSeqMask( 0x40, 0x00, 0x06);

	for(i=0x32; i<=0x36;i++) __svgalib_outcrtc(i, restore->crt[i]);
	for(i=0x50; i<=0xb7;i++) __svgalib_outcrtc(i, restore->crt[i]);
			
  
}


/* Return nonzero if mode is available */
static int unichrome_modeavailable(int mode)
{
    struct vgainfo *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;


    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];
    if (unichrome_memory * 1024 < info->ydim * info->xbytes)
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

/* Local, called by unichrome_setmode(). */

static void unichrome_initializemode(uint8_t *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{ /* long k; */

    int vd,vt,vbs,vbe,ht,hd,hss,hse, of, tmp;
    UNIRegPtr regs;

    regs=(UNIRegPtr)(moderegs+60);
   
    unichrome_saveregs(moderegs);

    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

    hd  = (modetiming->CrtcHDisplay>>3)-1;
    hss = (modetiming->CrtcHSyncStart>>3);
    hse = (modetiming->CrtcHSyncEnd>>3);
    ht  = (modetiming->CrtcHTotal>>3)-5;

	regs->crt[0x33] = 0;
	regs->crt[0x33] |= (hss&0x100)>>4;
	regs->crt[0x33] |= (hse&0x040)>>1;
	regs->crt[0x33] |= 7; /* line compare */
	
	regs->crt[0x36] &= ~0x08;
	regs->crt[0x36] |= (ht&0x100)>>5;
    
    vd   = modetiming->CrtcVDisplay - 1;
    vt   = modetiming->CrtcVTotal - 2;
    vbs  = vd;
    vbe  = vt; 

	regs->crt[0x35] &= ~0xef;
	regs->crt[0x35] |= (vt&0x400)>>10;
	regs->crt[0x35] |= (vbs&0x400)>>9;
	regs->crt[0x35] |= (vd&0x400)>>8;
	regs->crt[0x35] |= (vbe&0x400)>>7;
	
	of=modeinfo->lineWidth/8;
	moderegs[VGA_CR13] = of&0xff;
	regs->crt[0x35] |= (of&0x700)>>3;

	regs->crt[0x34] = 0; /* start address */

    switch (modeinfo->bitsPerPixel) {
		case 8:
			regs->seq[0x15] = 0x22;	
		    break;
	    case 15: 
	    case 16:if(modeinfo->greenWeight==5){
                        regs->seq[0x15] = 0xb6;
                    } else regs->seq[0x15] = 0xb6;
		    break;
//	    case 24: 
//               	    regs->vidProcCfg|=2<<18;
//		    break;
	    case 32: 
			regs->seq[0x15] = 0xae;
			break;
	    default: 
		    break;
    }

	tmp=CalcPLL(modetiming->pixelClock, 0);
	regs->seq[0x46] = tmp>>8;
	regs->seq[0x47] = tmp&0xff;
    
//	moderegs[VGA_GR5]=0;
	moderegs[VGA_MISCOUTPUT]|=0x0c; /* pixel clock = pllCtrl0 */

	/* FIFO */
	regs->seq[0x17] = 0x1f;
	regs->seq[0x1a] = 0x08;

	if(hd>=1600) {
		regs->seq[0x16] = 0x0f;	
		regs->seq[0x18] = 0x4f;
	} else if(hd>=1024) {
		regs->seq[0x16] = 0x0c;	
		regs->seq[0x18] = 0x4c;
	} else {
		regs->seq[0x16] = 0x08;	
		regs->seq[0x18] = 0x4e;
	}

	/* Fetch count */
	if(of&3)of=(of+3)&~3;
	regs->seq[0x1c] = (of>>1)&0xff;
	regs->seq[0x1d] = (of>>9)&0xff;

    unichrome_is_linear=0;

}

static int unichrome_setmode(int mode, int prv_mode)
{
    uint8_t *moderegs;
    ModeTiming modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {
		return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    }
    
	if (!unichrome_modeavailable(mode))
		return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    if (__svgalib_getmodetiming(&modetiming, modeinfo, cardspecs)) {
		free(modeinfo);
		return 1;
    }

    moderegs = malloc(sizeof(UNIRegRec) + 60);

    unichrome_initializemode(moderegs, &modetiming, modeinfo, mode);

    unichrome_setregs(moderegs, mode);		/* Set extended regs. */
    __svgalib_setregs(moderegs);	/* Set standard regs. */
    free(moderegs);

    free(modeinfo);

    return 0;
}

/* Unlock chipset-specific registers */

static void unichrome_unlock(void)
{
    __svgalib_outcrtc(0x11,__svgalib_incrtc(0x11)&0x7f);    
    __svgalib_outcrtc(0x47,__svgalib_incrtc(0x47)&0xfe);    
    __svgalib_outseq(0x10,__svgalib_inseq(0x10)|1);    
}

static void unichrome_lock(void)
{
    __svgalib_outseq(0x10,__svgalib_inseq(0x10)&0xfe);    
}


/* Indentify chipset, initialize and return non-zero if detected */

static int unichrome_test(void)
{
   return unichrome_init(0,0,0); 
}

/* Set display start address (not for 16 color modes) */
/* Cirrus supports any address in video memory (up to 2Mb) */

static void unichrome_setdisplaystart(int address)
{ 
  __svgalib_outcrtc(0x23, ((address>>2) & 0xFF0000)>>16);
  __svgalib_outcrtc(0x0c, ((address>>2) & 0x00FF00)>>8);
  __svgalib_outcrtc(0x0d, (address>>2) & 0x00FF);
}


/* Set logical scanline length (usually multiple of 8) */

static void unichrome_setlogicalwidth(int width)
{   
    int offset = width >> 3;
 
    __svgalib_outcrtc(0x13,offset&0xff);
    __svgalib_outcrtc(0x35, (__svgalib_incrtc(0x35)&0x1f)| ((offset&0x700)>>3));
}

static int unichrome_linear(int op, int param)
{
    if (op==LINEAR_DISABLE) { unichrome_is_linear=0; return 0;}
    if (op==LINEAR_ENABLE)  { unichrome_is_linear=1; return 0;}
    if (op==LINEAR_QUERY_BASE) return unichrome_linear_base;
    if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY) return 0;		/* No granularity or range. */
        else return -1;		/* Unknown function. */
}

static int unichrome_match_programmable_clock(int clock)
{
return clock ;
}

static int unichrome_map_clock(int bpp, int clock)
{
return clock ;
}

static int unichrome_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
return htiming;
}

static unsigned int cur_colors[16*2];

static int unichrome_cursor( int cmd, int p1, int p2, int p3, int p4, void *p5) {
    int i, j;
    unsigned long *b3;
    unsigned long l1, l2;
    
    switch(cmd){
        case CURSOR_INIT:
            return 1;
        case CURSOR_HIDE:
            port_outl_r(unichrome_io_base+0x5c,port_inl(unichrome_io_base+0x5c)&~(1<<27));
            break;
        case CURSOR_SHOW:
            port_outl_r(unichrome_io_base+0x5c,port_inl(unichrome_io_base+0x5c)|(1<<27));
            break;
        case CURSOR_POSITION:
            port_outl_r(unichrome_io_base+0x64,((p2+64)<<16)|(p1+64));
            break;
        case CURSOR_SELECT:
            i=unichrome_memory*1024-(p1+1)*4096;
            port_outl_r(unichrome_io_base+0x68,cur_colors[p1*2]);
            port_outl_r(unichrome_io_base+0x6c,cur_colors[p1*2+1]);
            port_outl_r(unichrome_io_base+0x60,i);
            break;
        case CURSOR_IMAGE:
            i=unichrome_memory*1024-(p1+1)*4096;
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

DriverSpecs __svgalib_unichrome_driverspecs =
{
    unichrome_saveregs,
    unichrome_setregs,
    unichrome_unlock,
    unichrome_lock,
    unichrome_test,
    unichrome_init,
//    unichrome_setpage,
	__svgalib_emul_setpage,
    NULL,
    NULL,
    unichrome_setmode,
    unichrome_modeavailable,
    unichrome_setdisplaystart,
    unichrome_setlogicalwidth,
    unichrome_getmodeinfo,
    0,				/* old blit funcs */
    0,
    0,
    0,
    0,
    0,				/* ext_set */
    0,				/* accel */
    unichrome_linear,
    0,				/* accelspecs, filled in during init. */
    NULL,                       /* Emulation */
    unichrome_cursor
};

/* Initialize chipset (called after detection) */

static int unichrome_init(int force, int par1, int par2)
{
    unsigned long buf[64];
    int found=0;

    if (force) {
		unichrome_memory = par1;
        chiptype = par2;
    } else {

    };
    
    found=(!__svgalib_pci_find_vendor_vga(0x1106,buf,0))&& (
            ((buf[0]>>16)==0x3108)||
            ((buf[0]>>16)==0x3118)||
            ((buf[0]>>16)==0x3122)||
            ((buf[0]>>16)==0x7205));
	if(!found) return 0;

    switch(buf[0]>>16) {
		case 0x3122:
			if((buf[3]&0xff)<0x10) chiptype=CLE266; else chiptype=CLE266CX;
//			unichrome_memory=1024*__svgalib_inseq(0x34);
			break;
		case 0x3108:
			chiptype=KM800;
			break;
		case 0x3118:
			chiptype=PM800;
			break;
		case 0x7205:
			chiptype=KM400;
			break;
	}		
    
    if (found){
       unichrome_linear_base=buf[4]&0xffffff00;
//       unichrome_io_base=buf[6]&0xff00;
//       __svgalib_io_reloc=unichrome_io_base-0x300;
//       __svgalib_rel_io_mapio();
    };

    if(unichrome_memory==0) {
		unichrome_memory = 16384;
    }
    
    unichrome_unlock();
    
    if (__svgalib_driver_report) {
	fprintf(stderr,"Using VIA Unichrome driver, %iKB.\n",unichrome_memory);
    }
    
	__svgalib_modeinfo_linearset |= IS_LINEAR;
	__svgalib_banked_mem_base=unichrome_linear_base;
	
    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = unichrome_memory;
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->maxPixelClock8bpp = 270000;
    cardspecs->maxPixelClock16bpp = 270000;
    cardspecs->maxPixelClock24bpp = 0;
    cardspecs->maxPixelClock32bpp = 270000;
    cardspecs->flags = CLOCK_PROGRAMMABLE | NO_INTERLACE;
    cardspecs->maxHorizontalCrtc = 2040;
    cardspecs->nClocks = 0;
    cardspecs->mapClock = unichrome_map_clock;
    cardspecs->mapHorizontalCrtc = unichrome_map_horizontal_crtc;
    cardspecs->matchProgrammableClock=unichrome_match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_unichrome_driverspecs;
//    __svgalib_banked_mem_base=0xa0000;
//    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=unichrome_linear_base;
    __svgalib_linear_mem_size=unichrome_memory*0x400;
    return 1;
}

#define REFFREQ 14318

static int
CalcPLL(int freq, int isBanshee) {
	int m, n, k, best_m, best_n, best_k, f_cur, best_error;
	int minm, maxm, minn, maxn;

  	best_error=freq;
  	best_n=best_m=best_k=0;

	minm=3;
  	maxm=16; 
  	minn=0x13;
  	maxn=0x79;

	for (n=minn; n<maxn; n++) {
		int e;
		f_cur=REFFREQ*n;
		m=(f_cur+(freq/2))/freq;
		k=0;
		
		while(m>maxm) {
			k++;
			m/=2;
		}

		while( (k<4) && ((m&1)==0) && (f_cur/m<160000)) {
			k++;
			m/=2;
		}

		if( k>3 || (m>maxm) || (m<minm)) continue;

		e=REFFREQ*n/m/(1<<k)-freq;
		if(e<0)e=-e;

		if(e<best_error) {
			best_error=e;
			best_n=n;
			best_m=m;
			best_k=k;
		}
  	}
	
  	return best_n|(best_m<<8)|(best_k<<14);
}


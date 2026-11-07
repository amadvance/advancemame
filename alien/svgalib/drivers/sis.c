/*
SIS chipset driver 

Matan Ziv-Av <matan@svgalib.org>

Fixes for 530 and for vesafb by Marcelo de Paula Bezerra 
<mosca@internetaddress.com>.

This driver is based on the XFree86 sis driver, written by

 Alan Hourihane

and modified by
 Xavier Ducoin,
 Mike Chapman,
 Juanjo Santamarta,
 Mitani Hiroshi,
 David Thomas,
 Thomas Winischhofer.
 
And on the documentation availbale from sis's web pages (not many
chipset companies still do this).

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
#include "vgarelvg.h"

#define SISREG_SAVE(i) (VGA_TOTAL_REGS+i)
#define SIS_TOTAL_REGS (VGA_TOTAL_REGS + 64 + 40 + 20)

#define XR(i) (VGA_TOTAL_REGS+i-5)
#define CR(i) (VGA_TOTAL_REGS+i+0x28)

#define PCI_CHIP_SG86C201       0x0001
#define PCI_CHIP_SG86C202       0x0002
#define PCI_CHIP_SG86C205       0x0205
#define PCI_CHIP_SG86C215       0x0215
#define PCI_CHIP_SG86C225       0x0225
#define PCI_CHIP_SIS5597        0x0200
#define PCI_CHIP_SIS6326        0x6326
#define PCI_CHIP_SIS530         0x6306

#define PCI_CHIP_SIS300		0x0300
#define PCI_CHIP_SIS540		0x5300
#define PCI_CHIP_SIS630		0x6300
#define PCI_CHIP_SIS730		0x7300

#define PCI_CHIP_SIS315H	0x0310
#define PCI_CHIP_SIS315		0x0315
#define PCI_CHIP_SIS315PRO	0x0325
#define PCI_CHIP_SIS550		0x5315
#define PCI_CHIP_SIS650		0x6325

#define ClockReg	XR(0x07) 
#define DualBanks	XR(0x0B)
#define BankReg		XR(0x06)
#define CRTCOff		XR(0x0A)
#define DispCRT		XR(0x27)
#define Unknown		XR(0x08)
#define LinearAddr0	XR(0x20)
#define LinearAddr1	XR(0x21)

enum { SIS_OLD=1, SIS_5597, SIS_6326, SIS_530, SIS_300, SIS_315 };

#define write_xr(num,val) {__svgalib_outseq(num,val);}
#define read_xr(num,var) {var=__svgalib_inseq(num);} 
#define in_xr(num) __svgalib_inseq(num)

static int sis_init(int, int, int);
static void sis_unlock(void);
static void sis_lock(void);
static void sisClockLoad(int, unsigned char *);
static void sis_CPUthreshold(int,int,int *,int *);
static int compute_vclk(int Clock, int *out_n, int *out_dn, int *out_div, int *out_sbit, int *out_scale);

static int sis_memory;
static int sis_is_linear, sis_linear_base;
static int chip;

static int sis_edo_vram = 0, sis_host_bus = 0, sis_fast_vram = 0,
           sis_pci_burst_on = 0, sis_pci_burst_off = 0;

static CardSpecs *cardspecs;

static void sis_setpage(int page)
{
        port_out_r(0x3cb+__svgalib_io_reloc,page);
        port_out_r(0x3cd+__svgalib_io_reloc,page);
}

static void sis_300_setpage(int page)
{
        port_out_r(0x3cb+__svgalib_io_reloc,((page>>4)&0x0f) | (page&0xf0));
        port_out_r(0x3cd+__svgalib_io_reloc,(page&0x0f) | ((page&0x0f)<<4));
}

static int __svgalib_sis_inlinearmode(void)
{
return sis_is_linear;
}

/* Fill in chipset specific mode information */

static void sis_getmodeinfo(int mode, vga_modeinfo *modeinfo)
{

    if(modeinfo->colors==16)return;

    modeinfo->maxpixels = sis_memory*1024/modeinfo->bytesperpixel;
    modeinfo->maxlogicalwidth = 8192;
    modeinfo->startaddressrange = sis_memory * 1024 - 1;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;

    if (modeinfo->bytesperpixel >= 1) {
	if(sis_linear_base)modeinfo->flags |= CAPABLE_LINEAR;
        if (__svgalib_sis_inlinearmode())
	    modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
    }
}

/* Read and save chipset-specific registers */

static int sis_saveregs(unsigned char regs[])
{ 
    int i;
    int max=0x37;
    
    switch(chip) {
        case SIS_5597:
            max=0x39;
            break;
        case SIS_6326:
        case SIS_530:
            max=0x3f;
            break;
        case SIS_300:
        case SIS_315:
            max=0x3d;
            break;
    }
    
    sis_unlock();		

    for(i=6; i<=max;i++)read_xr(i,regs[XR(i)]);

    if(chip>=SIS_300) {
        int tmp;
        for(i=0x19; i<0x40;i++)regs[CR(i)]=__svgalib_incrtc(i);
        read_xr(0x20,tmp);
        write_xr(0x20,tmp|0x01);
        *(uint32_t *)(regs+CR(0x40))=v_readl(0x8500 + 0);
        *(uint32_t *)(regs+CR(0x44))=v_readl(0x8500 + 4);
        *(uint32_t *)(regs+CR(0x48))=v_readl(0x8500 + 8);
        *(uint32_t *)(regs+CR(0x4c))=v_readl(0x8500 + 12);
        *(uint32_t *)(regs+CR(0x50))=v_readl(0x8500 + 16);
        write_xr(0x20,tmp);
    }
    
    return SIS_TOTAL_REGS - VGA_TOTAL_REGS;
}

/* Set chipset-specific registers */

static void sis_setregs(const unsigned char regs[], int mode)
{  
    int i;
    int max=0x37;
    
    switch(chip) {
        case SIS_5597:
            max=0x39;
            break;
        case SIS_6326:
        case SIS_530:
            max=0x3f;
            break;
        case SIS_300:
        case SIS_315:
            max=0x3d;
            break;
    }

    sis_unlock();

    if(chip>=SIS_300) {
        write_xr(0x20,in_xr(0x20)|0x01);
        v_writel(*(uint32_t *)(regs+CR(0x40)), 0x8500 + 0);
        v_writel(*(uint32_t *)(regs+CR(0x44)), 0x8500 + 4);
        v_writel(*(uint32_t *)(regs+CR(0x48)), 0x8500 + 8);
        v_writel(*(uint32_t *)(regs+CR(0x4c)), 0x8500 + 12);
        v_writel(*(uint32_t *)(regs+CR(0x50)), 0x8500 + 16);
        for(i=0x19; i<0x40;i++)__svgalib_outcrtc(i,regs[CR(i)]);
    }
    
    for(i=6; i<=max; i++)write_xr(i,regs[XR(i)]);
    
}

/* Return nonzero if mode is available */

static int sis_modeavailable(int mode)
{
    struct vgainfo *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];
    if (sis_memory * 1024 < info->ydim * info->xbytes)
	return 0;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);
    
    if(modeinfo->bytesPerPixel==4-(chip>=SIS_300)){
        free(modeinfo);
        return 0;
    };

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

/* Local, called by sis_setmode(). */

static void sis_initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{ 
    int offset;
    
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

    sis_saveregs(moderegs);
    
    sisClockLoad(modetiming->pixelClock,moderegs);
    offset = modetiming->CrtcHDisplay >> 
             ((modetiming->flags & INTERLACED) ? 2 : 3);

    moderegs[VGA_CR14] = 0x40;
    moderegs[VGA_CR17] = 0xA3;
    
    moderegs[ATT+0x10] = 0x01;   /* mode */
    moderegs[ATT+0x11] = 0x00;   /* overscan (border) color */
    moderegs[ATT+0x12] = 0x0F;   /* enable all color planes */
    moderegs[ATT+0x13] = 0x00;   /* horiz pixel panning 0 */

    if ( (modeinfo->bitsPerPixel == 16) || (modeinfo->bitsPerPixel == 24) )
	    moderegs[GRA+0x05] = 0x00;    /* normal read/write mode */

    if (modeinfo->bitsPerPixel == 16) {
	    offset <<= 1;	       /* double the width of the buffer */
    } else if (modeinfo->bitsPerPixel == 24) {
	    offset += offset << 1;
    } 

    moderegs[BankReg] = 0x02;
    moderegs[DualBanks] = 0x00; 

    if ( sis_is_linear || __svgalib_emulatepage) {
	    moderegs[BankReg] |= 0x80;  	/* enable linear mode addressing */
	    moderegs[LinearAddr0] = (sis_linear_base & 0x07f80000) >> 19 ; 
            moderegs[LinearAddr1] = ((sis_linear_base & 0xf8000000) >> 27) |
		                (0x60) ; /* Enable Linear with max 4 mb*/
    } else moderegs[DualBanks] |= 0x08;

    if (modeinfo->bitsPerPixel == 16) {
	    if (modeinfo->greenWeight == 5)
		moderegs[BankReg] |= 0x04;	/* 16bpp = 5-5-5 */
	    else
		moderegs[BankReg] |= 0x08;	/* 16bpp = 5-6-5 */
    }

    if (modeinfo->bitsPerPixel == 24) {
	    moderegs[BankReg] |= 0x10;
            moderegs[DualBanks] |= 0x80;
    }

    moderegs[0x13] = offset & 0xFF;
    moderegs[CRTCOff] = ((offset & 0xF00) >> 4) | 
	    (((modetiming->CrtcVTotal-2) & 0x400) >> 10 ) |
            (((modetiming->CrtcVDisplay-1) & 0x400) >> 9 ) |
            (((modetiming->CrtcVSyncStart) & 0x400) >> 8 ) |
            (((modetiming->CrtcVSyncStart) & 0x400) >> 7 ) ;
	
    if (modetiming->flags & INTERLACED)
		moderegs[BankReg] |= 0x20;
    
    if ((chip == SIS_5597) || 
        (chip == SIS_6326) || 
        (chip == SIS_530)) {
               moderegs[XR(0x0C)] |= 0x20; /* readahead cache */
               moderegs[XR(0x07)] |= 0x80; /* combine FIFOs */
    }

    if((chip == SIS_530) || (chip==SIS_5597)) {
	moderegs[XR(0x0c)] |= 0x80; /* 32 bit memory access */
        moderegs[XR(0x26)]&=0xfe; /* zero bit 20 of start address */
    };

    /* makes SR27 d[3:1]=0; If yes, I believe this is the offset for the 
       scroll, or somthing like that... */
    moderegs[XR(0x27)]&=0xf0; 

    {
        int CRT_ENGthreshold,CRT_CPUthresholdLow, CRT_CPUthresholdHigh;
    
        CRT_ENGthreshold = 0x1F; 
        sis_CPUthreshold(modetiming->pixelClock, modeinfo->bitsPerPixel,
			     &CRT_CPUthresholdLow, &CRT_CPUthresholdHigh);
        moderegs[XR(0x08)] = (CRT_ENGthreshold & 0x0F) | 
	    (CRT_CPUthresholdLow & 0x0F)<<4 ;
        moderegs[XR(0x09)] &= 0xF0;  /* Yeou */
        moderegs[XR(0x09)] |= (CRT_CPUthresholdHigh & 0x0F); /* Yeou */
    
        if (chip == SIS_530) {
	        /* CRT/CPU/Engine Threshold Bit[4] */
	        moderegs[XR(0x3F)] &= 0xE3;
                moderegs[XR(0x3F)] |= ((CRT_CPUthresholdHigh & 0x10) |
				        ((CRT_ENGthreshold & 0x10) >> 1) |
				        ((CRT_CPUthresholdLow & 0x10) >> 2));
        }
    }
    

    moderegs[59]=0x6f;

    moderegs[XR(0x1c)]=0;
    moderegs[XR(0x1f)]=0;

    return ;
}

static void sis_300_initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{ 
    int offset;
    int  n, m, d, sb, sc;
    
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

    sis_saveregs(moderegs);
    
    offset = modeinfo->lineWidth;
    
    moderegs[CR(0x19)] = 0;
    moderegs[CR(0x1A)] &= 0x0fc;
    
    if(modetiming->flags & INTERLACED) {
        offset>>=2;
        moderegs[XR(0x06)] |= 0x20;
    } else {
        offset>>=3;
        moderegs[XR(0x06)] &= ~0x20;
    }
    
    switch(modeinfo->bytesPerPixel) {
        case 1:
            moderegs[XR(0x06)] |= (0<<2) | 3;
            break;
        case 2:
            moderegs[XR(0x06)] |= ((modeinfo->greenWeight-4)<<2) | 3;
            break;
        case 3:
            moderegs[XR(0x06)] |= (3<<2) | 3;
            break;
        case 4:
            moderegs[XR(0x06)] |= (4<<2) | 3;
            break;
    }

    moderegs[XR(0x07)] |= 0x10;     	/* enable High Speed DAC */
    moderegs[XR(0x07)] &= 0xFC;
    if (modetiming->pixelClock < 100000)
        moderegs[XR(0x07)] |= 0x03;
    else if (modetiming->pixelClock < 200000)
        moderegs[XR(0x07)] |= 0x02;
    else if (modetiming->pixelClock < 250000)
        moderegs[XR(0x07)] |= 0x01;
    
    moderegs[XR(0x0a)] = 
	    (((modetiming->CrtcVTotal-2)   & 0x400) >> 10) |
            (((modetiming->CrtcVDisplay-1) & 0x400) >> 9 ) |
            (((modetiming->CrtcVSyncStart) & 0x400) >> 8 ) |
            (((modetiming->CrtcVSyncStart) & 0x400) >> 7 ) |
            (((modetiming->CrtcVSyncEnd)   & 0x100) >> 4 ) |
            (((modetiming->CrtcVSyncEnd)   & 0x010) << 1 ) ;

    moderegs[XR(0x0b)] = 
	    ((((modetiming->CrtcHTotal>>3)-5)   & 0x300) >> 8 ) |
            ((((modetiming->CrtcHDisplay>>3)-1) & 0x300) >> 6 ) |
            ((((modetiming->CrtcHSyncStart>>3)) & 0x300) >> 4 ) |
            ((((modetiming->CrtcVSyncStart>>3)) & 0x300) >> 2 ) ;
    
    moderegs[XR(0x0c)] &= 0xf8;
    moderegs[XR(0x0c)] =
	    ((((modetiming->CrtcHSyncEnd>>3))   & 0xc0) >> 6 ) |
	    ((((modetiming->CrtcHSyncEnd>>3))   & 0x20) >> 3 ) ;

    
    moderegs[0x13] = offset & 0xFF;
    moderegs[XR(0x0e)] &= 0xf0;
    moderegs[XR(0x0e)] |= (offset&0xf00)>>8;
     
    moderegs[XR(0x0f)] |= 0x08;
    
    moderegs[XR(0x10)] = ((modeinfo->lineWidth+63) >> 6 ) + 1;
    
    moderegs[XR(0x20)] &= 0xa1;
    moderegs[XR(0x20)] |= 0x80; /* Enable linear */
    moderegs[XR(0x20)] |= 0x01; /* Enable MMIO */

    if (!compute_vclk(modetiming->pixelClock, &n, &m, &d, &sb, &sc))
	return;

    moderegs[XR(0x2b)] = ((n-1) & 0x7f) | ((d-1)<<7);
    moderegs[XR(0x2c)] = ((m-1) & 0x1f) | (((sc-1)&3)<<5) | (sb ? 0x80:0);
    moderegs[XR(0x2d)] = 0x80;
    
/*
    if(modetiming->pixelClock>150000) {
        moderegs[XR(0x07)] |= 0x80;
        moderegs[XR(0x32)] |= 0x08;    
    } else {
        moderegs[XR(0x07)] &= ~0x80;
        moderegs[XR(0x32)] &= ~0x08;    
    }
*/
    
    moderegs[XR(0x21)] = 0xa5; /* What is this ??? */

    moderegs[ATT + 0x10] = 1;
    moderegs[ATT + 0x11] = 0;
    moderegs[ATT + 0x12] = 0;
    
    moderegs[MIS] |= 0x0c; /* Use programmed clock */
    
    return ;
}


static int sis_setmode(int mode, int prv_mode)
{
    unsigned char *moderegs;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;
    int tmp;

    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {
        if(chip >= SIS_5597) { 
		/* Work Arrout for vesafb case */
                read_xr(0x06,tmp);
                tmp&=~(0x1E); /* Depth registers */
                write_xr(0x06,tmp);
        }
 
	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    }
    if (!sis_modeavailable(mode))
	return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 1;
    }

    moderegs = malloc(SIS_TOTAL_REGS);
    
    if(chip>=SIS_300) 
        sis_300_initializemode(moderegs, modetiming, modeinfo, mode); else
        sis_initializemode(moderegs, modetiming, modeinfo, mode);

    free(modetiming);

    __svgalib_setregs(moderegs);	/* Set standard regs. */
    sis_setregs(moderegs, mode);		/* Set extended regs. */
    free(moderegs);

    free(modeinfo);
    return 0;
}


/* Unlock chipset-specific registers */

static void sis_unlock(void)
{
    __svgalib_outcrtc(0x11,__svgalib_incrtc(0x11)&0x7f);
    __svgalib_outseq(0x05,0x86);
}

static void sis_lock(void)
{
    __svgalib_outcrtc(0x11,__svgalib_incrtc(0x11)&0x7f);
    __svgalib_outseq(0x05,0x0);
}

#define VENDOR_ID 0x1039

/* Indentify chipset, initialize and return non-zero if detected */

static int sis_test(void)
{
    int found;
    long buf[64];
    
    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    
    if(!found&&(
       (((buf[0]>>16)&0xffff)==PCI_CHIP_SG86C201)||
       (((buf[0]>>16)&0xffff)==PCI_CHIP_SG86C202)||
       (((buf[0]>>16)&0xffff)==PCI_CHIP_SG86C205)||
       (((buf[0]>>16)&0xffff)==PCI_CHIP_SG86C215)||
       (((buf[0]>>16)&0xffff)==PCI_CHIP_SG86C225)||

       (((buf[0]>>16)&0xffff)==PCI_CHIP_SIS5597)||
       (((buf[0]>>16)&0xffff)==PCI_CHIP_SIS6326)||
       (((buf[0]>>16)&0xffff)==PCI_CHIP_SIS530)||

       (((buf[0]>>16)&0xffff)==PCI_CHIP_SIS540)||
       (((buf[0]>>16)&0xffff)==PCI_CHIP_SIS630)||
       (((buf[0]>>16)&0xffff)==PCI_CHIP_SIS730)||
       (((buf[0]>>16)&0xffff)==PCI_CHIP_SIS300)||

       (((buf[0]>>16)&0xffff)==PCI_CHIP_SIS315H)||
       (((buf[0]>>16)&0xffff)==PCI_CHIP_SIS315)||
       (((buf[0]>>16)&0xffff)==PCI_CHIP_SIS315PRO)||
       (((buf[0]>>16)&0xffff)==PCI_CHIP_SIS550)||
       (((buf[0]>>16)&0xffff)==PCI_CHIP_SIS650)||
       0)){
       sis_init(0,0,0);
       return 1;
    };
    return 0;
}


/* Set display start address (not for 16 color modes) */
static void sis_setdisplaystart(int address)
{ 
  int temp;
  address=address >> 2;
  __svgalib_outcrtc(0x0c,(address & 0x00FF00)>>8);
  __svgalib_outcrtc(0x0d,address & 0x0000ff);

  if(chip>=SIS_300) {
      write_xr(0x0d,(address&0xff0000)>>16);
  } else {
      read_xr(0x27,temp);
      temp &= 0xf0;
      temp |= (address&0xf0000)>>16;
      write_xr(0x27,temp);
  }
}

/* Set logical scanline length (usually multiple of 8) */
static void sis_setlogicalwidth(int width)
{   
    int offset = width >> 3;
    int temp;
    
    __svgalib_outcrtc(0x13,offset & 0xff);
    
    if(chip>=SIS_300) {        
        read_xr(0x0e,temp);
        temp &= 0xf0;
        temp |= (offset&0xf00)>>8;
        write_xr(0x0e,temp);
    } else {
        read_xr(0x0a,temp);
        temp &= 0xf;
        temp |= (offset&0xf00)>>4;
        write_xr(0x0a,temp);
    }
}

static int sis_linear(int op, int param)
{
    if (op==LINEAR_ENABLE){
        int temp;
        
        sis_is_linear=1; 
        if(chip<SIS_300) {
            read_xr(6,temp);
            temp |=0x80;
            write_xr(0x20,(sis_linear_base & 0x07f80000) >> 19) ; 
            write_xr(0x21,((sis_linear_base & 0xf8000000) >> 27) |
		                    (0x60)) ; /* Enable Linear with max 4 mb*/
            write_xr(6,temp);
        }
        return 0;
    };
    if (op==LINEAR_DISABLE && !__svgalib_emulatepage){
        int temp;

        sis_is_linear=0; 
        if(chip<SIS_300) {
            read_xr(6,temp);
            temp &=0x7f;
            write_xr(0x20,0) ; 
            write_xr(0x21,0) ; 
            write_xr(6,temp);
        }
        return 0;
    };
    if (op==LINEAR_QUERY_BASE) return sis_linear_base;
    if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY) return 0;		/* No granularity or range. */
        else return -1;		/* Unknown function. */
}

static unsigned char cur_colors[16*6];

static int sis_cursor( int cmd, int p1, int p2, int p3, int p4, void *p5) {
    unsigned long *b3;
    int i, j, k, l; 
    uint32_t l1, l2;
    unsigned char c;
    
    switch(cmd){
        case CURSOR_INIT:
            return 1;
        case CURSOR_HIDE:
            write_xr(6,in_xr(6)&0xbf);
            break;
        case CURSOR_SHOW:
            write_xr(6,in_xr(6)|0x40);
            break;
        case CURSOR_POSITION:
            write_xr(0x1a,p1&0xff);
            write_xr(0x1b,p1>>8);
            p2>>=1;
            write_xr(0x1d,p2&0xff);
            write_xr(0x1e,(in_xr(0x1e)&0xf8)|((p2>>8)&0x07));
            break;
        case CURSOR_SELECT:
#if 1
            i=sis_memory*1024-(16-p1)*1024;
#else
            i=sis_memory*1024-16384;
#endif
            if((chip==SIS_6326)&&(sis_memory>4096))
               i-=4096*1024; /* 6326 can't put cursor beyond 4MB */
            for(j=0;j<6;j++)write_xr(0x14+j,cur_colors[p1*6+j]>>2);
            write_xr(0x1e,(in_xr(0x1e)&0x0f)|(p1<<4));
            write_xr(0x38,(in_xr(0x38)&0x0f)|((i>>18)<<4));
            if(i&0x400000) {
               write_xr(0x3e,in_xr(0x3e)|0x04);
            } else {
               write_xr(0x3e,in_xr(0x3e)&0xfb);
            }
            break;
        case CURSOR_IMAGE:
            i=sis_memory*1024-(16-p1)*1024;
            if((chip==SIS_6326)&&(sis_memory>4096))
               i-=4096*1024; /* 6326 can't put cursor beyond 4MB */
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
                            *(LINEAR_POINTER+i+16*j+k)=c;
                            *(LINEAR_POINTER+i+16*j+k+8)=0xaa;
                        }
                    }
                    memset(LINEAR_POINTER+i+512,0xaa,512);
                    break;
            }
            break;
    }
    return 0;
}       

static int sis_300_cursor( int cmd, int p1, int p2, int p3, int p4, void *p5) {
    unsigned long *b3;
    int i, j; 
    uint32_t l1, l2;
    
    switch(cmd){
        case CURSOR_INIT:
            return 1;
        case CURSOR_HIDE:
            l1 = v_readl(0x8500 + 0);
            v_writel(l1 & 0x3fffffff, 0x8500 + 0);
            break;
        case CURSOR_SHOW:
            l1 = v_readl(0x8500 + 0);
            v_writel((l1 & 0x3fffffff) | 0x40000000, 0x8500 + 0);
            break;
        case CURSOR_POSITION:
            v_writel(p1, 0x8500 + 12);
            v_writel(p2, 0x8500 + 16);
            break;
        case CURSOR_SELECT:
            i=sis_memory-16+p1;
            if(i>=0x10000)i&=0xffff; /* can use more that 64MB ? */
            v_writel(cur_colors[p1*6+2]+(cur_colors[p1*6+1]<<8)+(cur_colors[p1*6+0]<<16), 0x8500 + 4);
            v_writel(cur_colors[p1*6+5]+(cur_colors[p1*6+4]<<8)+(cur_colors[p1*6+3]<<16), 0x8500 + 8);
            l1 = v_readl(0x8500 + 0);
            v_writel((l1 & 0xf0fe0000) | i, 0x8500 + 0);
            break;
        case CURSOR_IMAGE:
            i=(sis_memory-16+p1)*1024;
            if(i>0x4000000)i&=0x3ffffff;
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
                        l2=~(*(b3+32+j));
                        l1=(l1<<24) | ((l1&0xff00)<<8) | ((l1>>8)&0xff00) | (l1>>24);
                        l2=(l2<<24) | ((l2&0xff00)<<8) | ((l2>>8)&0xff00) | (l2>>24);
                        *(uint32_t *)(LINEAR_POINTER+i+16*j)=l2;
                        *(uint32_t *)(LINEAR_POINTER+i+16*j+8)=l1;
                        *(uint32_t *)(LINEAR_POINTER+i+16*j+4)=0xffffffff;
                        *(uint32_t *)(LINEAR_POINTER+i+16*j+12)=0;
                    }
                    for(j=32;j<64;j++) {
                        *(uint32_t *)(LINEAR_POINTER+i+16*j)=0xffffffff;
                        *(uint32_t *)(LINEAR_POINTER+i+16*j+8)=0;
                        *(uint32_t *)(LINEAR_POINTER+i+16*j+4)=0xffffffff;
                        *(uint32_t *)(LINEAR_POINTER+i+16*j+12)=0;
                    }
                    break;
            }
            break;
    }
    return 0;
}       

static int sis_match_programmable_clock(int clock)
{
return clock ;
}

static int sis_map_clock(int bpp, int clock)
{
return clock ;
}

static int sis_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
return htiming;
}

/* Function table (exported) */

DriverSpecs __svgalib_sis_driverspecs =
{
    sis_saveregs,
    sis_setregs,
    sis_unlock,
    sis_lock,
    sis_test,
    sis_init,
    sis_setpage,
    NULL,
    NULL,
    sis_setmode,
    sis_modeavailable,
    sis_setdisplaystart,
    sis_setlogicalwidth,
    sis_getmodeinfo,
    0,				/* old blit funcs */
    0,
    0,
    0,
    0,
    0,				/* ext_set */
    0,				/* accel */
    sis_linear,
    0,				/* accelspecs, filled in during init. */
    NULL,                       /* Emulation */
    sis_cursor
};

/* Initialize chipset (called after detection) */

static int sis_init(int force, int par1, int par2)
{
    unsigned long buf[64];
    int pci_id;
    int found=0;

    sis_unlock();
    if (force) {
	sis_memory = par1;
        chip = par2;
    } else {

    };

    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    sis_linear_base=0;
    chip=0;
    pci_id=0;
    if (!found){
       sis_linear_base=buf[4]&0xffffff00;
       pci_id=(buf[0]>>16)&0xffff;
       switch(pci_id) {
            case PCI_CHIP_SG86C201:
            case PCI_CHIP_SG86C202: 
            case PCI_CHIP_SG86C205:
            case PCI_CHIP_SG86C215:
            case PCI_CHIP_SG86C225:
                chip=SIS_OLD; break;

            case PCI_CHIP_SIS5597: 
                chip=SIS_5597; break;
            
            default:
            case PCI_CHIP_SIS6326: 
                chip=SIS_6326; break;

            case PCI_CHIP_SIS530: 
                chip=SIS_530; break;

            case PCI_CHIP_SIS300:
            case PCI_CHIP_SIS540:
            case PCI_CHIP_SIS630:
            case PCI_CHIP_SIS730:
                chip = SIS_300;
                break;

            case PCI_CHIP_SIS315H:
            case PCI_CHIP_SIS315:
            case PCI_CHIP_SIS315PRO:
            case PCI_CHIP_SIS550:
            case PCI_CHIP_SIS650:
                chip = SIS_315;
                break;
       };
    } else {
		return -1;
	}
    
    if(chip>=SIS_5597) {
       __svgalib_io_reloc=(buf[6]&0xffffff80)-0x380; /* should work at least for 6326 and 530 */
       __svgalib_rel_io_mapio();
    }
    
    if(sis_memory==0){
        int bsiz, temp, config;
        int ramsize[8]  = { 1,  2,  4, 0, 0,  2,  4,  8};
        
        switch(chip) {
            case SIS_OLD:
                read_xr(0x0F,temp);
                sis_memory=1024<<(temp&0x03);
                if(sis_memory==8192)sis_memory=4096;
                break;
                
            case SIS_5597:
                read_xr(0x0C,temp);
                bsiz = (temp >> 1) & 3;
                read_xr(0x2F,temp);
                temp &= 7;
                temp++;
                if (bsiz > 0) temp = temp << 1;
                sis_memory = 256 * temp;
                break;
                
            case SIS_6326:
            case SIS_530:
                read_xr(0x0c, temp);
                config = ((temp & 0x10) >> 2 ) | ((temp & 0x6) >> 1);
                sis_memory = ramsize[config] * 1024;
                break;

            case SIS_300:
                read_xr(0x14, config);
                sis_memory = ((config & 0x3F) + 1) * 1024;
                break;
            
            case SIS_315:
                read_xr(0x14, temp);
                sis_memory = (1 << ((temp & 0xF0)>>4)) * 1024;
                config = (temp & 0x0C) >> 2;
                /* If SINGLE_CHANNEL_2_RANK or DUAL_CHANNEL_1_RANK -> mem * 2 */
                if ((config == 0x01) || (config == 0x03))
                    sis_memory <<= 1;
                /* If DDR asymetric -> mem * 1,5*/
                if (config == 0x02)
                    sis_memory += sis_memory/2;
                break;
        }
    };

    if (__svgalib_driver_report) {
	fprintf(stderr,"Using SIS driver, %iKB. Chiptype=%i\n",sis_memory,chip);
    };

    /* program the 25 and 28 Mhz clocks */
    if ((chip==SIS_5597)||(chip==SIS_6326)||(chip==SIS_530)) {
       int temp,var;
       read_xr(0x38,temp);
       temp &= 0xfc;
       write_xr(0x38,temp|1);
       read_xr(0x13,var);
       write_xr(0x13,var|0x40);
       write_xr(0x2a,0x1b);
       write_xr(0x2b,0xe1);
       write_xr(0x38,temp|2);
       read_xr(0x13,var);
       write_xr(0x13,var|0x40);
       write_xr(0x2a,0x4e);
       write_xr(0x2b,0xe4);
       write_xr(0x38,temp);
    }
	if(chip>=SIS_300) {
    	__svgalib_modeinfo_linearset |= IS_LINEAR;
	}

    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = sis_memory;
    cardspecs->maxPixelClock4bpp = 75000;	
    cardspecs->maxPixelClock8bpp = 135000;	
    cardspecs->maxPixelClock16bpp = 135000;	
    cardspecs->maxPixelClock24bpp = 135000;
    cardspecs->maxPixelClock32bpp = 0;
    cardspecs->flags = INTERLACE_DIVIDE_VERT | CLOCK_PROGRAMMABLE;
    cardspecs->maxHorizontalCrtc = 8160;
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->nClocks =0;
    cardspecs->mapClock = sis_map_clock;
    cardspecs->mapHorizontalCrtc = sis_map_horizontal_crtc;
    cardspecs->matchProgrammableClock=sis_match_programmable_clock;

    if(chip==SIS_300 || chip==SIS_315) {
        if(pci_id!=PCI_CHIP_SIS630) /* maybe it's all 300 */
            __svgalib_sis_driverspecs.__svgalib_setpage=sis_300_setpage;
        __svgalib_sis_driverspecs.cursor=sis_300_cursor;
        cardspecs->maxPixelClock8bpp = 250000;	
        cardspecs->maxPixelClock16bpp = 250000;	
        cardspecs->maxPixelClock32bpp = 250000;
        cardspecs->maxPixelClock24bpp = 0;
        __svgalib_mmio_base=buf[5]&0xfffff000;
        __svgalib_mmio_size=0x10000;
    }

    __svgalib_driverspecs = &__svgalib_sis_driverspecs;
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=sis_linear_base;
    __svgalib_linear_mem_size=sis_memory*0x400;
    return 0;
}

static void
sisCalcClock(int Clock, int max_VLD, unsigned int *vclk)
{
    int M, N, P, PSN, VLD, PSNx;

    int bestM=0, bestN=0, bestP=0, bestPSN=0, bestVLD=0;
    double bestError, abest = 42.0, bestFout;
    double target;

    double Fvco, Fout;
    double error, aerror;

    /*
     *	fd = fref*(Numerator/Denumerator)*(Divider/PostScaler)
     *
     *	M 	= Numerator [1:128] 
     *  N 	= DeNumerator [1:32]
     *  VLD	= Divider (Vco Loop Divider) : divide by 1, 2
     *  P	= Post Scaler : divide by 1, 2, 3, 4
     *  PSN     = Pre Scaler (Reference Divisor Select) 
     * 
     * result in vclk[]
     */
#define Midx 	0
#define Nidx 	1
#define VLDidx 	2
#define Pidx 	3
#define PSNidx 	4
#define Fref 14318180
/* stability constraints for internal VCO -- MAX_VCO also determines 
 * the maximum Video pixel clock */
#define MIN_VCO Fref
#define MAX_VCO 135000000
#define MAX_VCO_5597 353000000
#define MAX_PSN 0 /* no pre scaler for this chip */
#define TOLERANCE 0.01	/* search smallest M and N in this tolerance */
  
  int M_min = 2;
  int M_max = 128;
  
/*  abest=10000.0; */
 
  target = Clock * 1000;
 
     if ((chip == SIS_5597) || (chip == SIS_6326)){
 	int low_N = 2;
 	int high_N = 5;
 	int PSN = 1;
 
 	P = 1;
 	if (target < MAX_VCO_5597 / 2)
 	    P = 2;
 	if (target < MAX_VCO_5597 / 3)
 	    P = 3;
 	if (target < MAX_VCO_5597 / 4)
 	    P = 4;
 	if (target < MAX_VCO_5597 / 6)
 	    P = 6;
 	if (target < MAX_VCO_5597 / 8)
 	    P = 8;
 
 	Fvco = P * target;
 
 	for (N = low_N; N <= high_N; N++){
 	    double M_desired = Fvco / Fref * N;
 	    if (M_desired > M_max * max_VLD)
 		continue;
 
 	    if ( M_desired > M_max ) {
 		M = M_desired / 2 + 0.5;
 		VLD = 2;
 	    } else {
 		M = Fvco / Fref * N + 0.5;
 		VLD = 1;
 	    };
 
 	    Fout = (double)Fref * (M * VLD)/(N * P);
 
 	    error = (target - Fout) / target;
 	    aerror = (error < 0) ? -error : error;
/* 	    if (aerror < abest && abest > TOLERANCE) {*/
 	    if (aerror < abest) {
 	        abest = aerror;
 	        bestError = error;
 	        bestM = M;
 	        bestN = N;
 	        bestP = P;
 	        bestPSN = PSN;
 	        bestVLD = VLD;
 	        bestFout = Fout;
 	    }
 	}
     }
     else {
         for (PSNx = 0; PSNx <= MAX_PSN ; PSNx++) {
 	    int low_N, high_N;
 	    double FrefVLDPSN;
 
 	    PSN = !PSNx ? 1 : 4;
 
 	    low_N = 2;
 	    high_N = 32;
 
 	    for ( VLD = 1 ; VLD <= max_VLD ; VLD++ ) {
 
 	        FrefVLDPSN = (double)Fref * VLD / PSN;
 	        for (N = low_N; N <= high_N; N++) {
 		    double tmp = FrefVLDPSN / N;
 
 		    for (P = 1; P <= 4; P++) {	
 		        double Fvco_desired = target * ( P );
 		        double M_desired = Fvco_desired / tmp;
 
 		        /* Which way will M_desired be rounded?  
 		         *  Do all three just to be safe.  
 		         */
 		        int M_low = M_desired - 1;
 		        int M_hi = M_desired + 1;
 
 		        if (M_hi < M_min || M_low > M_max)
 			    continue;
 
 		        if (M_low < M_min)
 			    M_low = M_min;
 		        if (M_hi > M_max)
 			    M_hi = M_max;
 
 		        for (M = M_low; M <= M_hi; M++) {
 			    Fvco = tmp * M;
 			    if (Fvco <= MIN_VCO)
 			        continue;
 			    if (Fvco > MAX_VCO)
 			        break;
 
 			    Fout = Fvco / ( P );
 
 			    error = (target - Fout) / target;
 			    aerror = (error < 0) ? -error : error;
 			    if (aerror < abest) {
 			        abest = aerror;
 			        bestError = error;
 			        bestM = M;
 			        bestN = N;
 			        bestP = P;
 			        bestPSN = PSN;
 			        bestVLD = VLD;
 			        bestFout = Fout;
 			    }
 		        }
 		    }
 	        }
 	    }
         }
  }
  vclk[Midx]    = bestM;
  vclk[Nidx]    = bestN;
  vclk[VLDidx]  = bestVLD;
  vclk[Pidx]    = bestP;
  vclk[PSNidx]  = bestPSN;
};

static void 
sisClockLoad(int Clock, unsigned char *regs)
 {
    unsigned int 	vclk[5];
    unsigned char 	temp, xr2a, xr2b;

    sisCalcClock(Clock, 2, vclk);

    xr2a = (vclk[Midx] - 1) & 0x7f ;
    xr2a |= ((vclk[VLDidx] == 2 ) ? 1 : 0 ) << 7 ;
    xr2b  = (vclk[Nidx] -1) & 0x1f ;	/* bits [4:0] contain denumerator -MC */
    if (vclk[Pidx] <= 4){
 	    xr2b |= (vclk[Pidx] -1 ) << 5 ; /* postscale 1,2,3,4 */
            temp=regs[XR(0x13)];
 	    temp &= 0xBF;
            regs[XR(0x13)]=temp;
    } else {
	    xr2b |= ((vclk[Pidx] / 2) -1 ) << 5 ;  /* postscale 6,8 */
 
            temp=regs[XR(0x13)];
 	    temp |= 0x40;
            regs[XR(0x13)]=temp;
    };
    xr2b |= 0x80 ;   /* gain for high frequency */
 
    regs[XR(0x2a)]=xr2a;
    regs[XR(0x2b)]=xr2b;

    if (chip == SIS_5597 || chip == SIS_6326) {
        temp=regs[XR(0x23)];
 	if (sis_edo_vram)
 	    temp |= 0x40;
        regs[XR(0x23)]=temp;
    }

    if (chip == SIS_5597 || chip == SIS_6326 || chip == SIS_530) {
 	/*write_xr(0x3B, 0x08 );*/
        if (chip == SIS_5597) {
            temp=regs[XR(0x34)];
 	    if (sis_host_bus)
 	    	temp |= 0x18; 
 	    else temp &= ~0x18;
            regs[XR(0x34)]=temp;

            temp=regs[XR(0x3D)];
 	    if (sis_host_bus)
 	     	  temp &= 0x0F; 
            regs[XR(0x3D)]=temp;
	}
	    
 	/* One-Cycle VRAM */
        temp=regs[XR(0x34)];
 	if (sis_fast_vram)
            regs[XR(0x34)]=temp;
 
 	/* pci burst */
        temp=regs[XR(0x35)];
 	if (sis_pci_burst_on) 
 	    temp |= 0x10;
 	else if (sis_pci_burst_off)
 	    temp &= ~0x10;
        regs[XR(0x35)]=temp;

 	/* pci burst,also */
	if (chip != SIS_530) {
                temp=regs[XR(0x26)];
		if (sis_pci_burst_on) 
			temp |= 0x20;
		else if (sis_pci_burst_off)
			temp &= ~0x20;
                regs[XR(0x26)]=temp;
	}
/* Merge FIFOs */	     
   	temp=regs[XR(0x07)];
        temp |= 0x80;
        regs[XR(0x07)]=temp;
    };

}

static int sisMClk(void)
{ int mclk;
  unsigned char xr28, xr29, xr13, xr10;

    if (chip == SIS_530) {
	/* The MCLK in SiS530/620 is set by BIOS in SR10 */
	read_xr(0x10, xr10);
	switch(xr10 & 0x0f) {
	case 1:
		mclk = 75000; /* 75 Mhz */
		break;
	case 2:
		mclk = 83000; /* 83 Mhz */
		break;
	case 3:
		mclk = 100000;/* 100 Mhz */
		break;
	case 0:
	default:
		mclk = 66000; /* 66 Mhz */
	}
	return(mclk);
    }
    /* Numerator */
    read_xr(0x28,xr28);
    mclk=14318*((xr28 & 0x7f)+1);

    /* Denumerator */
    read_xr(0x29,xr29);
    mclk=mclk/((xr29 & 0x1f)+1);

  /* Divider. Does not seem to work for mclk for older cards */
  if ( (chip==SIS_6326) &&
        ((xr28 & 0x80)!=0 ) ) {
         mclk = mclk*2;
    }
    /* Post-scaler. Values depends on SR13 bit 7  */
    read_xr(0x13,xr13);

    if ( (xr13 & 0x80)==0 ) {
      mclk = mclk / (((xr29 & 0x60) >> 5)+1);
    }
    else {
      /* Values 00 and 01 are reserved */
      if ((xr29 & 0x60) == 0x40) mclk=mclk/6;
      if ((xr29 & 0x60) == 0x60) mclk=mclk/8;
    }

    return(mclk);
}

static void
sis_CPUthreshold(int dotClock,int bpp,int *thresholdLow,int *thresholdHigh)
{
    unsigned char temp;
    int mclk;
    int safetymargin, gap;
    float z, z1, z2; /* Yeou */
    int factor; 
    
    mclk=sisMClk();

    if (chip == SIS_530) /* Yeou for 530 thresholod */
    {
        /* z = f *((dotClock * bpp)/(buswidth*mclk);
           thresholdLow  = (z+1)/2 + 4;
           thresholdHigh = 0x1F;
           
           where f = 0x60 when UMA (SR0D D[0] = 1)
                     0x30      LFB (SR0D D[0] = 0)
        */
        read_xr (0x0d, temp); 
	if (temp & 0x01) factor = 0x60;
        else factor = 0x30;
   
        z1 = (float)(dotClock * bpp); 
        z2 = (float)(64.0 * mclk);
 	z = ((float) factor * (z1 / z2));
        *thresholdLow = ((int)z + 1) / 2 + 4 ;   
        *thresholdHigh = 0x1F; 
    }
    else {   /* For sis other product */
        /* Adjust thresholds. Safetymargin is to be adjusted by fifo_XXX 
           options. Try to mantain a fifo margin of gap. At high Vclk*bpp
           this isn't possible, so limit the thresholds. 
       
           The values I guess are :

           FIFO_CONSERVATIVE : safetymargin = 5 ;
           FIFO_MODERATE     : safetymargin = 3 ;
           Default           : safetymargin = 1 ;  (good enough in many cases) 
           FIFO_AGGRESSIVE   : safetymargin = 0 ;
	 
           gap=4 seems to be the best value in either case...
       */
    
       safetymargin=3; 

       gap = 4;
       *thresholdLow = ((bpp*dotClock) / mclk)+safetymargin;
       *thresholdHigh = ((bpp*dotClock) / mclk)+gap+safetymargin;

       /* 24 bpp seems to need lower FIFO limits. 
          At 16bpp is possible to put a thresholdHigh of 0 (0x10) with 
          good results on my system(good performance, and virtually no noise) */
     
        if ( *thresholdLow > (bpp < 24 ? 0xe:0x0d) ) { 
    	     *thresholdLow = (bpp < 24 ? 0xe:0x0d); 
 	}

        if ( *thresholdHigh > (bpp < 24 ? 0x10:0x0f) ) { 
 	     *thresholdHigh = (bpp < 24 ? 0x10:0x0f);
	}
    }; /* sis530 */
}

static int compute_vclk(
        int Clock,
        int *out_n,
        int *out_dn,
        int *out_div,
        int *out_sbit,
        int *out_scale)
{
    float f,x,y,t, error, min_error;
    int n, dn, best_n=0, best_dn=0;

    /*
     * Rules
     *
     * VCLK = 14.318 * (Divider/Post Scalar) * (Numerator/DeNumerator)
     * Factor = (Divider/Post Scalar)
     * Divider is 1 or 2
     * Post Scalar is 1, 2, 3, 4, 6 or 8
     * Numberator ranged from 1 to 128
     * DeNumerator ranged from 1 to 32
     * a. VCO = VCLK/Factor, suggest range is 150 to 250 Mhz
     * b. Post Scalar selected from 1, 2, 4 or 8 first.
     * c. DeNumerator selected from 2.
     *
     * According to rule a and b, the VCO ranges that can be scaled by
     * rule b are:
     *      150    - 250    (Factor = 1)
     *       75    - 125    (Factor = 2)
     *       37.5  -  62.5  (Factor = 4)
     *       18.75 -  31.25 (Factor = 8)
     *
     * The following ranges use Post Scalar 3 or 6:
     *      125    - 150    (Factor = 1.5)
     *       62.5  -  75    (Factor = 3)
     *       31.25 -  37.5  (Factor = 6)
     *
     * Steps:
     * 1. divide the Clock by 2 until the Clock is less or equal to 31.25.
     * 2. if the divided Clock is range from 18.25 to 31.25, than
     *    the Factor is 1, 2, 4 or 8.
     * 3. if the divided Clock is range from 15.625 to 18.25, than
     *    the Factor is 1.5, 3 or 6.
     * 4. select the Numberator and DeNumberator with minimum deviation.
     *
     * ** this function can select VCLK ranged from 18.75 to 250 Mhz
     */
    f = (float) Clock;
    f /= 1000.0;
    if ((f > 250.0) || (f < 18.75))
        return 0;

    min_error = f;
    y = 1.0;
    x = f;
    while (x > 31.25) {
        y *= 2.0;
        x /= 2.0;
    }
    if (x >= 18.25) {
        x *= 8.0;
        y = 8.0 / y;
    } else if (x >= 15.625) {
        x *= 12.0;
        y = 12.0 / y;
    }

    t = y;
    if (t == (float) 1.5) {
        *out_div = 2;
        t *= 2.0;
    } else {
        *out_div = 1;
    }
    if (t > (float) 4.0) {
        *out_sbit = 1;
        t /= 2.0;
    } else {
        *out_sbit = 0;
    }

    *out_scale = (int) t;

    for (dn=2;dn<=32;dn++) {
        for (n=1;n<=128;n++) {
            error = x;
            error -= ((float) 14.318 * (float) n / (float) dn);
            if (error < (float) 0)
                    error = -error;
            if (error < min_error) {
                min_error = error;
                best_n = n;
                best_dn = dn;
            }
        }
    }
    *out_n = best_n;
    *out_dn = best_dn;
    return 1;
}


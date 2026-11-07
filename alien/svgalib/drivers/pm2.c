/*
Permedia 2 chipset driver 
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
#include "glint_re.h"
#include "pm2io.h"

#define REG_SAVE(i) (VGA_TOTAL_REGS+i)
#define TOTAL_REGS (VGA_TOTAL_REGS + 768 + 4*48)

static int init(int, int, int);
static void unlock(void);
static void lock(void);

static int memory;
static int is_linear, linear_base;

static enum {cPM2=1, cPM2V} chiptype;

static CardSpecs *cardspecs;

static int __svgalib_inlinearmode(void)
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
        if (__svgalib_inlinearmode())
	    modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
    }
}

/* Read and save chipset-specific registers */

static int saveregs(unsigned char regs[])
{ 
    int i;
    unsigned int *p = (unsigned int *)(regs+VGA_TOTAL_REGS);
    unsigned char *q = regs+188;

    unlock();

    p[0]=GLINT_READ_REG(Aperture0);
    p[1]=GLINT_READ_REG(Aperture1);
    p[2]=GLINT_READ_REG(PMFramebufferWriteMask);
    p[3]=GLINT_READ_REG(PMBypassWriteMask);
    p[4]=GLINT_READ_REG(DFIFODis);
    p[5]=GLINT_READ_REG(FIFODis);
    p[6]=GLINT_READ_REG(PMMemConfig);
    p[7]=GLINT_READ_REG(PMHTotal);
    p[8]=GLINT_READ_REG(PMHbEnd);
    p[9]=GLINT_READ_REG(PMHgEnd);
    p[10]=GLINT_READ_REG(PMScreenStride);
    p[24]=GLINT_READ_REG(PMHsStart);
    p[11]=GLINT_READ_REG(PMHsEnd);
    p[12]=GLINT_READ_REG(PMVTotal);
    p[13]=GLINT_READ_REG(PMVbEnd);
    p[14]=GLINT_READ_REG(PMVsStart);
    p[15]=GLINT_READ_REG(PMVsEnd);
    p[16]=GLINT_READ_REG(PMScreenBase);
    p[17]=GLINT_READ_REG(PMVideoControl);
    p[18]=GLINT_READ_REG(VClkCtl);
    p[19]=GLINT_READ_REG(ChipConfig);

    for(i=0;i<768;i++) {
	    Permedia2ReadAddress(i);
	    regs[128+VGA_TOTAL_REGS+i]=Permedia2ReadData();
    }
    
	switch(chiptype) {
		case cPM2:
			p[25]=Permedia2InIndReg(PM2DACIndexMCR);
    		p[26]=Permedia2InIndReg(PM2DACIndexMDCR);
    		p[27]=Permedia2InIndReg(PM2DACIndexCMR);
    		p[20]=Permedia2InIndReg(PM2DACIndexClockAM);
 			p[21]=Permedia2InIndReg(PM2DACIndexClockAN);
   			p[22]=Permedia2InIndReg(PM2DACIndexClockAP);
    		break;
		case cPM2V:
			p[23]=GLINT_READ_REG(PM2VDACRDIndexControl);
			p[25]=Permedia2vInIndReg(PM2VDACRDOverlayKey);
			p[26]=Permedia2vInIndReg(PM2VDACRDSyncControl);
			p[27]=Permedia2vInIndReg(PM2VDACRDMiscControl);
			p[28]=Permedia2vInIndReg(PM2VDACRDDACControl);
			p[29]=Permedia2vInIndReg(PM2VDACRDPixelSize);
			p[30]=Permedia2vInIndReg(PM2VDACRDColorFormat);
			p[20]=Permedia2vInIndReg(PM2VDACRDDClk0PreScale);
			p[21]=Permedia2vInIndReg(PM2VDACRDDClk0FeedbackScale);
			p[22]=Permedia2vInIndReg(PM2VDACRDDClk0PostScale);
                        
                        for(i=0;i<6;i++)
                            q[i]=Permedia2vInIndReg(PM2VDACRDCursorPalette+i);
                        q[6]=Permedia2vInIndReg(PM2VDACRDCursorHotSpotX);
                        q[7]=Permedia2vInIndReg(PM2VDACRDCursorHotSpotY);
                        q[8]=Permedia2vInIndReg(PM2VDACRDCursorXLow);
                        q[9]=Permedia2vInIndReg(PM2VDACRDCursorXHigh);
                        q[10]=Permedia2vInIndReg(PM2VDACRDCursorYLow);
                        q[11]=Permedia2vInIndReg(PM2VDACRDCursorYHigh);
                        q[12]=Permedia2vInIndReg(PM2DACCursorControl);
                        q[13]=Permedia2vInIndReg(PM2VDACRDCursorMode);
			break;
	}
	
    return TOTAL_REGS - VGA_TOTAL_REGS;
}

/* Set chipset-specific registers */

static void setregs(const unsigned char regs[], int mode)
{  
    int i;
    unsigned int *p = (unsigned int *)(regs+VGA_TOTAL_REGS);
    const unsigned char *q = regs+188;

    unlock();

    GLINT_SLOW_WRITE_REG(0xFF, PM2DACReadMask);

    if(chiptype == cPM2V)
    	GLINT_SLOW_WRITE_REG(p[19], ChipConfig);
    GLINT_SLOW_WRITE_REG(p[0], Aperture0);
    GLINT_SLOW_WRITE_REG(p[1], Aperture1);
    GLINT_SLOW_WRITE_REG(p[2], PMFramebufferWriteMask);
    GLINT_SLOW_WRITE_REG(p[3], PMBypassWriteMask);
    GLINT_SLOW_WRITE_REG(p[4], DFIFODis);
    GLINT_SLOW_WRITE_REG(p[5], FIFODis);
    if(0)
    	GLINT_SLOW_WRITE_REG(p[6], PMMemConfig);
    GLINT_SLOW_WRITE_REG(p[17], PMVideoControl);
    GLINT_SLOW_WRITE_REG(p[9], PMHgEnd);
    GLINT_SLOW_WRITE_REG(p[16], PMScreenBase);
    GLINT_SLOW_WRITE_REG(p[18], VClkCtl);
    GLINT_SLOW_WRITE_REG(p[10], PMScreenStride);
    GLINT_SLOW_WRITE_REG(p[7], PMHTotal);
    GLINT_SLOW_WRITE_REG(p[8], PMHbEnd);
    GLINT_SLOW_WRITE_REG(p[24], PMHsStart);
    GLINT_SLOW_WRITE_REG(p[11], PMHsEnd);
    GLINT_SLOW_WRITE_REG(p[12], PMVTotal);
    GLINT_SLOW_WRITE_REG(p[13], PMVbEnd);
    GLINT_SLOW_WRITE_REG(p[14], PMVsStart);
    GLINT_SLOW_WRITE_REG(p[15], PMVsEnd);
    if(chiptype != cPM2V)
    	GLINT_SLOW_WRITE_REG(p[19], ChipConfig);
    
    for (i=0;i<768;i++) {
	    Permedia2WriteAddress(i);
	    Permedia2WriteData(regs[VGA_TOTAL_REGS + 128 + i]);
    }
		
	switch(chiptype) {
		case cPM2:
                    Permedia2OutIndReg(PM2DACIndexMCR, 0x00, p[25]);
    		    Permedia2OutIndReg(PM2DACIndexMDCR, 0x00, p[26]);
    		    Permedia2OutIndReg(PM2DACIndexCMR, 0x00, p[27]);
    		    Permedia2OutIndReg(PM2DACIndexClockAM, 0x00, p[20]);
    		    Permedia2OutIndReg(PM2DACIndexClockAN, 0x00, p[21]);
    		    Permedia2OutIndReg(PM2DACIndexClockAP, 0x00, p[22]);
                    break;
		case cPM2V:
		    GLINT_SLOW_WRITE_REG(p[23],PM2VDACRDIndexControl);
		    Permedia2vOutIndReg(PM2VDACRDOverlayKey, 0x00, p[25]);
		    Permedia2vOutIndReg(PM2VDACRDSyncControl, 0x00, p[26]);
		    Permedia2vOutIndReg(PM2VDACRDMiscControl, 0x00, p[27]);
		    Permedia2vOutIndReg(PM2VDACRDDACControl, 0x00, p[28]);
		    Permedia2vOutIndReg(PM2VDACRDPixelSize, 0x00, p[29]);
		    Permedia2vOutIndReg(PM2VDACRDColorFormat, 0x00, p[30]);
		    i = Permedia2vInIndReg(PM2VDACIndexClockControl) & 0xFC;
		    Permedia2vOutIndReg(PM2VDACRDDClk0PreScale, 0x00, p[20]);
		    Permedia2vOutIndReg(PM2VDACRDDClk0FeedbackScale, 0x00, p[21]);
		    Permedia2vOutIndReg(PM2VDACRDDClk0PostScale, 0x00, p[22]);
		    Permedia2vOutIndReg(PM2VDACIndexClockControl, 0x00, i | 0x03);
                    for(i=0;i<6;i++)
                        Permedia2vOutIndReg(PM2VDACRDCursorPalette+i, 0x00, q[i]);
                    Permedia2vOutIndReg(PM2VDACRDCursorHotSpotX, 0x00, q[6]);
                    Permedia2vOutIndReg(PM2VDACRDCursorHotSpotY, 0x00, q[7]);
                    Permedia2vOutIndReg(PM2VDACRDCursorXLow, 0x00, q[8]);
                    Permedia2vOutIndReg(PM2VDACRDCursorXHigh, 0x00, q[9]);
                    Permedia2vOutIndReg(PM2VDACRDCursorYLow, 0x00, q[10]);
                    Permedia2vOutIndReg(PM2VDACRDCursorYHigh, 0x00, q[11]);
                    Permedia2vOutIndReg(PM2DACCursorControl, 0x00, q[12]);
                    Permedia2vOutIndReg(PM2VDACRDCursorMode, 0x00, q[13]);
	}

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

    if(modeinfo->bitsPerPixel==24) {
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

#define INITIALFREQERR 100000
#define MINCLK 110000       /* VCO frequency range */
#define MAXCLK 250000

static unsigned long
PM2DAC_CalculateMNPCForClock
(
 unsigned long reqclock,        /* In kHz units */
 unsigned long refclock,        /* In kHz units */
 unsigned char *rm,             /* M Out */
 unsigned char *rn,             /* N Out */
 unsigned char *rp          /* P Out */
 )
{
    unsigned char   m, n, p;
    unsigned long   f;
    long        freqerr, lowestfreqerr = INITIALFREQERR;
    unsigned long   clock,actualclock = 0;

    for (n = 2; n <= 14; n++) {
        for (m = 2; m != 0; m++) { /* this is a char, so this counts to 255 */
            f = refclock * m / n;
            if ( (f < MINCLK) || (f > MAXCLK) )
                continue;
            for (p = 0; p <= 4; p++) {
                clock = f >> p;
                freqerr = reqclock - clock;
                if (freqerr < 0)
                    freqerr = -freqerr;
                if (freqerr < lowestfreqerr) {
                    *rn = n;
                    *rm = m;
                    *rp = p;
                    lowestfreqerr = freqerr;
                    actualclock = clock;
                }
            }
        }
    }

    return(actualclock);
}

static unsigned long
PM2VDAC_CalculateClock
(
 unsigned long reqclock,        /* In kHz units */
 unsigned long refclock,        /* In kHz units */
 unsigned char *prescale,       /* ClkPreScale */
 unsigned char *feedback,       /* ClkFeedBackScale */
 unsigned char *postscale       /* ClkPostScale */
 )
{
    int         f, pre, post;
    unsigned long   freq;
    long        freqerr = 1000;
    unsigned long   actualclock = 0;

    for (f=1;f<256;f++) {
    for (pre=1;pre<256;pre++) {
        for (post=0;post<1;post++) {
            freq = ((refclock * f) / pre);
        if ((reqclock > freq - freqerr)&&(reqclock < freq + freqerr)){
            freqerr = (reqclock > freq) ?
                    reqclock - freq : freq - reqclock;
            *feedback = f;
            *prescale = pre;
            *postscale = post;
            actualclock = freq;
        }
        }
    }
    }

    return(actualclock);
}


/* Local, called by setmode(). */
static void initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{ /* long k; */
    unsigned int *p = (unsigned int *)(moderegs+VGA_TOTAL_REGS);
    int t1, t2, t3, t4;
    unsigned char m, n, r;
    
    p[0]=0;
    p[1]=0;
    p[2]=0xffffffff;
    p[3]=0xffffffff;
    p[4]=1;
    p[5]=1;
    
    if(0) p[6]=GLINT_READ_REG(PMMemConfig) | (1<<21);
  
    t1 = modetiming->CrtcHSyncStart - modetiming->CrtcHDisplay;
    t2 = modetiming->CrtcVSyncStart - modetiming->CrtcVDisplay;
    t3 = modetiming->CrtcHSyncEnd - modetiming->CrtcHSyncStart;
    t4 = modetiming->CrtcVSyncEnd - modetiming->CrtcVSyncStart;
   
    switch(modeinfo->bytesPerPixel) {
        case 1:
            p[7]=modetiming->CrtcHTotal >> 2;
            p[11]=(t1+t3) >> 2;
            p[24]=t1 >> 2;
            p[8]=(modetiming->CrtcHTotal-modetiming->CrtcHDisplay) >>2;
            p[10]=modeinfo->lineWidth >> 3;
            break;
        case 2:
            p[7]=modetiming->CrtcHTotal >> 1;
            p[11]=(t1+t3) >> 1;
            p[24]=t1 >> 1;
            p[8]=(modetiming->CrtcHTotal-modetiming->CrtcHDisplay) >>1;
            p[10]=modeinfo->lineWidth >> 3;
            break;
        case 3:
            p[7]=(3*modetiming->CrtcHTotal) >> 2;
            p[11]=(3*(t1+t3)) >> 2;
            p[24]=(3*t1) >> 2;
            p[8]=(3*(modetiming->CrtcHTotal-modetiming->CrtcHDisplay)) >> 2;
            p[10]=modeinfo->lineWidth >> 3;
	    break;
        case 4:
            p[7]=modetiming->CrtcHTotal >> 0;
            p[11]=(t1+t3) >> 0;
            p[24]=t1 >> 0;
            p[8]=(modetiming->CrtcHTotal-modetiming->CrtcHDisplay) >>0;
            p[10]=modeinfo->lineWidth >> 3;
	    break;
    }

    p[12]= modetiming->CrtcVTotal;
    p[15]= t2+t4;
    p[14]= t2;
    p[13]= modetiming->CrtcVTotal-modetiming->CrtcVDisplay;

    p[17]=(1<<5)|(1<<3)|1;

    if (modetiming->flags & DOUBLESCAN) {
        p[17] |= (1<<2);
    }
    
    if((modeinfo->bytesPerPixel>1)||(chiptype==cPM2V)) {
        p[17]|= 1<<16;
        p[7]>>=1;
        p[11]>>=1;
        p[24]>>=1;
        p[8]>>=1;
    }
    
    p[18]=GLINT_READ_REG(VClkCtl)&0xfffffffc;
    p[16]=0;
    p[7] -= 1;
    p[12] -= 1;
    p[24] -= 1;
    p[19]=GLINT_READ_REG(ChipConfig)&0xffffffdd;
    
    switch(chiptype) {
	case cPM2:
            PM2DAC_CalculateMNPCForClock(modetiming->pixelClock, 14318, &m, &n, &r);
    	    p[20]=m;
    	    p[21]=n;
    	    p[22]=r|8;
            break;
	case cPM2V:
            if(modetiming->pixelClock>28635) {
	           PM2VDAC_CalculateClock(modetiming->pixelClock, 14318, &m, &n, &r);
                   p[22]=0;
            } else {
	           PM2VDAC_CalculateClock(modetiming->pixelClock*2, 14318, &m, &n, &r);
                   p[22]=1;
            }
	    p[20]=m;
	    p[21]=n;
	    break;
    };
	
    if(chiptype == cPM2V) {
        p[23]=0;
        p[26]=0;
        p[28]=0;
        p[25]=0; /* ??? */
        
        if(!(modetiming->flags&PHSYNC)) p[26] |= 1;
        if(!(modetiming->flags&PVSYNC)) p[26] |= 8;
        switch(modeinfo->bytesPerPixel) {
            case 1:
                p[29]=0;
                p[30]=0x2e;
                p[27]=0;
                break;
            case 2:
                p[29]=1;
                p[27]=0x09;
                if(modeinfo->colorBits==15) {
                    p[30] |= 0x61;
                } else {
                    p[30] |= 0x70;
                }
		break;
            case 3:
                p[29]=4;
                p[27]=9;
                p[30]=0x60;
                break;
            case 4:
                p[29]=2;
                p[27]=9;
                p[30]=0x20;
                break;
        }    
    } else {
        p[25]=0;    
        switch(modeinfo->bytesPerPixel) {
            case 1:
                p[25]=2;
                p[27] = PM2DAC_RGB | PM2DAC_GRAPHICS | PM2DAC_CI8 ;
                break;
            case 2:
                p[27] = PM2DAC_RGB | PM2DAC_GRAPHICS | PM2DAC_TRUECOLOR ;
                if(modeinfo->colorBits==15) {
                    p[27] |= PM2DAC_5551;
                } else {
                    p[27] |= PM2DAC_565;
                }
                break;
            case 3:
                p[27] = PM2DAC_RGB | PM2DAC_GRAPHICS | PM2DAC_TRUECOLOR | PM2DAC_PACKED;
	        break;
            case 4:
                p[27] = PM2DAC_RGB | PM2DAC_GRAPHICS | PM2DAC_TRUECOLOR | PM2DAC_8888;
	        break;
        }

        if(!(modetiming->flags&PHSYNC)) p[25] |= 4;
        if(!(modetiming->flags&PVSYNC)) p[25] |= 8;

    }
	
    p[9]=p[8];
    
    return ;
}


static int local_setmode(int mode, int prv_mode)
{
    unsigned char *moderegs;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {

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

    setregs(moderegs, mode);		/* Set extended regs. */
    free(moderegs);

    free(modeinfo);
    return 0;
}


/* Unlock chipset-specific registers */

static void unlock(void)
{
}

static void lock(void)
{
}

#define VENDOR_ID 0x3d3d

/* Indentify chipset, initialize and return non-zero if detected */

static int test(void)
{
    unsigned long buf[64];
    int found, id;
 
    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    id=(buf[0]>>16)&0xffff;
    if(found)return 0;
    switch(id) {
        case 0x0007:
        case 0x0009:
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
    GLINT_SLOW_WRITE_REG(address>>3, PMScreenBase);
}


/* Set logical scanline length (usually multiple of 8) */
static void setlogicalwidth(int width)
{   
    GLINT_SLOW_WRITE_REG(width>>3, PMScreenStride);
}

static int linear(int op, int param)
{
    if (op==LINEAR_ENABLE){is_linear=1; return 0;};
    if (op==LINEAR_DISABLE){is_linear=0; return 0;};
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

unsigned char *cursors[16];
unsigned int cur_cols[32];

static int pm2v_cursor( int cmd, int p1, int p2, int p3, int p4, void *p5) {
    int i, j;
    unsigned long *b3;
    unsigned long l1, l2, k, c, l;
    
    switch(cmd){
        case CURSOR_INIT:
            cursors[0]=malloc(16*1024);
            for(i=1;i<16;i++)cursors[i]=cursors[0]+(i*1024);
            return 1;
        case CURSOR_HIDE:
            Permedia2vOutIndReg(PM2VDACRDCursorMode, 0x00, 0x10);
            break;
        case CURSOR_SHOW:
            Permedia2vOutIndReg(PM2VDACRDCursorMode, 0x00, 0x11);
            break;
        case CURSOR_POSITION:
            p1+=64;
            p2+=64;
            Permedia2vOutIndReg(PM2VDACRDCursorHotSpotX, 0x00, 0x3f);
            Permedia2vOutIndReg(PM2VDACRDCursorHotSpotY, 0x00, 0x3f);
            Permedia2vOutIndReg(PM2VDACRDCursorXLow, 0x00, p1&0xff);
            Permedia2vOutIndReg(PM2VDACRDCursorXHigh, 0x00, p1>>8);
            Permedia2vOutIndReg(PM2VDACRDCursorYLow, 0x00, p2&0xff);
            Permedia2vOutIndReg(PM2VDACRDCursorYHigh, 0x00, p2>>8);
        break;
        case CURSOR_SELECT:
            for(i=0;i<1024;i++)Permedia2vOutIndReg(PM2VDACRDCursorPattern+i, 0x00, cursors[p1][i]);
            Permedia2vOutIndReg(PM2VDACRDCursorPalette, 0x00, cur_cols[p1*2]>>16);
            Permedia2vOutIndReg(PM2VDACRDCursorPalette+1, 0x00, cur_cols[p1*2]>>8);
            Permedia2vOutIndReg(PM2VDACRDCursorPalette+2, 0x00, cur_cols[p1*2]);
            Permedia2vOutIndReg(PM2VDACRDCursorPalette+3, 0x00, cur_cols[p1*2+1]>>16);
            Permedia2vOutIndReg(PM2VDACRDCursorPalette+4, 0x00, cur_cols[p1*2+1]>>8);
            Permedia2vOutIndReg(PM2VDACRDCursorPalette+5, 0x00, cur_cols[p1*2+1]);
            break;
        case CURSOR_IMAGE:
            b3=(unsigned long *)p5;
            switch(p2) {
                case 0:
                    cur_cols[p1*2]=p3;
                    cur_cols[p1*2+1]=p4;
                    for(j=0;j<32;j++) {
                        l1=*(b3+j);
                        l2=*(b3+32+j);
                        for(k=0;k<8;k++) {
                            c=0;
                            for(l=0;l<4;l++) {
                                c>>=1;
                                c|=(l1>>31)<<7;
                                c>>=1;
                                c|=(l2>>31)<<7;
                                l1<<=1;
                                l2<<=1;
                            }
                            *(cursors[p1]+16*j+k)=c;
                            *(cursors[p1]+16*j+k+8)=0;
                        }
                    }
                    memset(cursors[p1]+512,0,512);
                break;
            }
            break;
    }
    return 0;
}       


/* Function table (exported) */
DriverSpecs __svgalib_pm2_driverspecs =
{
    saveregs,
    setregs,
    unlock,
    lock,
    test,
    init,
    __svgalib_emul_setpage,
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
};

/* Initialize chipset (called after detection) */

static int init(int force, int par1, int par2)
{
    unsigned long buf[64];
    int found=0, id;
    unsigned int mmio_base;

    unlock();
    if (force) {
	memory = par1;
        chiptype = par2;
    } else {

    };

    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    linear_base=0;
    id=(buf[0]>>16)&0xffff;
    if(!found) {
        switch(id) {
            case 0x0007:
				chiptype = cPM2;
				break;
            case 0x0009:
				chiptype = cPM2V;
                break;
            default:
                return 1;
        }
    }

    linear_base = buf[6] & 0xffffc000;
    mmio_base = buf[4] & 0xffffc000;
    __svgalib_mmio_base=mmio_base;
    __svgalib_mmio_size=64*1024;
    map_mmio();
    
    memory = (((GLINT_READ_REG(PMMemConfig) >> 29) & 0x03) + 1) * 2048;

    switch(chiptype) {
        case cPM2V:
            __svgalib_pm2_driverspecs.cursor = pm2v_cursor;
            break;
        default:
			break;
    }
    
    if (__svgalib_driver_report) {
	fprintf(stderr,"Using PM2 driver, %iKB.\n",memory);
    };
    
    pm2_mapio();
    
	__svgalib_modeinfo_linearset |= IS_LINEAR;
	
    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = memory;
    cardspecs->maxPixelClock4bpp = 75000;	
    cardspecs->maxPixelClock8bpp = 230000;	
    cardspecs->maxPixelClock16bpp = 230000;	
    cardspecs->maxPixelClock24bpp = 150000;
    cardspecs->maxPixelClock32bpp = 110000;
    cardspecs->flags = INTERLACE_DIVIDE_VERT | CLOCK_PROGRAMMABLE | EMULATE_BANK;
    cardspecs->maxHorizontalCrtc = 4080;
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->nClocks =0;
    cardspecs->mapClock = map_clock;
    cardspecs->mapHorizontalCrtc = map_horizontal_crtc;
    cardspecs->matchProgrammableClock=match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_pm2_driverspecs;
    __svgalib_banked_mem_base=linear_base;
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=linear_base;
    __svgalib_linear_mem_size=memory*0x400;
    return 0;
}

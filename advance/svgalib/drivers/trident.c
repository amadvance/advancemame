/*
Trident PCI driver 
Only tested on 9685
*/

#include <stdlib.h>
#include <stdio.h>		
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"
#include "timing.h"
#include "vgaregs.h"
#include "interfac.h"
#include "vgapci.h"
#include "trident.h"
#include "vgammvga.h"
#include "endianes.h"

#define TOTAL_REGS (VGA_TOTAL_REGS + 500)

static int init(int, int, int);
static void unlock(void);
static void lock(void);

static int memory, chip, NewClockCode;
static float frequency;
static int is_linear, linear_base, mmio_base;

static CardSpecs *cardspecs;

static void setpage(int page)
{
    OUTB(0x3D8, page);
    OUTB(0x3D9, page);
}

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
	    modeinfo->flags |= IS_LINEAR;
    }
}

/* Read and save chipset-specific registers */

static int saveregs(unsigned char regs[])
{ 
    int temp;
    
    TRIDENTRegPtr tridentReg;
    
    tridentReg = (TRIDENTRegPtr)(regs+60);

    unlock();		

    /* Goto New Mode */
    OUTB(0x3C4, 0x0B);
    temp = INB(0x3C5);

    INB_3C4(NewMode1);
    if (chip > PROVIDIA9685)
    	INB_3C4(Protection);
    
    /* Unprotect registers */
    OUTW(0x3C4, ((0xC0 ^ 0x02) << 8) | NewMode1);
    OUTW(0x3D4, (0x92 << 8) | NewMode1);

    INB_3x4(Offset);
    INB_3x4(LinearAddReg);
    INB_3x4(CRTCModuleTest);
    INB_3x4(CRTHiOrd);
    INB_3x4(Performance);
    INB_3x4(InterfaceSel);
    INB_3x4(DRAMControl);
    INB_3x4(AddColReg);
    INB_3x4(PixelBusReg);
    INB_3x4(GraphEngReg);
    INB_3x4(PCIReg);
    INB_3x4(PCIRetry);
    INB_3C4(SSetup);
    INB_3C4(SKey);
    INB_3C4(SPKey);
    INB_3x4(PreEndControl);
    INB_3x4(PreEndFetch);
    if (chip >= PROVIDIA9685) INB_3x4(Enhancement0);
    if (chip >= BLADE3D)      INB_3x4(RAMDACTiming);
    if (chip == CYBERBLADEE4) INB_3x4(New32);
#if 0
    if (pTrident->IsCyber) {
	CARD8 tmp;
	INB_3CE(VertStretch);
	INB_3CE(HorStretch);
	INB_3CE(BiosMode);
	INB_3CE(BiosReg);	
    	INB_3CE(CyberControl);
    	INB_3CE(CyberEnhance);
	SHADOW_ENABLE(tmp);
	INB_3x4(0x0);
	INB_3x4(0x3);
	INB_3x4(0x4);
	INB_3x4(0x5);
	INB_3x4(0x6);
	INB_3x4(0x7);
	INB_3x4(0x10);
	INB_3x4(0x11);
	INB_3x4(0x16);
	SHADOW_RESTORE(tmp);
    }
#endif
    /* save cursor registers */
    INB_3x4(CursorControl);

    INB_3CE(MiscExtFunc);
    INB_3CE(MiscIntContReg);

    temp = INB(0x3C8);
    temp = INB(0x3C6);
    temp = INB(0x3C6);
    temp = INB(0x3C6);
    temp = INB(0x3C6);
    tridentReg->tridentRegsDAC[0x00] = INB(0x3C6);
    temp = INB(0x3C8);

    tridentReg->tridentRegsClock[0x00] = INB(0x3CC);
    if (Is3Dchip) {
	OUTB(0x3C4, ClockLow);
	tridentReg->tridentRegsClock[0x01] = INB(0x3C5);
	OUTB(0x3C4, ClockHigh);
	tridentReg->tridentRegsClock[0x02] = INB(0x3C5);
#if 0
	if (pTrident->MCLK > 0) {
	    OUTB(0x3C4, MCLKLow);
	    tridentReg->tridentRegsClock[0x03] = INB(0x3C5);
	    OUTB(0x3C4, MCLKHigh);
	    tridentReg->tridentRegsClock[0x04] = INB(0x3C5);
	}
#endif
    } else {
	tridentReg->tridentRegsClock[0x01] = INB(0x43C8);
	tridentReg->tridentRegsClock[0x02] = INB(0x43C9);
#if 0
	if (pTrident->MCLK > 0) {
	    tridentReg->tridentRegsClock[0x03] = INB(0x43C6);
	    tridentReg->tridentRegsClock[0x04] = INB(0x43C7);
	}
#endif
    }

    INB_3C4(NewMode2);

    /* Protect registers */
    OUTW_3C4(NewMode1);
    return TOTAL_REGS - VGA_TOTAL_REGS;
}

/* Set chipset-specific registers */

static void setregs(const unsigned char regs[], int mode)
{  
    int temp;
    TRIDENTRegPtr tridentReg;
    
    tridentReg = (TRIDENTRegPtr)(regs+60);

    unlock();		
    if (chip > PROVIDIA9685) {
    	OUTB(0x3C4, Protection);
    	OUTB(0x3C5, 0x92);
    }
    /* Goto New Mode */
    OUTB(0x3C4, 0x0B);
    temp = INB(0x3C5);

    /* Unprotect registers */
    OUTW(0x3C4, ((0xC0 ^ 0x02) << 8) | NewMode1);
    
    temp = INB(0x3C8);
    temp = INB(0x3C6);
    temp = INB(0x3C6);
    temp = INB(0x3C6);
    temp = INB(0x3C6);
    OUTB(0x3C6, tridentReg->tridentRegsDAC[0x00]);
    temp = INB(0x3C8);

    OUTW_3x4(CRTCModuleTest);
    OUTW_3x4(LinearAddReg);
    OUTW_3C4(NewMode2);
    OUTW_3x4(CursorControl);
    OUTW_3x4(CRTHiOrd);
    OUTW_3x4(AddColReg);
    OUTW_3x4(GraphEngReg);
    OUTW_3x4(Performance);
    OUTW_3x4(InterfaceSel);
    OUTW_3x4(DRAMControl);
    OUTW_3x4(PixelBusReg);
    OUTW_3x4(PCIReg);
    OUTW_3x4(PCIRetry);
    OUTW_3CE(MiscIntContReg);
    OUTW_3CE(MiscExtFunc);
    OUTW_3x4(Offset);
    OUTW_3C4(SSetup);
    OUTW_3C4(SKey);
    OUTW_3C4(SPKey);
    OUTW_3x4(PreEndControl);
    OUTW_3x4(PreEndFetch);
    if (chip >= PROVIDIA9685) OUTW_3x4(Enhancement0);
    if (chip >= BLADE3D)      OUTW_3x4(RAMDACTiming);
    if (chip == CYBERBLADEE4) OUTW_3x4(New32);
#if 0
    if (pTrident->IsCyber) {
	CARD8 tmp;

	OUTW_3CE(VertStretch);
	OUTW_3CE(HorStretch);
	OUTW_3CE(BiosMode);
	OUTW_3CE(BiosReg);	
    	OUTW_3CE(CyberControl);
    	OUTW_3CE(CyberEnhance);
	SHADOW_ENABLE(tmp);
	OUTW_3x4(0x0);
	OUTW_3x4(0x3);
	OUTW_3x4(0x4);
	OUTW_3x4(0x5);
	OUTW_3x4(0x6);
	OUTW_3x4(0x7);	
	OUTW_3x4(0x10);
	OUTW_3x4(0x11); 
	OUTW_3x4(0x16);
	SHADOW_RESTORE(tmp);
    }
#endif
    
    if (Is3Dchip) {
	{
	    OUTW(0x3C4, (tridentReg->tridentRegsClock[0x01])<<8 | ClockLow);
	    OUTW(0x3C4, (tridentReg->tridentRegsClock[0x02])<<8 | ClockHigh);
	}
#if 0
	if (pTrident->MCLK > 0) {
	    OUTW(0x3C4,(tridentReg->tridentRegsClock[0x03])<<8 | MCLKLow);
	    OUTW(0x3C4,(tridentReg->tridentRegsClock[0x04])<<8 | MCLKHigh);
	}
#endif
    } else {
	{
	    OUTB(0x43C8, tridentReg->tridentRegsClock[0x01]);
	    OUTB(0x43C9, tridentReg->tridentRegsClock[0x02]);
	}
#if 0
	if (pTrident->MCLK > 0) {
	    OUTB(0x43C6, tridentReg->tridentRegsClock[0x03]);
	    OUTB(0x43C7, tridentReg->tridentRegsClock[0x04]);
	}
#endif
    }
#ifdef READOUT
    if (!pTrident->DontSetClock)
#endif
    {
	OUTB(0x3C2, tridentReg->tridentRegsClock[0x00]);
    }
    
    if (chip > PROVIDIA9685) {
    	OUTB(0x3C4, Protection);
    	OUTB(0x3C5, tridentReg->tridentRegs3C4[Protection]);
    }

    OUTW(0x3C4, ((tridentReg->tridentRegs3C4[NewMode1] ^ 0x02) << 8)| NewMode1);

}


/* Return nonzero if mode is available */

static int modeavailable(int mode)
{
    struct info *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];
    if (memory * 1024 < info->ydim * info->xbytes)
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

static void TGUISetClock(int clock, CARD8 *a, CARD8 *b)
{
	int powerup[4] = { 1,2,4,8 };
	int clock_diff = 750;
	int freq, ffreq;
	int m, n, k;
	int p, q, r, s; 
	int endn, endm, endk, startk=0;

	p = q = r = s = 0;

	if (NewClockCode)
	{
		endn = 255;
		endm = 63;
		endk = 2;
		if (clock >= 100000) startk = 0;
		if (clock < 100000) startk = 1;
		if (clock < 50000) startk = 2;
	}
	else
	{
		endn = 121;
		endm = 31;
		endk = 1;
		if(clock>50000) startk=1; else startk = 0;
	}

 	freq = clock;

	for (k=startk;k<=endk;k++)
	  for (n=0;n<=endn;n++)
	    for (m=1;m<=endm;m++)
	    {
		ffreq = ( ( ((n + 8) * frequency) / ((m + 2) * powerup[k]) ) * 1000);
		if ((ffreq > freq - clock_diff) && (ffreq < freq + clock_diff)) 
		{
/*
 * It seems that the 9440 docs have this STRICT limitation, although
 * most 9440 boards seem to cope. 96xx/Cyber chips don't need this limit
 * so, I'm gonna remove it and it allows lower clocks < 25.175 too !
 */
#ifdef STRICT
			if ( (n+8)*100/(m+2) < 978 && (n+8)*100/(m+2) > 349 ) {
#endif
				clock_diff = (freq > ffreq) ? freq - ffreq : ffreq - freq;
				p = n; q = m; r = k; s = ffreq;
#ifdef STRICT
			}
#endif
		}
	    }

	if (s == 0)
	{
            exit(1);
        }

	if (NewClockCode)
	{
		/* N is all 8bits */
		*a = p;
		/* M is first 6bits, with K last 2bits */
		*b = (q & 0x3F) | (r << 6);
	}
	else
	{
		/* N is first 7bits, first M bit is 8th bit */
		*a = ((1 & q) << 7) | p;
		/* first 4bits are rest of M, 1bit for K value */
		*b = (((q & 0xFE) >> 1) | (r << 4));
	}
}

static void initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{ /* long k; */

    int offset=0, protect, tmp;
    int clock;
    
    TRIDENTRegPtr pReg;
    
    pReg = (TRIDENTRegPtr)(moderegs+60);
   
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

    clock=modetiming->pixelClock;

    OUTB(0x3C4, 0x11);
    protect = INB(0x3C4);
    OUTB(0x3C5, 0x92);

    OUTB(0x3C4, 0x0B); tmp=INB(0x3C5); /* Ensure we are in New Mode */

    pReg->tridentRegs3x4[PixelBusReg] = 0x00;
    pReg->tridentRegsDAC[0x00] = 0x00;
    pReg->tridentRegs3C4[NewMode2] = 0x20;
    OUTB(0x3CE, MiscExtFunc);
    pReg->tridentRegs3CE[MiscExtFunc] = INB(0x3CF) & 0xF0;
    pReg->tridentRegs3x4[GraphEngReg] = 0x00; 
    pReg->tridentRegs3x4[PreEndControl] = 0;
    pReg->tridentRegs3x4[PreEndFetch] = 0;

    pReg->tridentRegs3x4[CRTHiOrd] = (((modetiming->CrtcVSyncEnd-1) & 0x400)>>4) |
 				     (((modetiming->CrtcVTotal - 2) & 0x400) >> 3) |
 				     ((modetiming->CrtcVSyncStart & 0x400) >> 5) |
 				     (((modetiming->CrtcVDisplay - 1) & 0x400) >> 6)|
 				     0x08;

#if 0
    if (pTrident->IsCyber) {
	Bool LCDActive;
#ifdef READOUT
	Bool ShadowModeActive;
#endif
	int i = pTrident->lcdMode;
#ifdef READOUT
	OUTB(0x3CE, CyberControl);
	ShadowModeActive = ((INB(0x3CF) & 0x81) == 0x81);
#endif
	OUTB(0x3CE, FPConfig);
	LCDActive = (INB(0x3CF) & 0x10);
	
	OUTB(0x3CE, CyberEnhance); 
	pReg->tridentRegs3CE[CyberEnhance] = INB(0x3CF) & 0x8F;
	if (mode->CrtcVDisplay > 768)
	    pReg->tridentRegs3CE[CyberEnhance] |= 0x30;
	else
	if (mode->CrtcVDisplay > 600)
	    pReg->tridentRegs3CE[CyberEnhance] |= 0x20;
	else
	if (mode->CrtcVDisplay > 480)
	    pReg->tridentRegs3CE[CyberEnhance] |= 0x10;

	OUTB(0x3CE, CyberControl);
	pReg->tridentRegs3CE[CyberControl] = INB(0x3CF);

	OUTB(0x3CE,HorStretch);
	pReg->tridentRegs3CE[HorStretch] = INB(0x3CF);
	OUTB(0x3CE,VertStretch);
	pReg->tridentRegs3CE[VertStretch] = INB(0x3CF);

#ifdef READOUT
	if ((!((pReg->tridentRegs3CE[VertStretch] & 1) ||
	       (pReg->tridentRegs3CE[HorStretch] & 1)))
	    && (!LCDActive || ShadowModeActive)) 
	  {
	    unsigned char tmp;
	    
	    SHADOW_ENABLE(tmp);
	    OUTB(vgaIOBase + 4,0);
	    pReg->tridentRegs3x4[0x0] = INB(vgaIOBase + 5);
	    OUTB(vgaIOBase + 4,3);
	    pReg->tridentRegs3x4[0x3] = INB(vgaIOBase + 5);
	    OUTB(vgaIOBase + 4,4);
	    pReg->tridentRegs3x4[0x4] = INB(vgaIOBase + 5);
	    OUTB(vgaIOBase + 4,5);
  	    pReg->tridentRegs3x4[0x5] = INB(vgaIOBase + 5);
  	    OUTB(vgaIOBase + 4,0x6);
  	    pReg->tridentRegs3x4[0x6] = INB(vgaIOBase + 5);
  	    SHADOW_RESTORE(tmp);
 	} else
#endif
 	{
 	    if (i != 0xff) {
  		pReg->tridentRegs3x4[0x0] = LCD[i].shadow_0;
  		pReg->tridentRegs3x4[0x3] = LCD[i].shadow_3;
  		pReg->tridentRegs3x4[0x4] = LCD[i].shadow_4;
  		pReg->tridentRegs3x4[0x5] = LCD[i].shadow_5;
  		pReg->tridentRegs3x4[0x6] = LCD[i].shadow_6;
 		xf86DrvMsgVerb(pScrn->scrnIndex,X_INFO,1,
 			       "Overriding Horizontal timings.\n");
  	    }
  	}
 
 	if (i != 0xff) {
 	    pReg->tridentRegs3x4[0x7] = LCD[i].shadow_7;
 	    pReg->tridentRegs3x4[0x10] = LCD[i].shadow_10;
 	    pReg->tridentRegs3x4[0x11] = LCD[i].shadow_11;
 	    pReg->tridentRegs3x4[0x16] = LCD[i].shadow_16;
 	    if (LCDActive) 
 		pReg->tridentRegs3x4[CRTHiOrd] = LCD[i].shadow_HiOrd;

	    fullSize = (pScrn->currentMode->HDisplay == LCD[i].display_x) 
	        && (pScrn->currentMode->VDisplay == LCD[i].display_y);
 	}
 	
  	/* copy over common bits from normal VGA */
  	
  	pReg->tridentRegs3x4[0x7] &= ~0x4A;
	pReg->tridentRegs3x4[0x7] |= (vgaReg->CRTC[0x7] & 0x4A);

	if (LCDActive && fullSize) {	
	    regp->CRTC[0] = pReg->tridentRegs3x4[0];
	    regp->CRTC[3] = pReg->tridentRegs3x4[3];
	    regp->CRTC[4] = pReg->tridentRegs3x4[4];
	    regp->CRTC[5] = pReg->tridentRegs3x4[5];
	    regp->CRTC[6] = pReg->tridentRegs3x4[6];
	    regp->CRTC[7] = pReg->tridentRegs3x4[7];
	    regp->CRTC[0x10] = pReg->tridentRegs3x4[0x10];
	    regp->CRTC[0x11] = pReg->tridentRegs3x4[0x11];
	    regp->CRTC[0x16] = pReg->tridentRegs3x4[0x16];
	}
	if (LCDActive && !fullSize) {
	  /* 
	   * If the LCD is active and we don't fill the entire screen
	   * and the previous mode was stretched we may need help from
	   * the BIOS to set all registers for the unstreched mode.
	   */
	    pTrident->doInit =  ((pReg->tridentRegs3CE[HorStretch] & 1)
				|| (pReg->tridentRegs3CE[VertStretch] & 1));
	    pReg->tridentRegs3CE[CyberControl] |= 0x81;
	    xf86DrvMsgVerb(pScrn->scrnIndex,X_INFO,1,"Shadow on\n");
	    isShadow = TRUE;
	} else {
	    pReg->tridentRegs3CE[CyberControl] &= 0x7E;
	    xf86DrvMsgVerb(pScrn->scrnIndex,X_INFO,1,"Shadow off\n");
	}


	if (pTrident->CyberShadow) {
	    pReg->tridentRegs3CE[CyberControl] &= 0x7E;
	    isShadow = FALSE;
	    xf86DrvMsgVerb(pScrn->scrnIndex,X_INFO,1,"Forcing Shadow off\n");
	}

 	xf86DrvMsgVerb(pScrn->scrnIndex,X_INFO,1,"H-timing shadow registers:"
 		       " 0x%2.2x           0x%2.2x 0x%2.2x 0x%2.2x\n",
 		       pReg->tridentRegs3x4[0], pReg->tridentRegs3x4[3],
 		       pReg->tridentRegs3x4[4], pReg->tridentRegs3x4[5]);
 	xf86DrvMsgVerb(pScrn->scrnIndex,X_INFO,1,"H-timing registers:       "
 		       " 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x\n",
 		       regp->CRTC[0], regp->CRTC[1], regp->CRTC[2],
		       regp->CRTC[3], regp->CRTC[4], regp->CRTC[5]);
 	xf86DrvMsgVerb(pScrn->scrnIndex,X_INFO,1,"V-timing shadow registers: "
 		       "0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x"
		       "           0x%2.2x (0x%2.2x)\n",
		       pReg->tridentRegs3x4[6], pReg->tridentRegs3x4[7],
 		       pReg->tridentRegs3x4[0x10],pReg->tridentRegs3x4[0x11],
 		       pReg->tridentRegs3x4[0x16],
 		       pReg->tridentRegs3x4[CRTHiOrd]);
 	xf86DrvMsgVerb(pScrn->scrnIndex,X_INFO,1,"V-timing registers:        "
 		       "0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x "
		       "0x%2.2x 0x%2.2x 0x%2.2x\n",
 		       regp->CRTC[6], regp->CRTC[7], regp->CRTC[0x10],
		       regp->CRTC[0x11],regp->CRTC[0x12],
 		       regp->CRTC[0x14],regp->CRTC[0x16]);
 	
	
	/* disable stretching, enable centering */
	pReg->tridentRegs3CE[VertStretch] &= 0xFC;
	pReg->tridentRegs3CE[VertStretch] |= 0x80;
	pReg->tridentRegs3CE[HorStretch] &= 0xFC;
	pReg->tridentRegs3CE[HorStretch] |= 0x80;
#if 1
	{
  	    int mul = pScrn->bitsPerPixel >> 3; 
	    int val;
	    
	    if (!mul) mul = 1;
	    
	    /* this is what my BIOS does */ 
	    val = (pScrn->currentMode->HDisplay * mul / 8) + 16;

	    pReg->tridentRegs3x4[PreEndControl] = ((val >> 8) < 2 ? 2 :0)
	      | ((val >> 8) & 0x01);
	    pReg->tridentRegs3x4[PreEndFetch] = val & 0xff;
	}
#else
	OUTB(vgaIOBase + 4,PreEndControl);
	pReg->tridentRegs3x4[PreEndControl] = INB(vgaIOBase + 5);
	OUTB(vgaIOBase + 4,PreEndFetch);
	pReg->tridentRegs3x4[PreEndFetch] = INB(vgaIOBase + 5);
#endif
	/* set mode */
	pReg->tridentRegs3CE[BiosMode] = TridentFindMode(
	    pScrn->currentMode->HDisplay,
	    pScrn->currentMode->VDisplay,
	    pScrn->depth);
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 1, "Setting BIOS Mode: %x\n",
		       pReg->tridentRegs3CE[BiosMode]);
	
	/* no stretch */
	pReg->tridentRegs3CE[BiosReg] = 0;

	if (pTrident->CyberStretch) {
	    pReg->tridentRegs3CE[VertStretch] |= 0x01;
	    pReg->tridentRegs3CE[HorStretch] |= 0x01;
	    xf86DrvMsgVerb(pScrn->scrnIndex,X_INFO,1,"Enabling StretchMode\n");
	}
    }
#endif /* is cyber */
    
    /* Calculate skew offsets for video overlay */
#if 0
    {
        int HTotal, HSyncStart;
	int VTotal, VSyncStart;
	int h_off = 0;
	int v_off = 0;

        if (isShadow) {
	    HTotal = pReg->tridentRegs3x4[0] << 3;
	    VTotal = pReg->tridentRegs3x4[6] 
	            | ((pReg->tridentRegs3x4[7] & (1<<0)) << 8)
	            | ((pReg->tridentRegs3x4[7] & (1<<5)) << 4);
	    HSyncStart = pReg->tridentRegs3x4[4] << 3;
	    VSyncStart = pReg->tridentRegs3x4[0x10] 
	            | ((pReg->tridentRegs3x4[7] & (1<<2)) << 6)
	            | ((pReg->tridentRegs3x4[7] & (1<<7)) << 2);
	    if (pTrident->lcdMode != 0xff) {
	        h_off = (LCD[pTrident->lcdMode].display_x 
		  - pScrn->currentMode->HDisplay) >> 1;
	        v_off = (LCD[pTrident->lcdMode].display_y 
		  - pScrn->currentMode->VDisplay) >> 1;
	    }
	} else {
	  HTotal = regp->CRTC[0] << 3;
	  VTotal = regp->CRTC[6] 
	            | ((regp->CRTC[7] & (1<<0)) << 8)
	            | ((regp->CRTC[7] & (1<<5)) << 4);	  
	  HSyncStart = regp->CRTC[4] << 3;;
	  VSyncStart = regp->CRTC[0x10] 
	            | ((regp->CRTC[7] & (1<<2)) << 6)
	            | ((regp->CRTC[7] & (1<<7)) << 2);
	}
	pTrident->hsync = (HTotal - HSyncStart) + 23 + h_off;
	pTrident->vsync = (VTotal - VSyncStart) - 2 + v_off;
	/* a little more skew for the Blade series */
	if (chip >= BLADE3D) pTrident->hsync -= 6;
    }
#endif
    
    /* Enable Chipset specific options */
    switch (chip) {
	case CYBERBLADEXP:
	case CYBERBLADEXPm:
	case CYBERBLADEI7:
	case CYBERBLADEI7D:
	case CYBERBLADEI1:
	case CYBERBLADEI1D:
	case CYBERBLADEAI1:
	case CYBERBLADEAI1D:
	case CYBERBLADEE4:
	case BLADE3D:
	    OUTB(0x3D4, RAMDACTiming);
	    pReg->tridentRegs3x4[RAMDACTiming] = INB(0x3D5) | 0x0F;
	    /* Fall Through */
	case CYBER9520:
	case CYBER9525DVD:
	case CYBER9397DVD:
	case CYBER9397:
	case IMAGE975:
	case IMAGE985:
	case CYBER9388:
	    if (modeinfo->bitsPerPixel >= 8)
    	    	pReg->tridentRegs3CE[MiscExtFunc] |= 0x10;
	    if (!pReg->tridentRegs3x4[PreEndControl])
	    	pReg->tridentRegs3x4[PreEndControl] = 0x01;
	    if (!pReg->tridentRegs3x4[PreEndFetch])
	    	pReg->tridentRegs3x4[PreEndFetch] = 0xFF;
	    /* Fall Through */
	case PROVIDIA9685:
	case CYBER9385:
	    pReg->tridentRegs3x4[Enhancement0] = 0x40;
	    /* Fall Through */
	case PROVIDIA9682:
	case CYBER9382:
#if 0
	    if (pTrident->UsePCIRetry) 
	    	pReg->tridentRegs3x4[PCIRetry] = 0xDF;
	    else
#endif
	    	pReg->tridentRegs3x4[PCIRetry] = 0x1F;
	    /* Fall Through */
	case TGUI9660:
	case TGUI9680:
#if 0
	    if (pTrident->MUX && pScrn->bitsPerPixel == 8 && mode->CrtcHAdjusted) {
	    	pReg->tridentRegs3x4[PixelBusReg] |= 0x01; /* 16bit bus */
	    	pReg->tridentRegs3C4[NewMode2] |= 0x02; /* half clock */
    		pReg->tridentRegsDAC[0x00] |= 0x20;	/* mux mode */
	    }	
#endif
	    break;
    }


    /* Defaults for all trident chipsets follows */
    switch (modeinfo->bitsPerPixel) {
	case 1:
	case 4:
    	    offset = modeinfo->width >> 4;
	    break;
	case 8:
	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x02;
    	    offset = modeinfo->width >> 3;
	    break;
	case 16:
	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x02;
    	    offset = modeinfo->width >> 2;
	    if (modeinfo->colorBits == 15)
    	    	pReg->tridentRegsDAC[0x00] = 0x10;
	    else
	    	pReg->tridentRegsDAC[0x00] = 0x30;
    	    pReg->tridentRegs3x4[PixelBusReg] = 0x04;
	    /* Reload with any chipset specific stuff here */
	    if (chip >= TGUI9660) 
		pReg->tridentRegs3x4[PixelBusReg] |= 0x01;
	    if (chip == TGUI9440AGi) {
    	        pReg->tridentRegs3CE[MiscExtFunc] |= 0x08;/*Clock Division / 2*/
	        clock *= 2;	/* Double the clock */
	    }
	    break;
	case 24:
	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x02;
    	    offset = (modeinfo->width * 3) >> 3;
    	    pReg->tridentRegs3x4[PixelBusReg] = 0x29;
	    pReg->tridentRegsDAC[0x00] = 0xD0;
	    if (chip == CYBERBLADEE4) {
    		OUTB(0x3D4, New32);
		pReg->tridentRegs3x4[New32] = INB(0x3D5) & 0x7F;
	    }
	    break;
	case 32:
	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x02;
	    if (chip != CYBERBLADEE4) {
	        /* Clock Division by 2*/
	        pReg->tridentRegs3CE[MiscExtFunc] |= 0x08; 
		clock *= 2;	/* Double the clock */
	    }
    	    offset = modeinfo->width >> 1;
    	    pReg->tridentRegs3x4[PixelBusReg] = 0x09;
	    pReg->tridentRegsDAC[0x00] = 0xD0;
	    if (chip == CYBERBLADEE4) {
    		OUTB(0x3D4, New32);
		pReg->tridentRegs3x4[New32] = INB(0x3D5) | 0x80;
		/* With new mode 32bpp we set the packed flag */
    	    	pReg->tridentRegs3x4[PixelBusReg] |= 0x20;
	    }
	    break;
    }
    
    pReg->tridentRegs3x4[Offset] = offset & 0xFF;

    {
	CARD8 a, b;
	TGUISetClock(clock, &a, &b);
	pReg->tridentRegsClock[0x00] = (INB(0x3CC) & 0xF3) | 0x08;
	pReg->tridentRegsClock[0x01] = a;
	pReg->tridentRegsClock[0x02] = b;
#if 0
	if (pTrident->MCLK > 0) {
	    TGUISetMCLK(pScrn, pTrident->MCLK, &a, &b);
	    pReg->tridentRegsClock[0x03] = a;
	    pReg->tridentRegsClock[0x04] = b;
	}
#endif
    }

    pReg->tridentRegs3C4[NewMode1] = 0xC0;
    pReg->tridentRegs3C4[Protection] = 0x92;

    pReg->tridentRegs3x4[LinearAddReg] = 0;

    if (0 /* pTrident->Linear */) {
#if 0
	if (chip < PROVIDIA9685)
    	    pReg->tridentRegs3x4[LinearAddReg] |=
					((pTrident->FbAddress >> 24) << 6)|
					((pTrident->FbAddress >> 20) & 0x0F);
	/* Turn on linear mapping */
    	pReg->tridentRegs3x4[LinearAddReg] |= 0x20; 
#endif
    } else {
	pReg->tridentRegs3CE[MiscExtFunc] |= 0x04;
    }
    
    pReg->tridentRegs3x4[CRTCModuleTest] = 
				(modetiming->flags & INTERLACED ? 0x84 : 0x80);
    OUTB(0x3D4, InterfaceSel);
    pReg->tridentRegs3x4[InterfaceSel] = INB(0x3D5) | 0x40;
    OUTB(0x3D4, Performance);
    pReg->tridentRegs3x4[Performance] = INB(0x3D5) | 0x10;
    OUTB(0x3D4, DRAMControl);
    pReg->tridentRegs3x4[DRAMControl] = INB(0x3D5) | 0x10;
#if 0
    if (pTrident->IsCyber && !pTrident->MMIOonly)
	pReg->tridentRegs3x4[DRAMControl] |= 0x20;
#endif
    OUTB(0x3D4, AddColReg);
    pReg->tridentRegs3x4[AddColReg] = INB(0x3D5) & 0xEF;
    pReg->tridentRegs3x4[AddColReg] |= (offset & 0x100) >> 4;

    if (chip >= TGUI9660) {
    	pReg->tridentRegs3x4[AddColReg] &= 0xDF;
    	pReg->tridentRegs3x4[AddColReg] |= (offset & 0x200) >> 4;
    }
   
#if 0
    if (IsPciCard && UseMMIO) {
    	if (!pTrident->NoAccel)
	    pReg->tridentRegs3x4[GraphEngReg] |= 0x80; 
    } else {
    	if (!pTrident->NoAccel)
	    pReg->tridentRegs3x4[GraphEngReg] |= 0x82; 
    }
#endif

    OUTB(0x3CE, MiscIntContReg);
    pReg->tridentRegs3CE[MiscIntContReg] = INB(0x3CF) | 0x04;

    /* Fix hashing problem in > 8bpp on 9320 chipset */
    if (chip == CYBER9320 && modeinfo->bitsPerPixel > 8) 
    	pReg->tridentRegs3CE[MiscIntContReg] &= ~0x80;

    OUTB(0x3D4, PCIReg);
    pReg->tridentRegs3x4[PCIReg] = INB(0x3D5) & 0xF9; 

    /* Enable PCI Bursting on capable chips */
    if (chip >= TGUI9660) {
	if(0 /*pTrident->UsePCIBurst*/ ) {
	    pReg->tridentRegs3x4[PCIReg] |= 0x06;
	} else {
	    pReg->tridentRegs3x4[PCIReg] &= 0xF9;
	}
    }

    /* Video */
    pReg->tridentRegs3C4[SSetup] = 0x00;
    pReg->tridentRegs3C4[SKey] = 0x00;
    pReg->tridentRegs3C4[SPKey] = 0x00;

    pReg->tridentRegs3x4[CursorControl] = 0x40; /* X11 style cursor */

     /* restore */
    OUTB(0x3C4, 0x11);
    OUTB(0x3C5, protect);

    return ;
}


static int setmode(int mode, int prv_mode)
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

    moderegs = calloc(TOTAL_REGS,1);

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
}

static void lock(void)
{
}

#define VENDOR_ID 0x1023

/* Indentify chipset, initialize and return non-zero if detected */

static int test(void)
{
    unsigned long buf[64];
    int found, id;
    
    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    id=(buf[0]>>16)&0xffff;
    if(found)return 0;
    if((id>=0x8400) && (id<=0x9930)) {
        init(0,0,0);
        return 1;
    }
    return 0;
}


/* Set display start address (not for 16 color modes) */
/* Cirrus supports any address in video memory (up to 2Mb) */

static void setdisplaystart(int address)
{ 
  address=address >> 2;
  __svgalib_outcrtc(0x0d,address&0xff);
  __svgalib_outcrtc(0x0c,(address>>8)&0xff);
  __svgalib_outcrtc(CRTCModuleTest, (__svgalib_incrtc(CRTCModuleTest)&0xdf) | ((address&0x10000)>>11));
  __svgalib_outcrtc(CRTHiOrd, (__svgalib_incrtc(CRTHiOrd)&0xf8) | ((address&0xe0000)>>17));
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
        int t;
        OUTB(0x3CE, MiscExtFunc);
        t=INB(0x3CF);
        OUTB(0x3CF, t&~0x04);
        OUTB(0x3D4, LinearAddReg);
        if(chip<PROVIDIA9685) {
            t=((linear_base>>24)<<6) | ((linear_base>>20)&0x0f) | 0x20;
        } else {
            t=0x20;
        }
        OUTB(0x3D5, t);
        is_linear=1; 
        return 0;
    };
    if (op==LINEAR_DISABLE){
        int t;
        OUTB(0x3CE, MiscExtFunc);
        t=INB(0x3CF);
        OUTB(0x3CF, t|0x04);
        OUTB(0x3D4, LinearAddReg);
        OUTB(0x3D5, 0 );
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
    int i, j;
    unsigned long *b3, *b4;
    
    switch(cmd){
        case CURSOR_INIT:
            return 1;
        case CURSOR_HIDE:
            __svgalib_outcrtc(0x50, __svgalib_incrtc(0x50) & 0x7f);
            break;
        case CURSOR_SHOW:
            __svgalib_outcrtc(0x50, __svgalib_incrtc(0x50) | 0x80);
            break;
        case CURSOR_POSITION:
            __svgalib_outcrtc(0x46, 0);
            __svgalib_outcrtc(0x47, 0);
            __svgalib_outcrtc(0x40, p1&0xff);
            __svgalib_outcrtc(0x41, (p1>>8)&0xff);
            __svgalib_outcrtc(0x42, p2&0xff);
            __svgalib_outcrtc(0x43, (p2>>8)&0xff);
            break;
        case CURSOR_SELECT:
            i=memory-(p1+1);
            switch(CI.colors) {
                case 256:
                   __svgalib_outcrtc(0x48,cursor_colors[p1*2+1].c8);
                   __svgalib_outcrtc(0x4c,cursor_colors[p1*2].c8);
                   break;
                case 32768:
                   __svgalib_outcrtc(0x49,cursor_colors[p1*2+1].c15>>8);
                   __svgalib_outcrtc(0x48,cursor_colors[p1*2+1].c15&0xff);
                   __svgalib_outcrtc(0x4d,cursor_colors[p1*2].c15>>8);
                   __svgalib_outcrtc(0x4c,cursor_colors[p1*2].c15&0xff);
                   break;
                case 65536:
                   __svgalib_outcrtc(0x49,cursor_colors[p1*2+1].c16>>8);
                   __svgalib_outcrtc(0x48,cursor_colors[p1*2+1].c16&0xff);
                   __svgalib_outcrtc(0x4d,cursor_colors[p1*2].c16>>8);
                   __svgalib_outcrtc(0x4c,cursor_colors[p1*2].c16&0xff);
                   break;
                case (1<<24):
                   __svgalib_outcrtc(0x4a,cursor_colors[p1*2+1].c32>>16);
                   __svgalib_outcrtc(0x49,(cursor_colors[p1*2+1].c32>>8)&0xff);
                   __svgalib_outcrtc(0x48,cursor_colors[p1*2+1].c32&0xff);
                   __svgalib_outcrtc(0x4e,cursor_colors[p1*2].c32>>16);
                   __svgalib_outcrtc(0x4d,(cursor_colors[p1*2].c32>>8)&0xff);
                   __svgalib_outcrtc(0x4c,cursor_colors[p1*2].c32&0xff);
                   break;
            }
            __svgalib_outcrtc(0x44, i&0xff);
            __svgalib_outcrtc(0x45, i>>8);
            break;
        case CURSOR_IMAGE:
            i=memory*1024-(p1+1)*1024;
            b3=(unsigned long *)p5;
            b4=(unsigned long *)(LINEAR_POINTER+i);
            if (!is_linear){
                int t;
                OUTB(0x3CE, MiscExtFunc);
                t=INB(0x3CF);
                OUTB(0x3CF, t&~0x04);
                OUTB(0x3D4, LinearAddReg);
                if(chip<PROVIDIA9685) {
                    t=((linear_base>>24)<<6) | ((linear_base>>20)&0x0f) | 0x20;
                } else {
                    t=0x20;
                }
                OUTB(0x3D5, t);
            };
            cursor_colors[p1*2].c8=findcolor(p3);
            cursor_colors[p1*2].c32=p3;
            cursor_colors[p1*2].c16=((p3&0xf80000)>>8)|((p3&0xfc00)>>5)|((p3&0xf8)>>3);
            cursor_colors[p1*2].c15=((p3&0xf80000)>>9)|((p3&0xf800)>>5)|((p3&0xf8)>>3);
            cursor_colors[p1*2+1].c8=findcolor(p4);
            cursor_colors[p1*2+1].c32=p4;
            cursor_colors[p1*2+1].c16=((p4&0xf80000)>>8)|((p4&0xfc00)>>5)|((p4&0xf8)>>3);
            cursor_colors[p1*2+1].c15=((p4&0xf80000)>>9)|((p4&0xf800)>>5)|((p4&0xf8)>>3);
            switch(p2) {
                case 0:
                    for(j=0;j<32;j++) {
                        *b4=BE32(*(b3+32));
                        b4++;
                        *b4=BE32(*b3);
                        b4++;
                        b3++;
                    }
                break;
            }
            if (!is_linear){
                int t;
                OUTB(0x3CE, MiscExtFunc);
                t=INB(0x3CF);
                OUTB(0x3CF, t|0x04);
                OUTB(0x3D4, LinearAddReg);
                OUTB(0x3D5, 0 );
            };
            break;
    }
    return 0;
}       

/* Function table (exported) */

DriverSpecs __svgalib_trident_driverspecs =
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
    setmode,
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
    int rev, i, id;
    int found=0;
    char *chipnames[]={
        "9420", "9430", "9440", "9320", "9660", "9680",
        "9682", "9685", "9382", "9385", "9388", "9397",
        "9397DVD", "9520", "9525", "975", "985", "Blade3D",
        "CyberBladeI7", "CyberBladeI7D", "CyberBladeI1",
        "CyberBladeI1D", "CyberBladeAI1", "CyberBladeAI1D",
        "CyberBladeE4", "CyberBladeXP", "CyberBladeXPm"
    };  
    unlock();
    if (force) {
	memory = par1;
        chip = par2;
    } else {

    };

    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    linear_base=0;
    id=(buf[0]>>16)&0xffff;
    if(found || (id<0x8400) || (id>0x9880))
        return 1;

    linear_base=buf[4]&0xffff0000;
    mmio_base=buf[5]&0xffff0000;

#if 0
    /* This should work */
    __svgalib_vgammbase=mmap(0,0x8000,PROT_READ|PROT_WRITE,MAP_SHARED,__svgalib_mem_fd,mmio_base);
    __svgalib_mm_io_mapio();
#endif

    OUTB(0x3C4, RevisionID);
    rev=INB(0x3C5);

    NewClockCode=0;
    
    switch(id) {
        case 0x9420:
            chip=TGUI9420DGi;
            break;
        case 0x9430:
            chip=TGUI9430DGi;
            break;
        case 0x9440:
        case 0x9460:
        case 0x9470:
            chip=TGUI9440AGi;
            break;
        case 0x9320:
            chip=CYBER9320;
            break;
        case 0x9660:
            switch(rev) {
                case 0x0:
                    chip=TGUI9660;
                    break;
                case 0x1:
                    chip=TGUI9680;
                    break;
                case 0x10:
                    chip=PROVIDIA9682;
                    break;
                case 0x21:
                    chip=PROVIDIA9685;
                    NewClockCode=1;
                    break;
                case 0x22:
                case 0x23:
                    chip=CYBER9397;
                    NewClockCode=1;
                    break;
                case 0x2a:
                    chip=CYBER9397DVD;
                    NewClockCode=1;
                    break;
                case 0x30:
                case 0x33:
                case 0x34:
                case 0x35:
                case 0x38:
                case 0x3a:
                case 0xb3:
                    chip=CYBER9385;
                    NewClockCode=1;
                    break;
                case 0x40:
                case 0x41:
                case 0x42:
                case 0x43:
                    chip=CYBER9382;
                    NewClockCode=1;
                    break;
                case 0x4a:
                    chip=CYBER9388;
                    NewClockCode=1;
                    break;
                default:
                    chip=TGUI9660;
                    break;
            }
            break;
        case 0x9388:
            chip=CYBER9388;
            NewClockCode=1;
            break;            
        case 0x9397:
            chip=CYBER9397;
            NewClockCode=1;
            break;            
        case 0x939a:
            chip=CYBER9397DVD;
            NewClockCode=1;
            break;            
        case 0x9520:
            chip=CYBER9520;
            NewClockCode=1;
            break;            
        case 0x9525:
            chip=CYBER9525DVD;
            NewClockCode=1;
            break;  
        case 0x9540:
            chip=CYBERBLADEE4;
            NewClockCode=1;
            break;  
        case 0x9750:
            chip=IMAGE975;
            NewClockCode=1;
            break;  
        case 0x9850:
            chip=IMAGE985;
            NewClockCode=1;
            break;  
        case 0x9880:
            chip=BLADE3D;
            NewClockCode=1;
            break;
        case 0x8400:
            chip=CYBERBLADEI7;
            NewClockCode=1;
            break;
        case 0x8420:
            chip=CYBERBLADEI7D;
            NewClockCode=1;
            break;
        case 0x8500:
            chip=CYBERBLADEI1;
            NewClockCode=1;
            break;
        case 0x8520:
            chip=CYBERBLADEI1D;
            NewClockCode=1;
            break;
        case 0x8600:
            chip=CYBERBLADEAI1;
            NewClockCode=1;
            break;
        case 0x8620:
            chip=CYBERBLADEAI1D;
            NewClockCode=1;
            break;
        case 0x9910:
            chip=CYBERBLADEXP;
            NewClockCode=1;
            break;
        case 0x9930:
            chip=CYBERBLADEXPm;
            NewClockCode=1;
            break;
        default:
            chip=TGUI9440AGi;
            break;
    }

    i=__svgalib_incrtc(SPR)&0x0f;
    switch(i) {
        case 3:
            memory=1024;
            break;
        case 4:
            memory=4096; /* actually, 8192 - but it has problems */
            break;
        case 7:
            memory=2048;
            break;
        case 15:
            memory=4096;
            break;
        default:
            memory=1024;
    }

    if(__svgalib_incrtc(TVinterface) & 0x80) frequency = 17.73448; else frequency = 14.31818;

    if (__svgalib_driver_report) {
	fprintf(stderr,"Using Trident driver, %s with %iKB.\n", chipnames[chip],memory);
    };

    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = memory;
    cardspecs->maxPixelClock8bpp = 160000;	
    cardspecs->maxPixelClock16bpp = 160000;	
    cardspecs->maxPixelClock24bpp = 160000;
    cardspecs->maxPixelClock32bpp = 160000;
    cardspecs->flags = INTERLACE_DIVIDE_VERT | CLOCK_PROGRAMMABLE;
    cardspecs->maxHorizontalCrtc = 2040;
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->nClocks =0;
    cardspecs->mapClock = map_clock;
    cardspecs->mapHorizontalCrtc = map_horizontal_crtc;
    cardspecs->matchProgrammableClock=match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_trident_driverspecs;
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=linear_base;
    __svgalib_linear_mem_size=memory*0x400;
    return 0;
}

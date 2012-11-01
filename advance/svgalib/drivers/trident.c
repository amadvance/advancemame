/*
Trident PCI driver 
Tested on 9685, CyberbladeXP4
*/

#include <stdlib.h>
#include <stdio.h>		
#include <unistd.h>
#include "vga.h"
#include "libvga.h"
#include "svgadriv.h"
#include "timing.h"
#include "vgaregs.h"
#include "interfac.h"
#include "vgapci.h"
#include "trident.h"
#include "vgammvga.h"
#include "endianes.h"

#define TOTAL_REGS (VGA_TOTAL_REGS + sizeof(TRIDENTRegRec))

static int init(int, int, int);
static void unlock(void);
static void lock(void);

static int memory, chip, NewClockCode, shadowNew, lcdactive, lcdmode, stretch=1;
static int frequency=0;
static int is_linear, linear_base, mmio_base;

static CardSpecs *cardspecs;

static int TridentFindMode(int xres, int yres, int depth)
{
    int xres_s;
    int i, size;
    biosMode *mode;

    switch (depth) {
		case 1:
			size = sizeof(bios1) / sizeof(biosMode);
			mode = bios1;
			break;
		case 4:
			size = sizeof(bios4) / sizeof(biosMode);
			mode = bios4;
			break;
		case 8:
			size = sizeof(bios8) / sizeof(biosMode);
			mode = bios8;
			break;
		case 15:
			size = sizeof(bios15) / sizeof(biosMode);
			mode = bios15;
			break;
		case 16:
			size = sizeof(bios16) / sizeof(biosMode);
			mode = bios16;
			break;
		case 24:
			size = sizeof(bios24) / sizeof(biosMode);
			mode = bios24;
			break;
		default:
			return 0;
	}
    
	for (i = 0; i < size; i++) {
		if (xres <= mode[i].x_res) {
			xres_s = mode[i].x_res;
			for (; i < size; i++) {
				if (mode[i].x_res != xres_s)
					return mode[i-1].mode;
				if (yres <= mode[i].y_res)
					return mode[i].mode;
			}
		}
	}
    
	return mode[size - 1].mode;
}

static void TridentFindNewMode(int xres, int yres, CARD8 *gr5a, CARD8 *gr5c)
{
    int xres_s;
    int i, size;
    
    size = sizeof(newModeRegs) / sizeof(newModes);

    for (i = 0; i < size; i++) {
		if (xres <= newModeRegs[i].x_res) {
			xres_s = newModeRegs[i].x_res;
			for (; i < size; i++) {
				if (newModeRegs[i].x_res != xres_s 
						|| yres <= newModeRegs[i].y_res) {
					*gr5a = newModeRegs[i].GR5a;
					*gr5c = newModeRegs[i].GR5c;
					return;
				}
			}
		}
	}
	
	*gr5a = newModeRegs[size - 1].GR5a;
	*gr5c = newModeRegs[size - 1].GR5c;
	
	return;
}


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
	    	modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
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
    __svgalib_outseq(NewMode1, 0xC0 ^ 0x02);
    __svgalib_outseq(Protection, 0x92);

    INB_3x4(Offset);
    INB_3x4(LinearAddReg);
    INB_3x4(CRTCModuleTest);
    INB_3x4(CRTHiOrd);
    INB_3x4(HorizOverflow);
    INB_3x4(Performance);
    INB_3x4(InterfaceSel);
    INB_3x4(DRAMControl);
    INB_3x4(AddColReg);
    INB_3x4(PixelBusReg);
    INB_3x4(GraphEngReg);
    INB_3x4(PCIReg);
    INB_3x4(PCIRetry);
    
	if(chip >= CYBER9388) {
		INB_3C4(Threshold);
		INB_3C4(SSetup);
		INB_3C4(SKey);
		INB_3C4(SPKey);
		INB_3x4(PreEndControl);
		INB_3x4(PreEndFetch);
		INB_3C4(GBslope1);
		INB_3C4(GBslope2);
		INB_3C4(GBslope3);
		INB_3C4(GBslope4);
		INB_3C4(GBintercept1);
		INB_3C4(GBintercept2);
		INB_3C4(GBintercept3);
		INB_3C4(GBintercept4);
	}
										
    if (chip >= PROVIDIA9685) INB_3x4(Enhancement0);
    if (chip >= BLADE3D)      INB_3x4(RAMDACTiming);
    if (chip == CYBERBLADEE4 || chip == CYBERBLADEXP4) INB_3x4(New32);
	if (chip == CYBERBLADEXP4) INB_3CE(DisplayEngCont);

    if (IsCyber) {
		uint8_t tmp;
		INB_3CE(VertStretch);
		INB_3CE(HorStretch);

		if(chip < CYBERBLADEXP) {
			INB_3CE(BiosMode);
		} else {
			INB_3CE(BiosNewMode1);
			INB_3CE(BiosNewMode2);
		}
		
		INB_3CE(BiosReg);
		INB_3CE(FPConfig);
		INB_3CE(CyberControl);
		INB_3CE(CyberEnhance);
		SHADOW_ENABLE(tmp);
		INB_3x4(0x0);
		if(shadowNew) {
			INB_3x4(0x1);
			INB_3x4(0x2);
		}
		INB_3x4(0x3);
		INB_3x4(0x4);
		INB_3x4(0x5);
		INB_3x4(0x6);
		INB_3x4(0x7);
		INB_3x4(0x10);
		INB_3x4(0x11);
		if(shadowNew) {
			INB_3x4(0x12);
			INB_3x4(0x15);
		}
		INB_3x4(0x16);
		SHADOW_RESTORE(tmp);
	}

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
    __svgalib_outseq(NewMode1, 0xC0 ^ 0x02);
    
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
    OUTW_3x4(HorizOverflow);
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
	
	if(chip >= CYBER9388) {
		OUTW_3C4(Threshold);
		OUTW_3C4(SSetup);
		OUTW_3C4(SKey);
		OUTW_3C4(SPKey);
		OUTW_3x4(PreEndControl);
		OUTW_3x4(PreEndFetch);
		OUTW_3C4(GBslope1);
		OUTW_3C4(GBslope2);
		OUTW_3C4(GBslope3);
		OUTW_3C4(GBslope4);
		OUTW_3C4(GBintercept1);
		OUTW_3C4(GBintercept2);
		OUTW_3C4(GBintercept3);
		OUTW_3C4(GBintercept4);
	
	}
	
    if (chip >= PROVIDIA9685) OUTW_3x4(Enhancement0);
    if (chip >= BLADE3D)      OUTW_3x4(RAMDACTiming);
    if (chip == CYBERBLADEE4 || chip == CYBERBLADEXP4) OUTW_3x4(New32);
	if (chip == CYBERBLADEXP4) OUTW_3CE(DisplayEngCont);
	if (IsCyber) {
		uint8_t tmp;

		OUTW_3CE(VertStretch);
		OUTW_3CE(HorStretch);
		if(chip <= CYBERBLADEXP) {
			OUTW_3CE(BiosMode);
		} else {
			OUTW_3CE(BiosNewMode1);
			OUTW_3CE(BiosNewMode2);
		}
		OUTW_3CE(BiosReg);	
		OUTW_3CE(FPConfig);	
		OUTW_3CE(CyberControl);
		OUTW_3CE(CyberEnhance);
		SHADOW_ENABLE(tmp);
		OUTW_3x4(0x0);
		if(shadowNew) {
			OUTW_3x4(0x1);
			OUTW_3x4(0x2);
		}
		OUTW_3x4(0x3);
		OUTW_3x4(0x4);
		OUTW_3x4(0x5);
		OUTW_3x4(0x6);
		OUTW_3x4(0x7);	
		OUTW_3x4(0x10);
		OUTW_3x4(0x11); 
		if(shadowNew) {
			OUTW_3x4(0x12);
			OUTW_3x4(0x15);
		}
		OUTW_3x4(0x16);
		SHADOW_RESTORE(tmp);
	}
	
    if (Is3Dchip) {
        __svgalib_outseq(ClockLow, tridentReg->tridentRegsClock[0x01]);
        __svgalib_outseq(ClockHigh, tridentReg->tridentRegsClock[0x02]);

#if 0
		if (pTrident->MCLK > 0) {
			OUTW(0x3C4,(tridentReg->tridentRegsClock[0x03])<<8 | MCLKLow);
			OUTW(0x3C4,(tridentReg->tridentRegsClock[0x04])<<8 | MCLKHigh);
		}
#endif
    } else {
	    OUTB(0x43C8, tridentReg->tridentRegsClock[0x01]);
	    OUTB(0x43C9, tridentReg->tridentRegsClock[0x02]);
#if 0
		if (pTrident->MCLK > 0) {
			OUTB(0x43C6, tridentReg->tridentRegsClock[0x03]);
			OUTB(0x43C7, tridentReg->tridentRegsClock[0x04]);
		}
#endif
    }
	OUTB(0x3C2, tridentReg->tridentRegsClock[0x00]);
    
    if (chip > PROVIDIA9685) {
    	OUTB(0x3C4, Protection);
    	OUTB(0x3C5, tridentReg->tridentRegs3C4[Protection]);
    }

    __svgalib_outseq(NewMode1, tridentReg->tridentRegs3C4[NewMode1] ^ 0x02);

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

static void TGUISetClock(int clock, uint8_t *a, uint8_t *b)
{
	int powerup[4] = { 1,2,4,8 };
	int clock_diff;
	int setup;
	int freq, ffreq;
	int m, n, k;
	int p, q, r;
	int endn, endm, endk, startk=0, startn;

	p = q = r = setup = 0;
	clock_diff = clock;

	if (NewClockCode)
	{	
		startn = 64;
		endn = 255;
		endm = 63;
		endk = 2;
		if (clock >= 100000) startk = 0;
		if (clock < 100000) startk = 1;
		if (clock < 50000) startk = 2;
	}
	else
	{
		startn = 32;
		endn = 121;
		endm = 31;
		endk = 1;
		if(clock>50000) startk=1; else startk = 0;
	}

 	freq = clock;

	for (k=startk;k<=endk;k++)
	  for (n=startn;n<=endn;n++)
	    for (m=1;m<=endm;m++)
	    {
		ffreq =  ( ((n + 8) * frequency) / ((m + 2) * powerup[k]) ) ;
		if (!setup || abs(ffreq - freq) < clock_diff)
		{
			clock_diff = abs(ffreq - freq);
			p = n; q = m; r = k; setup = 1;
		}
	    }

	if (NewClockCode) {
		/* N is all 8bits */
		*a = p;
		/* M is first 6bits, with K last 2bits */
		*b = (q & 0x3F) | (r << 6);
	} else {
		/* N is first 7bits, first M bit is 8th bit */
		*a = ((1 & q) << 7) | p;
		/* first 4bits are rest of M, 1bit for K value */
		*b = (((q & 0xFE) >> 1) | (r << 4));
	}
}

static void initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{ /* long k; */

    int offset=0, protect=0, fullsize=0, MUX;
    int clock;
    
    TRIDENTRegPtr pReg;
    
    pReg = (TRIDENTRegPtr)(moderegs+60);
   
    clock=modetiming->pixelClock;

	if((clock>90000) && (modeinfo->bitsPerPixel==8)) {
		MUX=1;
		modetiming->CrtcHDisplay>>=1;
		modetiming->CrtcHTotal>>=1;
		modetiming->CrtcHSyncStart>>=1;
		modetiming->CrtcHSyncEnd>>=1;
	} else {
		MUX=0;
	}
		
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

	if(chip>PROVIDIA9685) {
		protect=__svgalib_inseq(Protection);
		__svgalib_outseq( Protection, 0x92);
	}

	__svgalib_inseq(0x0b);

	pReg->tridentRegs3x4[PixelBusReg] = 0x00;
	pReg->tridentRegsDAC[0x00] = 0x00;
	pReg->tridentRegs3C4[NewMode2] = 0x20;
	pReg->tridentRegs3CE[MiscExtFunc] = __svgalib_ingra(MiscExtFunc);
	pReg->tridentRegs3x4[GraphEngReg] = 0x00;
	pReg->tridentRegs3x4[PreEndControl] = 0;
	pReg->tridentRegs3x4[PreEndFetch] = 0;

	pReg->tridentRegs3x4[CRTHiOrd] = (((modetiming->CrtcVSyncEnd-1) & 0x400)>>4) |
		(((modetiming->CrtcVTotal - 2) & 0x400) >> 3) |
		((modetiming->CrtcVSyncStart & 0x400) >> 5) |
		(((modetiming->CrtcVDisplay - 1) & 0x400) >> 6)|
		0x08;

	pReg->tridentRegs3x4[HorizOverflow] = ((modetiming->CrtcHTotal & 0x800) >> 11) |
		((modetiming->CrtcHSyncStart & 0x800)>>7);

	if(IsCyber) {
		pReg->tridentRegs3CE[FPConfig] = __svgalib_ingra(FPConfig);
		pReg->tridentRegs3CE[CyberEnhance] = __svgalib_ingra(CyberEnhance) & 0x8f;
		
		if (modetiming->CrtcVDisplay > 1024)
			pReg->tridentRegs3CE[CyberEnhance] |= 0x50;
		else if (modetiming->CrtcVDisplay > 768)
			pReg->tridentRegs3CE[CyberEnhance] |= 0x30;
		else if (modetiming->CrtcVDisplay > 600)
			pReg->tridentRegs3CE[CyberEnhance] |= 0x20;
		else if (modetiming->CrtcVDisplay > 480)
			pReg->tridentRegs3CE[CyberEnhance] |= 0x10;

		pReg->tridentRegs3CE[CyberControl] = __svgalib_ingra(CyberControl);
		pReg->tridentRegs3CE[HorStretch] = __svgalib_ingra(HorStretch);
		pReg->tridentRegs3CE[VertStretch] = __svgalib_ingra(VertStretch);
				
		if(lcdmode!=0xff) {
			pReg->tridentRegs3x4[0x0] = LCD[lcdmode].shadow_0;
			pReg->tridentRegs3x4[0x1] = moderegs[0x1];
			pReg->tridentRegs3x4[0x2] = moderegs[0x2];
			pReg->tridentRegs3x4[0x3] = LCD[lcdmode].shadow_3;
			pReg->tridentRegs3x4[0x4] = LCD[lcdmode].shadow_4;
			pReg->tridentRegs3x4[0x5] = LCD[lcdmode].shadow_5;
			pReg->tridentRegs3x4[0x6] = LCD[lcdmode].shadow_6;
			pReg->tridentRegs3x4[0x7] = LCD[lcdmode].shadow_7;
			pReg->tridentRegs3x4[0x10] = LCD[lcdmode].shadow_10;
			pReg->tridentRegs3x4[0x11] = LCD[lcdmode].shadow_11;
			pReg->tridentRegs3x4[0x12] = moderegs[0x12];
			pReg->tridentRegs3x4[0x15] = moderegs[0x15];
			pReg->tridentRegs3x4[0x16] = LCD[lcdmode].shadow_16;
			if(lcdactive) {
				pReg->tridentRegs3x4[CRTHiOrd] = LCD[lcdmode].shadow_HiOrd;
			}
			fullsize = (modetiming->HDisplay == LCD[lcdmode].display_x)
				&& (modetiming->VDisplay == LCD[lcdmode].display_y);
		}
	    
		pReg->tridentRegs3x4[0x7] &= ~0x4A;
		pReg->tridentRegs3x4[0x7] |= moderegs[0x7] & 0x4A;

		if(lcdactive && fullsize) {
			moderegs[0x0]=pReg->tridentRegs3x4[0x0];
			moderegs[0x3]=pReg->tridentRegs3x4[0x3];
			moderegs[0x4]=pReg->tridentRegs3x4[0x4];
			moderegs[0x5]=pReg->tridentRegs3x4[0x5];
			moderegs[0x6]=pReg->tridentRegs3x4[0x6];
			moderegs[0x7]=pReg->tridentRegs3x4[0x7];
			moderegs[0x10]=pReg->tridentRegs3x4[0x10];
			moderegs[0x11]=pReg->tridentRegs3x4[0x11];
			moderegs[0x16]=pReg->tridentRegs3x4[0x16];
		}
		
		if(lcdactive && !fullsize) {
			moderegs[VGA_MISCOUTPUT] |= 0xc0;
			pReg->tridentRegs3CE[CyberControl] |= 0x81;
		} else {
			pReg->tridentRegs3CE[CyberControl] &= 0x7e;
		}

		/* FPDELAY CYBERSHADOW ??? */

		/* disable stretching, enable centering */
		pReg->tridentRegs3CE[VertStretch] &= 0xFC;
		pReg->tridentRegs3CE[VertStretch] |= 0x80;
		pReg->tridentRegs3CE[HorStretch] &= 0xFC;
		pReg->tridentRegs3CE[HorStretch] |= 0x80;
						
#if 1
		{
			int mul = (modeinfo->bitsPerPixel+1) >> 3;
			int val;
			
			if (!mul) mul = 1;
			
			/* this is what my BIOS does */
			val = (modetiming->HDisplay * mul / 8) + 16;
			
			pReg->tridentRegs3x4[PreEndControl] = ((val >> 8) < 2 ? 2 :0)
				| ((val >> 8) & 0x01);
			pReg->tridentRegs3x4[PreEndFetch] = val & 0xff;
		}
#else
		pReg->tridentRegs3x4[PreEndControl]=__svgalib_incrtc(PreEndControl);
		pReg->tridentRegs3x4[PreEndFetch]=__svgalib_incrtc(PreEndFetch);
#endif
		
		if(chip<CYBERBLADEXP) {
			pReg->tridentRegs3CE[BiosMode] = TridentFindMode(
					modetiming->HDisplay, modetiming->VDisplay,	modeinfo->colorBits);
		} else {
			TridentFindNewMode(modetiming->HDisplay, modetiming->VDisplay,
					&pReg->tridentRegs3CE[BiosNewMode1],
					&pReg->tridentRegs3CE[BiosNewMode2]);
		}

		if(chip != CYBERBLADEXPAI1) {
			pReg->tridentRegs3CE[BiosReg] = 0;
		} else {
			pReg->tridentRegs3CE[BiosReg] = 8;
		}

		if(stretch) {
			pReg->tridentRegs3CE[HorStretch] |= 0x01;
			pReg->tridentRegs3CE[VertStretch] |= 0x01;
		}
	}

	switch(chip) {
		case CYBERBLADEXP4:
		case CYBERBLADEXPAI1:
		case CYBERBLADEXP:
		case CYBERBLADEI7:
		case CYBERBLADEI7D:
		case CYBERBLADEI1:
		case CYBERBLADEI1D:
		case CYBERBLADEAI1:
		case CYBERBLADEAI1D:
		case CYBERBLADEE4:
		case BLADE3D:
			pReg->tridentRegs3x4[RAMDACTiming] = __svgalib_incrtc(RAMDACTiming) | 0x0F;
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
			else
				pReg->tridentRegs3CE[MiscExtFunc] &= ~0x10;
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
			if (MUX && modeinfo->bitsPerPixel == 8) {
				pReg->tridentRegs3x4[PixelBusReg] |= 0x01; /* 16bit bus */
				pReg->tridentRegs3C4[NewMode2] |= 0x02; /* half clock */
				pReg->tridentRegsDAC[0x00] |= 0x20;	/* mux mode */
			}	
	}
	
    /* Defaults for all trident chipsets follows */
    switch (modeinfo->bitsPerPixel) {
		case 1:
		case 4:
			offset = modeinfo->lineWidth >> 4;
			break;
		case 8:
			pReg->tridentRegs3CE[MiscExtFunc] |= 0x02;
			offset = modeinfo->lineWidth >> 3;
			break;
		case 16:
			pReg->tridentRegs3CE[MiscExtFunc] |= 0x02;
			offset = modeinfo->lineWidth >> 3;
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
			offset = modeinfo->lineWidth >> 3;
			pReg->tridentRegs3x4[PixelBusReg] = 0x29;
			pReg->tridentRegsDAC[0x00] = 0xD0;
			if (chip == CYBERBLADEXP4 || chip == CYBERBLADEE4) {
				pReg->tridentRegs3x4[New32] = __svgalib_incrtc(New32) & 0x7F;
			}
			break;
		case 32:
			pReg->tridentRegs3CE[MiscExtFunc] |= 0x02;
			if (chip != CYBERBLADEXP4 
					&& chip != CYBERBLADEE4	&& chip != CYBERBLADEXPAI1) {
				/* Clock Division by 2*/
				pReg->tridentRegs3CE[MiscExtFunc] |= 0x08; 
				clock *= 2;	/* Double the clock */
			}
			offset = modeinfo->lineWidth >> 3;
			pReg->tridentRegs3x4[PixelBusReg] = 0x09;
			pReg->tridentRegsDAC[0x00] = 0xD0;
			if (chip == CYBERBLADEXP4 || chip == CYBERBLADEE4
					|| chip == CYBERBLADEXPAI1) {
				pReg->tridentRegs3x4[New32] = __svgalib_incrtc(New32) | 0x80;
				/* With new mode 32bpp we set the packed flag */
				pReg->tridentRegs3x4[PixelBusReg] |= 0x20; 
			}
			break;
	}
 	
	pReg->tridentRegs3x4[Offset] = offset & 0xFF;
    {
		uint8_t a, b;
		TGUISetClock(clock, &a, &b);
		pReg->tridentRegsClock[0x00] = moderegs[VGA_MISCOUTPUT] | 0x08;
		pReg->tridentRegsClock[0x01] = a;
		pReg->tridentRegsClock[0x02] = b;
	}

	pReg->tridentRegs3C4[NewMode1] = 0xC0;
    pReg->tridentRegs3C4[Protection] = 0x92;

    pReg->tridentRegs3x4[LinearAddReg] = 0;

	if(chip<CYBER9385) {
		pReg->tridentRegs3x4[LinearAddReg] |= 0x20;
	}

	pReg->tridentRegs3CE[MiscExtFunc] |= 0x04;

	pReg->tridentRegs3x4[CRTCModuleTest] = 
				(modetiming->flags & INTERLACED ? 0x84 : 0x80);
	
    pReg->tridentRegs3x4[InterfaceSel] = __svgalib_incrtc(InterfaceSel)| 0x40;
    
	pReg->tridentRegs3x4[Performance] = __svgalib_incrtc(Performance);
	if(chip<CYBERBLADEXP) pReg->tridentRegs3x4[Performance] |= 0x10;
    
	if(chip >= CYBER9388) pReg->tridentRegs3x4[DRAMControl] =  
							  __svgalib_incrtc(DRAMControl) | 0x10;

    if (IsCyber) pReg->tridentRegs3x4[DRAMControl] |= 0x20;

	if(NewClockCode && chip <= CYBER9397DVD) {
		pReg->tridentRegs3x4[ClockControl] = __svgalib_incrtc(ClockControl) | 0x01;
	}
	
	pReg->tridentRegs3x4[AddColReg] = __svgalib_incrtc(AddColReg) & 0xEF;
    pReg->tridentRegs3x4[AddColReg] |= (offset & 0x100) >> 4;

    if (chip >= TGUI9660) {
    	pReg->tridentRegs3x4[AddColReg] &= 0xDF;
    	pReg->tridentRegs3x4[AddColReg] |= (offset & 0x200) >> 4;
    }

	pReg->tridentRegs3CE[MiscIntContReg] = __svgalib_ingra(MiscIntContReg) | 0x04;


	pReg->tridentRegs3x4[PCIReg] = __svgalib_incrtc(PCIReg) & 0xF8;

	/* PCIBURST   BRIGHTNESS */

	pReg->tridentRegs3C4[SSetup] = __svgalib_inseq(0x20) | 0x4;
	pReg->tridentRegs3C4[SKey] = 0x00;
	pReg->tridentRegs3C4[SPKey] = 0xC0;
	pReg->tridentRegs3C4[Threshold] = __svgalib_inseq(0x12);
	if (modeinfo->bitsPerPixel > 16) pReg->tridentRegs3C4[Threshold] =
			(pReg->tridentRegs3C4[Threshold] & 0xf0) | 0x2;

	
	if (chip > PROVIDIA9685) {
		__svgalib_outseq( Protection, protect);
	}
	
	if (chip == CYBERBLADEXP4)
		pReg->tridentRegs3CE[DisplayEngCont] = 0x08;
	
	/* Avoid lockup on Blade3D, PCI Retry is permanently on */
	if (chip == BLADE3D)
		pReg->tridentRegs3x4[PCIRetry] = 0x9F;
				
	return;
}


static int local_setmode(int mode, int prv_mode)
{
    unsigned char *moderegs;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {
		return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
	}
	
	if (!modeavailable(mode)) return 1;
	
	__svgalib_modeinfo_linearset &= ~IS_LINEAR;
	is_linear=0;

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
    if( !init(0,0,0)) return 1;
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
    int rev, i, id;
    int found=0;
    char *chipnames[]={
        "9420", "9430", "9440", "9320", "9660", "9680",
        "9682", "9382", "9385", "9685", "9388", "9397",
        "9397DVD", "9520", "9525", "975", "985", "Blade3D",
        "CyberBladeI7", "CyberBladeI7D", "CyberBladeI1",
        "CyberBladeI1D", "CyberBladeAI1", "CyberBladeAI1D",
        "CyberBladeE4", "CyberBladeXP", "CyberBladeXPAI1",
		"CyberBladeXP4"
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
    if(found)
        return 1;

    linear_base=buf[4]&0xffff0000;
    mmio_base=buf[5]&0xffff0000;

#if 0
    /* This should work */
	__svgalib_mmio_base=mmio_base;
	__svgalib_mmio_size=32*0x400;
	map_mmio();
	__svgalib_vgammbase=0;
	__svgalib_mm_io_mapio();
#endif

    OUTB(0x3C4, RevisionID);
    rev=INB(0x3C5);

    NewClockCode=0;
	shadowNew=0;
    
    switch(id) {
        case 0x9420:
            chip=TGUI9420DGi;
            frequency = 14318;
            break;
        case 0x9430:
            chip=TGUI9430DGi;
            frequency = 14318;
            break;
        case 0x9440:
        case 0x9460:
        case 0x9470:
            chip=TGUI9440AGi;
            frequency = 14318;
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
            frequency = 14318;
            break;
        case 0x8400:
            chip=CYBERBLADEI7;
            NewClockCode=1;
            frequency = 14318;
            break;
        case 0x8420:
            chip=CYBERBLADEI7D;
            NewClockCode=1;
            frequency = 14318;
            break;
        case 0x8500:
            chip=CYBERBLADEI1;
            NewClockCode=1;
            frequency = 14318;
            break;
        case 0x8520:
            chip=CYBERBLADEI1D;
            NewClockCode=1;
            frequency = 14318;
            break;
        case 0x8600:
            chip=CYBERBLADEAI1;
            NewClockCode=1;
            frequency = 14318;
            break;
        case 0x8620:
            chip=CYBERBLADEAI1D;
            NewClockCode=1;
            frequency = 14318;
            break;
        case 0x9910:
            chip=CYBERBLADEXP;
            NewClockCode=1;
            frequency = 14318;
            break;
        case 0x2100:
        case 0x9930:
            chip=CYBERBLADEXP4;
            NewClockCode=1;
			shadowNew=1;
            frequency = 14318;
            break;
        default:
            chip=TGUI9440AGi;
            frequency = 14318;
            break;
    }

    i=__svgalib_incrtc(SPR)&0x0f;
    switch(i) {
		case 1:
			memory=512;
			break;
		case 2:
			memory=6144;
			break;
        case 3:
            memory=1024;
            break;
        case 4:
            memory=4096; /* actually, 8192 - but it has problems */
            break;
        case 6:
            memory=10240;
            break;
        case 7:
            memory=2048;
            break;
        case 8:
            memory=12288;
            break;
        case 10:
            memory=14336;
            break;
        case 12:
            memory=16384;
            break;
		case 14:
			memory=32768;
			break;
        case 15:
            memory=4096;
            break;
        default:
            memory=1024;
    }

    if((frequency==0) && (__svgalib_incrtc(TVinterface) & 0x80)) 
		frequency = 17734; else frequency=14318;


	if(IsCyber) {
		int mod, dsp, dsp1;
		dsp = __svgalib_ingra(0x42);
		dsp1 = __svgalib_ingra(0x43);
		mod = __svgalib_ingra(0x52);
		
		for (i = 0; LCD[i].mode != 0xff; i++) {
			if (LCD[i].mode == ((mod >> 4) & 3)) {
				lcdactive = __svgalib_ingra(FPConfig) & 0x10;
				lcdmode=LCD[i].mode;
				fprintf(stderr, "%s Panel %ix%i found (%s).\n",
						(dsp & 0x80) ? "TFT" :
						((dsp1 & 0x20) ? "DSTN" : "STN"),
						LCD[i].display_x,LCD[i].display_y,
						lcdactive ? "active":"inactive");
			}
		}
	}
	
	
    if (__svgalib_driver_report) {
		fprintf(stderr,"Using Trident driver, %s with %iKB.\n", chipnames[chip],memory);
    };

    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = memory;
    cardspecs->maxPixelClock8bpp = ClockLimit[chip+12];
    cardspecs->maxPixelClock16bpp = ClockLimit16bpp[chip+12];	
    cardspecs->maxPixelClock24bpp = ClockLimit24bpp[chip+12];
    cardspecs->maxPixelClock32bpp = ClockLimit32bpp[chip+12];
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

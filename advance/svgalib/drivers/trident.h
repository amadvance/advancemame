typedef struct {
	unsigned char tridentRegs3x4[0xd0];
	unsigned char tridentRegs3CE[0x60];
	unsigned char tridentRegs3C4[0x60];
	unsigned char tridentRegsDAC[0x01];
        unsigned char tridentRegsClock[0x03];
} TRIDENTRegRec, *TRIDENTRegPtr;

enum {
    TGUI9420DGi,
    TGUI9430DGi,
    TGUI9440AGi,
    CYBER9320,
    TGUI9660,
    TGUI9680,
    PROVIDIA9682,
    PROVIDIA9685,
    CYBER9382,
    CYBER9385,
    CYBER9388,
    CYBER9397,
    CYBER9397DVD,
    CYBER9520,
    CYBER9525DVD,
    IMAGE975,
    IMAGE985,
    BLADE3D,
    CYBERBLADEI7,
    CYBERBLADEI7D,
    CYBERBLADEI1,
    CYBERBLADEI1D,
    CYBERBLADEAI1,
    CYBERBLADEAI1D,
    CYBERBLADEE4,
    CYBERBLADEXP,
    CYBERBLADEXPm
};

#define Is3Dchip	((chip == CYBER9397) || \
			 (chip == CYBER9397DVD) || \
			 (chip == CYBER9520) || \
			 (chip == CYBER9525DVD) || \
			 (chip == CYBERBLADEE4)  || \
			 (chip == IMAGE975)  || \
			 (chip == IMAGE985)  || \
			 (chip == CYBERBLADEI7)  || \
			 (chip == CYBERBLADEI7D)  || \
			 (chip == CYBERBLADEI1)  || \
			 (chip == CYBERBLADEI1D)  || \
			 (chip == CYBERBLADEAI1)  || \
			 (chip == CYBERBLADEAI1D)  || \
			 (chip == BLADE3D) || \
			 (chip == CYBERBLADEXP) || \
			 (chip == CYBERBLADEXPm))

#if 0
#define INB(addr) (*(unsigned char *)(__svgalib_vgammbase+addr))
#define OUTB(addr, val) (*(unsigned char *)(__svgalib_vgammbase+addr) = val)
#define OUTW(addr, val) (*(unsigned short *)(__svgalib_vgammbase+addr) = val)
#else
#define INB(addr) inb(addr)
#define OUTB(addr, val) outb(addr,val)
#define OUTW(addr, val) outw(addr,val)
#endif

#define OUTW_3C4(reg) \
    	__svgalib_outseq(reg, tridentReg->tridentRegs3C4[reg])
#define OUTW_3CE(reg) \
    	__svgalib_outgra(reg, tridentReg->tridentRegs3CE[reg])
#define OUTW_3x4(reg) \
    	__svgalib_outcrtc(reg, tridentReg->tridentRegs3x4[reg])

#define INB_3x4(reg) \
    	tridentReg->tridentRegs3x4[reg] = __svgalib_incrtc(reg)
#define INB_3C4(reg) \
    	tridentReg->tridentRegs3C4[reg] = __svgalib_inseq(reg)
#define INB_3CE(reg) \
    	tridentReg->tridentRegs3CE[reg] = __svgalib_ingra(reg)

/* 3C4 */
#define RevisionID 0x09
#define NewMode2 0x0D
#define NewMode1 0x0E
#define Protection 0x11
#define MCLKLow 0x16
#define MCLKHigh 0x17
#define ClockLow 0x18
#define ClockHigh 0x19
#define SPR	0x1F		/* Software Programming Register (videoram) */
#define SSetup 0x20
#define SKey 0x37
#define SPKey 0x57

/* 3D4 */
#define Offset 0x13
#define Underline 0x14
#define CRTCMode 0x17
#define CRTCModuleTest 0x1E
#define FIFOControl 0x20
#define LinearAddReg 0x21
#define DRAMTiming 0x23
#define New32 0x23
#define RAMDACTiming 0x25
#define CRTHiOrd 0x27
#define AddColReg 0x29
#define InterfaceSel 0x2A
#define GETest 0x2D
#define Performance 0x2F
#define GraphEngReg 0x36
#define I2C 0x37
#define PixelBusReg 0x38
#define PCIReg 0x39
#define DRAMControl 0x3A
#define MiscContReg 0x3C
#define CursorXLow 0x40
#define CursorXHigh 0x41
#define CursorYLow 0x42
#define CursorYHigh 0x43
#define CursorLocLow 0x44
#define CursorLocHigh 0x45
#define CursorXOffset 0x46
#define CursorYOffset 0x47
#define CursorFG1 0x48
#define CursorFG2 0x49
#define CursorFG3 0x4A
#define CursorFG4 0x4B
#define CursorBG1 0x4C
#define CursorBG2 0x4D
#define CursorBG3 0x4E
#define CursorBG4 0x4F
#define CursorControl 0x50
#define PCIRetry 0x55
#define PreEndControl 0x56
#define PreEndFetch 0x57
#define PCIMaster 0x60
#define Enhancement0 0x62
#define NewEDO 0x64
#define TVinterface 0xC0
#define TVMode 0xC1
#define ClockControl 0xCF

/* 3CE */
#define MiscExtFunc 0x0F
#define MiscIntContReg 0x2F
#define CyberControl 0x30
#define CyberEnhance 0x31
#define FPConfig     0x33
#define VertStretch  0x52
#define HorStretch   0x53
#define BiosMode     0x5c
#define BiosReg      0x5d


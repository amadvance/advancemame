typedef unsigned char CARD8;

typedef struct {
	unsigned char tridentRegs3x4[0xd0];
	unsigned char tridentRegs3CE[0xe0];
	unsigned char tridentRegs3C4[0xc0];
	unsigned char tridentRegsDAC[0x01];
	unsigned char tridentRegsClock[0x03];
} TRIDENTRegRec, *TRIDENTRegPtr;

enum {
    TGUI9420DGi=0,
    TGUI9430DGi,
    TGUI9440AGi,
    CYBER9320,
    TGUI9660,
    TGUI9680,
    PROVIDIA9682,
    CYBER9382,
    CYBER9385,
    PROVIDIA9685,
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
    CYBERBLADEXPAI1,
    CYBERBLADEXP4
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
			 (chip == CYBERBLADEXPAI1) || \
			 (chip == CYBERBLADEXP4))

#define IsCyber ((chip == CYBER9397) || \
			 (chip == CYBER9397DVD) || \
			 (chip == CYBER9525DVD) || \
			 (chip == CYBER9382) || \
			 (chip == CYBER9385) || \
			 (chip == CYBER9388) || \
			 (chip == CYBER9520) || \
			 (chip == CYBERBLADEE4)  || \
			 (chip == CYBERBLADEI7D)  || \
			 (chip == CYBERBLADEI1)  || \
			 (chip == CYBERBLADEI1D)  || \
			 (chip == CYBERBLADEAI1)  || \
			 (chip == CYBERBLADEAI1D)  || \
			 (chip == CYBERBLADEXP) || \
			 (chip == CYBERBLADEXPAI1) || \
			 (chip == CYBERBLADEXP4))



#if 0
#define INB(addr) (*(unsigned char *)(__svgalib_vgammbase+addr))
#define OUTB(addr, val) (*(unsigned char *)(__svgalib_vgammbase+addr) = val)
#define OUTW(addr, val) (*(unsigned short *)(__svgalib_vgammbase+addr) = val)
#else
#define INB(addr) port_in(addr)
#define OUTB(addr, val) port_out_r(addr,val)
#define OUTW(addr, val) port_outw_r(addr,val)
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

#define SPR 0x1F        /* Software Programming Register (videoram) */
	
/* 3C4 */
#define RevisionID 0x09
#define ConfPort1 0x0C
#define ConfPort2 0x0C
#define NewMode2 0x0D
#define OldMode2 0x00 /* Should be 0x0D - dealt with in trident_dac.c */
#define OldMode1 0x0E
#define NewMode1 0x0E
#define Protection 0x11
#define Threshold 0x12
#define MCLKLow 0x16
#define MCLKHigh 0x17
#define ClockLow 0x18
#define ClockHigh 0x19
#define SSetup 0x20
#define SKey 0x37
#define SPKey 0x57
#define GBslope1 0xB4
#define GBslope2 0xB5
#define GBslope3 0xB6
#define GBslope4 0xB7
#define GBintercept1 0xB8
#define GBintercept2 0xB9
#define GBintercept3 0xBA
#define GBintercept4 0xBB
	
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
#define HorizOverflow 0x2B
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
#define BiosNewMode1 0x5a
#define BiosNewMode2 0x5c
#define BiosReg      0x5d
#define DisplayEngCont 0xD1
	
#define SHADOW_ENABLE(oldval) \
	do {\
		OUTB(0x3CE, CyberControl); \
		oldval = INB(0x3CF);\
		OUTB(0x3CF,oldval | (1 << 6));\
	} while (0)
#define SHADOW_RESTORE(val) \
    do {\
		OUTB(0x3CE, CyberControl); \
		OUTB(0x3CF,val); \
	} while (0);

static int ClockLimit[] = {
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
	80000,
/* 9440 */
	70000, /* is it 90000 ?? The strict mode limitation means 70 */
	90000,
	135000,
	135000,
	170000,
	170000,
	170000,
/* 9685 */
	170000,
	170000,
	170000,
	170000,
	230000,
	230000,
	230000,
	230000,
	230000,
/* CYBERBLADEI7 */
	230000,
	230000,
	230000,
	230000,
	230000,
	230000,
	230000,
	230000,
	230000,
	230000,
};

static int ClockLimit16bpp[] = {
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
	40000,
/* 9440 */
	35000, /* is it 45000 ?? The strict mode limitation means 35 */
	45000,
	90000,
	90000,
	135000,
	135000,
	170000,
/* 9685 */
	170000,
	170000,
	170000,
	170000,
	230000,
	230000,
	230000,
	230000,
	230000,
/* CYBERBLADEI7 */
	230000,
	230000,
	230000,
	230000,
	230000,
	230000,
	230000,
	230000,
	230000,
	230000,
	230000,
	230000,
	230000,
}; 

static int ClockLimit24bpp[] = {
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
/* 9440 */
	25180,
	25180,
	40000,
	40000,
	70000,
	70000,
	70000,
/* 9685 */
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
/* CYBERBLADEI7 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

static int ClockLimit32bpp[] = {
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
	25180,
/* 9440 */
	25180,
	25180,
	40000,
	40000,
	70000,
	70000,
	70000,
/* 9685 */
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
/* CYBERBLADEI7 */
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
	115000,
};

typedef struct {
	int mode;
	int display_x;
	int display_y;
	int clock;
	int shadow_0;
	int shadow_3;
	int shadow_4;
	int shadow_5;
	int shadow_6;
	int shadow_7;
	int shadow_10;
	int shadow_11;
	int shadow_16;
	int shadow_HiOrd;
} tridentLCD;


tridentLCD LCD[] = {
	{ 1,640,480,25200,0x5f,0x80,0x52,0x1e,0xb,0x3e,0xea,0x0c,0xb,0x08},
	{ 3,800,600,40000,0x7f,0x00,0x69,0x7f,0x72,0xf0,0x59,0x0d,0x00,0x08},
	{ 2,1024,768,65000,0xa3,0x00,0x84,0x94,0x24,0xf5,0x03,0x09,0x24,0x08},
	{ 0,1280,1024,108000,0xce,0x91,0xa6,0x14,0x28,0x5a,0x01,0x04,0x28,0xa8},
	{ 4,1400,1050,122000,0xe6,0xcd,0xba,0x1d,0x38,0x00,0x1c,0x28,0x28,0xf8},
	{ 0xff,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

typedef struct {
	int x_res;
	int y_res;
	int mode;
} biosMode;

typedef struct {
	int x_res;
	int y_res;
	CARD8 GR5a;
	CARD8 GR5c;
} newModes;

static biosMode bios1[] = {
	{ 640, 480, 0x11 }
};

static biosMode bios4[] = {
	{ 320, 200, 0xd },
	{ 640, 200, 0xe },
	{ 640, 350, 0x11 },
	{ 640, 480, 0x12 },
	{ 800, 600, 0x5b },
	{ 1024, 768 , 0x5f },
	{ 1280, 1024, 0x63 },
	{ 1600, 1200, 0x65 }
};

static biosMode bios8[] = {
	{ 320, 200, 0x13 },
	{ 640, 400, 0x5c },
	{ 640, 480, 0x5d },
	{ 720, 480, 0x60 },
	{ 800, 600, 0x5e },
	{ 1024, 768, 0x62 },
	{ 1280, 1024, 0x64 },
	{ 1600, 1200, 0x66 }
};

static biosMode bios15[] = {
	{ 640, 400, 0x72 },
	{ 640, 480, 0x74 },
	{ 720, 480, 0x70 },
	{ 800, 600, 0x76 },
	{ 1024, 768, 0x78 },
	{ 1280, 1024, 0x7a },
	{ 1600, 1200, 0x7c }
};

static biosMode bios16[] = {
	{ 640, 400, 0x73 },
	{ 640, 480, 0x75 },
	{ 720, 480, 0x71 },
	{ 800, 600, 0x77 },
	{ 1024, 768, 0x79 },
	{ 1280, 1024, 0x7b },
	{ 1600, 1200, 0x7d }
};

static biosMode bios24[] = {
	{ 640, 400, 0x6b },
	{ 640, 480, 0x6c },
	{ 720, 480, 0x61 },
	{ 800, 600, 0x6d },
	{ 1024, 768, 0x6e }
};

static newModes newModeRegs [] = {
	{ 320, 200, 0x13, 0x30 },
	{ 640, 480, 0x13, 0x61 },
	{ 800, 600, 0x13, 0x61 },
	{ 1024, 768, 0x3b, 0x63 },
	{ 1280, 1024, 0x7b, 0x64 },
	{ 1400, 1050, 0x11, 0x7b }
};



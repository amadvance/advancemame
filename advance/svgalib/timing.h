
#ifndef TIMING_H
#define TIMING_H

/*
 * Generic mode timing module.
 */

/* This is the type of a basic (monitor-oriented) mode timing. */
typedef struct _MMT_S MonitorModeTiming;
struct _MMT_S {
    int pixelClock;		/* Pixel clock in kHz. */
    int HDisplay;		/* Horizontal Timing. */
    int HSyncStart;
    int HSyncEnd;
    int HTotal;
    int VDisplay;		/* Vertical Timing. */
    int VSyncStart;
    int VSyncEnd;
    int VTotal;
    int flags;
    MonitorModeTiming *next;
};

/* This is for the hardware (card)-adjusted mode timing. */
typedef struct {
    int pixelClock;		/* Pixel clock in kHz. */
    int HDisplay;		/* Horizontal Timing. */
    int HSyncStart;
    int HSyncEnd;
    int HTotal;
    int VDisplay;		/* Vertical Timing. */
    int VSyncStart;
    int VSyncEnd;
    int VTotal;
    int flags;
/* The following field are optionally filled in according to card */
/* specific parameters. */
    int programmedClock;	/* Actual clock to be programmed. */
    int selectedClockNo;	/* Index number of fixed clock used. */
    int CrtcHDisplay;		/* Actual programmed horizontal CRTC timing. */
    int CrtcHSyncStart;
    int CrtcHSyncEnd;
    int CrtcHTotal;
    int CrtcVDisplay;		/* Actual programmed vertical CRTC timing. */
    int CrtcVSyncStart;
    int CrtcVSyncEnd;
    int CrtcVTotal;
} ModeTiming;

/* Flags in ModeTiming. */
#define PHSYNC		0x1	/* Positive hsync polarity. */
#define NHSYNC		0x2	/* Negative hsync polarity. */
#define PVSYNC		0x4	/* Positive vsync polarity. */
#define NVSYNC		0x8	/* Negative vsync polarity. */
#define INTERLACED	0x10	/* Mode has interlaced timing. */
#define DOUBLESCAN	0x20	/* Mode uses VGA doublescan (see note). */
#define HADJUSTED	0x40	/* Horizontal CRTC timing adjusted. */
#define VADJUSTED	0x80	/* Vertical CRTC timing adjusted. */
#define USEPROGRCLOCK	0x100	/* A programmable clock is used. */
#define TVMODE		0x200
#define TVPAL		0x400
#define TVNTSC		0x800 

/*
 * Note: Double scan implies that each scanline is displayed twice. The
 * vertical CRTC timings are programmed to double the effective vertical
 * resolution (the CRT still displays 400 scanlines for a 200 line
 * resolution).
 */

/* Cards specifications. */
typedef struct {
    int videoMemory;		/* Video memory in kilobytes. */
    int maxPixelClock4bpp;	/* Maximum pixel clocks in kHz for each depth. */
    int maxPixelClock8bpp;
    int maxPixelClock16bpp;
    int maxPixelClock24bpp;
    int maxPixelClock32bpp;
    int flags;			/* Flags (e.g. programmable clocks). */
    int nClocks;		/* Number of fixed clocks. */
    int *clocks;		/* Pointer to array of fixed clock values. */
    int maxHorizontalCrtc;
    /*
     * The following function maps from a pixel clock and depth to
     * the raw clock frequency required.
     */
    int (*mapClock) (int bpp, int pixelclock);
    /*
     * The following function maps from a requested clock value
     * to the closest clock that the programmable clock device
     * can produce.
     */
    int (*matchProgrammableClock) (int desiredclock);
    /*
     * The following function maps from a pixel clock, depth and
     * horizontal CRTC timing parameter to the horizontal timing
     * that has to be programmed.
     */
    int (*mapHorizontalCrtc) (int bpp, int pixelclock, int htiming);
} CardSpecs;

/* Card flags. */
/* The card has programmable clocks (matchProgrammableClock is valid). */
#define CLOCK_PROGRAMMABLE		0x1
/* For interlaced modes, the vertical timing must be divided by two. */
#define INTERLACE_DIVIDE_VERT		0x2
/* For modes with vertical timing greater or equal to 1024, vertical */
/* timing must be divided by two. */
#define GREATER_1024_DIVIDE_VERT	0x4
/* The DAC doesn't support 64K colors (5-6-5) at 16bpp, just 5-5-5. */
#define NO_RGB16_565			0x8
/* Card (or driver) can't do interlaced modes */
#define NO_INTERLACE			0x10
/* Don't have banked memory - emulated with mmap from linear memory */
#define EMULATE_BANK			0x20

/* Mode info. */
typedef struct {
/* Basic properties. */
    short width;		/* Width of the screen in pixels. */
    short height;		/* Height of the screen in pixels. */
    char bytesPerPixel;		/* Number of bytes per pixel. */
    char bitsPerPixel;		/* Number of bits per pixel. */
    char colorBits;		/* Number of significant bits in pixel. */
    char __padding1;
/* Truecolor pixel specification. */
    char redWeight;		/* Number of significant red bits. */
    char greenWeight;		/* Number of significant green bits. */
    char blueWeight;		/* Number of significant blue bits. */
    char __padding2;
    char redOffset;		/* Offset in bits of red value into pixel. */
    char blueOffset;		/* Offset of green value. */
    char greenOffset;		/* Offset of blue value. */
    char __padding3;
    unsigned redMask;		/* Pixel mask of read value. */
    unsigned blueMask;		/* Pixel mask of green value. */
    unsigned greenMask;		/* Pixel mask of blue value. */
/* Structural properties of the mode. */
    int lineWidth;		/* Offset in bytes between scanlines. */
    short realWidth;		/* Real on-screen resolution. */
    short realHeight;		/* Real on-screen resolution. */
    int flags;
} ModeInfo;


/* Prototypes of functions defined in timing.c. */

/*
 * This function will look up mode timings for a mode matching ModeInfo
 * that is within monitor spec and matches the capabilities (clocks etc.)
 * of the card.
 */

int __svgalib_getmodetiming(
		     ModeTiming *,	/* Resulting mode timing. */
		     ModeInfo *,	/* Structural mode info. */
		     CardSpecs *	/* Card specs (dot clocks etc.). */
);

void __svgalib_addusertiming(
		      MonitorModeTiming *
);

/* GTF constants */
#define GTF_lockVF	1		/* Lock to vertical frequency	*/
#define GTF_lockHF	2		/* Lock to horizontal frequency	*/
#define GTF_lockPF	3		/* Lock to pixel clock frequency*/

#endif

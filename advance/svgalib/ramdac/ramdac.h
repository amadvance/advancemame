/*
 * ramdac.h:
 * 
 * Structures and functions for programmable ramdac support.
 */

/* DacMethods type. */

typedef struct {
    int id;
    char *name;
    int flags;
    /*
     * The following function tries to detect the DAC;
     * returns nonzero if succesful.
     */
    int (*probe) (void);
    /*
     * The following function initializes the DAC; it is usually
     * called once after detection.
     */
    void (*initialize) (void);
    /*
     * The following function fills in dot clock limits, and
     * mapping functions for the raw clock and horizontal
     * CRTC timing, in the cardspecs structure.
     * 
     * dacspeed is the max pixel clock in kHz the dac can handle, 
     * or 0 for default.
     */
    void (*qualifyCardSpecs) (CardSpecs * cardspecs, int dacspeed);
    /*
     * The following function saves RAMDAC registers into regs.
     */
    void (*saveState) (unsigned char *regs);
    /*
     * The following function sets the RAMDAC registers with the
     * values from regs.
     */
    void (*restoreState) (const unsigned char *regs);
    /*
     * The following function sets up the RAMDAC register values
     * for the desired color operating mode. If the DAC has
     * programmable clocks, it should also program the clock.
     */
    void (*initializeState) (unsigned char *regs, int bpp, int colormode,
			     int pixelclock);
    int stateSize;		/* Size in bytes of the state (saved registers). */
} DacMethods;

/* IDs */

#define NORMAL_DAC	1
#define S3_GENDAC	2	/* S3-GenDAC (8-bit DAC). */
#define S3_SDAC		3	/* S3-SDAC (16-bit DAC). */
#define TRIO64		4	/* Trio64 internal DAC. */
#define SIERRA_32K	5	/* Basic DAC with 32K color mode support. */
#define ATT20C490	6	/* Standard AT&T 8-bit DAC with truecolor. */
#define ATT20C498	7	/* Standard AT&T 16-bit DAC. */
#define IC_WORKS	8	/* IC Works DAC (16-bit ZoomDac). */
#define SIERRA_15025	9	/* Sierra SC15025/26 DAC. */
#define IBMRGB52x	10	/* IBM RGB52x Palette DAC. */
#define SIERRA_1148X	11	/* Sierra SC1148x DAC. */

/* Flags. */

#define DAC_HAS_PROGRAMMABLE_CLOCKS	0x1

/* Color modes. */

#define CLUT8_6		0
#define CLUT8_8		1
#define RGB16_555	2
#define RGB16_565	3
#define RGB24_888_B	4	/* 3 bytes per pixel, blue byte first. */
#define RGB32_888_B	5	/* 4 bytes per pixel. */

/* State size */
#define MAX_DAC_STATE	0x101	/* IBMRGB has this many */

/* RAMDAC methods */

#ifdef INCLUDE_NORMAL_DAC
extern DacMethods __svgalib_normal_dac_methods;
#endif
#ifdef INCLUDE_S3_SDAC_DAC
extern DacMethods __svgalib_S3_SDAC_methods;
#endif
#ifdef INCLUDE_S3_GENDAC_DAC
extern DacMethods __svgalib_S3_GENDAC_methods;
#endif
#ifdef INCLUDE_S3_TRIO64_DAC
extern DacMethods __svgalib_Trio64_methods;
#endif
#ifdef INCLUDE_SIERRA_DAC
extern DacMethods __svgalib_Sierra_32K_methods;
#endif
#ifdef INCLUDE_SC15025_DAC
extern DacMethods __svgalib_SC15025_methods;
#endif
#ifdef INCLUDE_ATT20C490_DAC
extern DacMethods __svgalib_ATT20C490_methods;
#endif
#ifdef INCLUDE_ATT20C498_DAC
extern DacMethods __svgalib_ATT20C498_methods;
#endif
#ifdef INCLUDE_ICW_DAC
extern DacMethods __svgalib_ICW_methods;
#endif
#ifdef INCLUDE_IBMRGB52x_DAC
extern DacMethods __svgalib_IBMRGB52x_methods;
#endif
#ifdef INCLUDE_SC1148X_DAC
extern DacMethods __svgalib_SC1148X_methods;
#endif
#ifdef INCLUDE_ICS_GENDAC_DAC
extern DacMethods __svgalib_ICS_GENDAC_methods;
#endif

extern DacMethods *__svgalib_all_dacs[];	/* List of all defined DACs. */

/* Functions defined in ramdac.c. */

/*
 * The following function probes the DACs in daclist, which must be
 * terminated by NULL. It returns the detected DAC if succesful, NULL
 * otherwise. The detected DAC is also initialized.
 */

DacMethods *__svgalib_probeDacs(DacMethods ** daclist);

/* 
 * Internal functions (not meant for export, but no such mechanism in C)
 */
int __svgalib_setDacSpeed(int dacspeed, int defspeed);

void __svgalib_Sierra_32K_savestate(unsigned char *regs);
void __svgalib_Sierra_32K_restorestate(const unsigned char *regs);

#ifdef __OPTIMIZE__
static __inline__ void _ramdac_dactocomm(void)
{
    port_in(PEL_IW);
    port_in(PEL_MSK);
    port_in(PEL_MSK);
    port_in(PEL_MSK);
    port_in(PEL_MSK);
}

static __inline__ void _ramdac_dactopel(void)
{
    port_in(PEL_IW);
}

static __inline__ unsigned char _ramdac_setcomm(unsigned char data)
{
    _ramdac_dactocomm();
    port_out_r(PEL_MSK, data);
    _ramdac_dactocomm();
    return port_in(PEL_MSK);
}
#else
void _ramdac_dactocomm(void);
void _ramdac_dactopel(void);
unsigned char _ramdac_setcomm(unsigned char data);
#endif

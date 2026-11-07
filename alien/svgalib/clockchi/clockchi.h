/*
 * clockchip.h
 */

/* ClockChipMethods type. */

typedef struct {
    /*
     * The following function initializes the ClockChip; it is usually
     * called once after detection.
     */
    void (*initialize) (CardSpecs * cardspecs, DacMethods * DAC);
    /* 
     * ClockChip functions that override DAC methods.
     */
    void (*saveState) (unsigned char *regs);
    void (*restoreState) (const unsigned char *regs);
    void (*initializeState) (unsigned char *regs, int bpp, int colormode,
			     int pixelclock);
    /*
     * Original DAC save and restore functions, 
     * to be called before clock manipulation.
     */
    void (*DAC_saveState) (unsigned char *regs);
    void (*DAC_restoreState) (const unsigned char *regs);
    void (*DAC_initializeState) (unsigned char *regs, int bpp, int colormode,
				 int pixelclock);
    long TextFrequency;
    int DAC_stateSize;
} ClockChipMethods;

extern ClockChipMethods __svgalib_I2061A_clockchip_methods;

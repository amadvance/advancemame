

/* Prototypes of functions defined in interface.c. */

/*
 * This is a temporary function that allocates and fills in a ModeInfo
 * structure based on a svgalib mode number.
 */

ModeInfo *__svgalib_createModeInfoStructureForSvgalibMode(int mode);

/*
 * This function converts a number of significant color bits to a matching
 * DAC mode type as defined in the RAMDAC interface.
 */

int __svgalib_colorbits_to_colormode(int bpp, int colorbits);

/*
 * Clear the accelspecs structure (disable acceleration).
 */

void __svgalib_clear_accelspecs(AccelSpecs * accelspecs);

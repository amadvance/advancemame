

/* Register indices into mode state array. */

#define VGA_CRTC_COUNT		24
#define VGA_ATC_COUNT		21
#define VGA_GRAPHICS_COUNT	9
#define VGA_SEQUENCER_COUNT	5

#define VGA_CRTC_OFFSET		0	/* 24 registers */
#define VGA_ATC_OFFSET		24	/* 21 registers */
#define VGA_GRAPHICS_OFFSET	45	/* 9 registers. */
#define VGA_SEQUENCER_OFFSET	54	/* 5 registers. */
#define VGA_MISCOUTPUT		59	/* (single register) */
#define VGA_TOTAL_REGS		60

/* Total of 60 registers. */

#define VGAREG_CR(i)		(i)
#define VGAREG_AR(i)		(i + VGA_ATC_OFFSET)
#define VGAREG_GR(i)		(i + VGA_GRAPHICS_OFFSET)
#define VGAREG_SR(i)		(i + VGA_SEQUENCER_OFFSET)

#define VGA_CR0			VGAREG_CR(0x00)
#define VGA_CR1			VGAREG_CR(0x01)
#define VGA_CR2			VGAREG_CR(0x02)
#define VGA_CR3			VGAREG_CR(0x03)
#define VGA_CR4			VGAREG_CR(0x04)
#define VGA_CR5			VGAREG_CR(0x05)
#define VGA_CR6			VGAREG_CR(0x06)
#define VGA_CR7			VGAREG_CR(0x07)
#define VGA_CR8			VGAREG_CR(0x08)
#define VGA_CR9			VGAREG_CR(0x09)
#define VGA_CRA			VGAREG_CR(0x0A)
#define VGA_CRB			VGAREG_CR(0x0B)
#define VGA_CRC			VGAREG_CR(0x0C)
#define VGA_CRD			VGAREG_CR(0x0D)
#define VGA_CRE			VGAREG_CR(0x0E)
#define VGA_CRF			VGAREG_CR(0x0F)
#define VGA_CR10		VGAREG_CR(0x10)
#define VGA_CR11		VGAREG_CR(0x11)
#define VGA_CR12		VGAREG_CR(0x12)
#define VGA_CR13		VGAREG_CR(0x13)
#define VGA_SCANLINEOFFSET	VGAREG_CR(0x13)
#define VGA_CR14		VGAREG_CR(0x14)
#define VGA_CR15		VGAREG_CR(0x15)
#define VGA_CR16		VGAREG_CR(0x16)
#define VGA_CR17		VGAREG_CR(0x17)
#define VGA_CR18		VGAREG_CR(0x18)

#define VGA_AR0			VGAREG_AR(0x00)
#define VGA_AR10		VGAREG_AR(0x10)
#define VGA_AR11		VGAREG_AR(0x11)
#define VGA_AR12		VGAREG_AR(0x12)
#define VGA_AR13		VGAREG_AR(0x13)
#define VGA_AR14		VGAREG_AR(0x14)

#define VGA_GR0			VGAREG_GR(0x00)
#define VGA_GR1			VGAREG_GR(0x01)
#define VGA_GR2			VGAREG_GR(0x02)
#define VGA_GR3			VGAREG_GR(0x03)
#define VGA_GR4			VGAREG_GR(0x04)
#define VGA_GR5			VGAREG_GR(0x05)
#define VGA_GR6			VGAREG_GR(0x06)
#define VGA_GR7			VGAREG_GR(0x07)
#define VGA_GR8			VGAREG_GR(0x08)

#define VGA_SR0			VGAREG_SR(0x00)
#define VGA_SR1			VGAREG_SR(0x01)
#define VGA_SR2			VGAREG_SR(0x02)
#define VGA_SR3			VGAREG_SR(0x03)
#define VGA_SR4			VGAREG_SR(0x04)


/*
 * Set the bits bytemask in variable bytevar with the value of bits
 * valuemask in value (masks must match, but may be shifted relative
 * to eachother). With proper masks, should optimize into shifts.
 */

#define SETBITSFROMVALUE(bytevar, bytemask, value, valuemask) \
	if (valuemask > bytemask) \
		bytevar = (bytevar & (~(unsigned char)bytemask)) \
			| (((value) & valuemask) / (valuemask / bytemask)); \
	else \
		bytevar = (bytevar & (~(unsigned char)bytemask)) \
			| (((value) & valuemask) * (bytemask / valuemask));

/*
 * Set bits bytemask in bytevar, with value bits (no shifting).
 */

#define SETBITS(bytevar, bytemask, bits) \
	bytevar = (bytevar & (~(unsigned char)bytemask)) + bits;

#define min(x, y) ((x) < (y) ? (x) : (y))
#define LIMIT(var, lim) if (var > lim) var = lim;


/* Prototypes of functions defined in vgaregs.c. */

void __svgalib_setup_VGA_registers(
			    unsigned char *moderegs,
			    ModeTiming * modetiming,
			    ModeInfo * modeinfo
);

#define __svgalib_inGR __svgalib_ingra
#define __svgalib_outGR __svgalib_outgra
#define __svgalib_outbGR __svgalib_outgra
#define __svgalib_inSR __svgalib_inseq
#define __svgalib_outSR __svgalib_outseq
#define __svgalib_outbSR __svgalib_outseq
#define __svgalib_inCR __svgalib_incrtc
#define __svgalib_outCR __svgalib_outcrtc
#define __svgalib_outbCR __svgalib_outcrtc

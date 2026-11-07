
#include "timing.h"
#include "vgaregs.h"


/*
 * Setup VGA registers for SVGA mode timing. Adapted from XFree86,
 * vga256/vga/vgaHW.c vgaHWInit().
 *
 * Note that VGA registers are set up in a way that is common for
 * SVGA modes. This is not particularly useful for standard VGA
 * modes, since VGA does not have a clean packed-pixel mode.
 */

void __svgalib_setup_VGA_registers(unsigned char *moderegs, ModeTiming * modetiming,
			 ModeInfo * modeinfo)
{
    int i;
/* Sync Polarities */
    if ((modetiming->flags & (PHSYNC | NHSYNC)) &&
	(modetiming->flags & (PVSYNC | NVSYNC))) {
	/*
	 * If both horizontal and vertical polarity are specified,
	 * set them as specified.
	 */
	moderegs[VGA_MISCOUTPUT] = 0x23;
	if (modetiming->flags & NHSYNC)
	    moderegs[VGA_MISCOUTPUT] |= 0x40;
	if (modetiming->flags & NVSYNC)
	    moderegs[VGA_MISCOUTPUT] |= 0x80;
    } else {
	/*
	 * Otherwise, calculate the polarities according to
	 * monitor standards.
	 */
	if (modetiming->VDisplay < 400)
	    moderegs[VGA_MISCOUTPUT] = 0xA3;
	else if (modetiming->VDisplay < 480)
	    moderegs[VGA_MISCOUTPUT] = 0x63;
	else if (modetiming->VDisplay < 768)
	    moderegs[VGA_MISCOUTPUT] = 0xE3;
	else
	    moderegs[VGA_MISCOUTPUT] = 0x23;
    }

/* Sequencer */
    moderegs[VGA_SR0] = 0x00;
    if (modeinfo->bitsPerPixel == 4)
	moderegs[VGA_SR0] = 0x02;
    moderegs[VGA_SR1] = 0x01;
    moderegs[VGA_SR2] = 0x0F;	/* Bitplanes. */
    moderegs[VGA_SR3] = 0x00;
    moderegs[VGA_SR4] = 0x0E;
    if (modeinfo->bitsPerPixel == 4)
	moderegs[VGA_SR4] = 0x06;

/* CRTC Timing */
    moderegs[VGA_CR0] = (modetiming->CrtcHTotal / 8) - 5;
    moderegs[VGA_CR1] = (modetiming->CrtcHDisplay / 8) - 1;
    moderegs[VGA_CR2] = (modetiming->CrtcHSyncStart / 8) - 1;
    moderegs[VGA_CR3] = ((modetiming->CrtcHSyncEnd / 8) & 0x1F) | 0x80;
    moderegs[VGA_CR4] = (modetiming->CrtcHSyncStart / 8);
    moderegs[VGA_CR5] = (((modetiming->CrtcHSyncEnd / 8) & 0x20) << 2)
	| ((modetiming->CrtcHSyncEnd / 8) & 0x1F);
    moderegs[VGA_CR6] = (modetiming->CrtcVTotal - 2) & 0xFF;
    moderegs[VGA_CR7] = (((modetiming->CrtcVTotal - 2) & 0x100) >> 8)
	| (((modetiming->CrtcVDisplay - 1) & 0x100) >> 7)
	| ((modetiming->CrtcVSyncStart & 0x100) >> 6)
	| (((modetiming->CrtcVSyncStart) & 0x100) >> 5)
	| 0x10
	| (((modetiming->CrtcVTotal - 2) & 0x200) >> 4)
	| (((modetiming->CrtcVDisplay - 1) & 0x200) >> 3)
	| ((modetiming->CrtcVSyncStart & 0x200) >> 2);
    moderegs[VGA_CR8] = 0x00;
    moderegs[VGA_CR9] = ((modetiming->CrtcVSyncStart & 0x200) >> 4) | 0x40;
    if (modetiming->flags & DOUBLESCAN)
	moderegs[VGA_CR9] |= 0x80;
    moderegs[VGA_CRA] = 0x00;
    moderegs[VGA_CRB] = 0x00;
    moderegs[VGA_CRC] = 0x00;
    moderegs[VGA_CRD] = 0x00;
    moderegs[VGA_CRE] = 0x00;
    moderegs[VGA_CRF] = 0x00;
    moderegs[VGA_CR10] = modetiming->CrtcVSyncStart & 0xFF;
    moderegs[VGA_CR11] = (modetiming->CrtcVSyncEnd & 0x0F) | 0x20;
    moderegs[VGA_CR12] = (modetiming->CrtcVDisplay - 1) & 0xFF;
    moderegs[VGA_CR13] = modeinfo->lineWidth >> 4;	/* Just a guess. */
    moderegs[VGA_CR14] = 0x00;
    moderegs[VGA_CR15] = modetiming->CrtcVSyncStart & 0xFF;
    moderegs[VGA_CR16] = (modetiming->CrtcVSyncStart + 1) & 0xFF;
    moderegs[VGA_CR17] = 0xC3;
    if (modeinfo->bitsPerPixel == 4)
	moderegs[VGA_CR17] = 0xE3;
    moderegs[VGA_CR18] = 0xFF;

/* Graphics Controller */
    moderegs[VGA_GR0] = 0x00;
    moderegs[VGA_GR1] = 0x00;
    moderegs[VGA_GR2] = 0x00;
    moderegs[VGA_GR3] = 0x00;
    moderegs[VGA_GR4] = 0x00;
    moderegs[VGA_GR5] = 0x40;
    if (modeinfo->bitsPerPixel == 4)
	moderegs[VGA_GR5] = 0x02;
    moderegs[VGA_GR6] = 0x05;
    moderegs[VGA_GR7] = 0x0F;
    moderegs[VGA_GR8] = 0xFF;

/* Attribute Controller */
    for (i = 0; i < 16; i++)
	moderegs[VGA_AR0 + i] = i;
    moderegs[VGA_AR10] = 0x41;
    if (modeinfo->bitsPerPixel == 4)
	moderegs[VGA_AR10] = 0x01;	/* was 0x81 */
    /* Attribute register 0x11 is the overscan color. 
       Should have no affect in svga modes.           */
    moderegs[VGA_AR11] = 0x00;
    moderegs[VGA_AR12] = 0x0F;
    moderegs[VGA_AR13] = 0x00;
    moderegs[VGA_AR14] = 0x00;
}


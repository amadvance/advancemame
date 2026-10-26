// license:BSD-3-Clause
// copyright-holders:Angelo Salese, Miodrag Milanovic, Carl
/******************************************************************************

	i82720cm.h
	Video driver for the Intel 82720 and NEC uPD7220 GDC.

******************************************************************************/

#ifndef __I82720CM_H__
#define __I82720CM_H__

/* Command codes */
#define UPD7220_COMMAND_RESET           0x00
#define UPD7220_COMMAND_SYNC            0x0e // & 0xfe
#define UPD7220_COMMAND_VSYNC           0x6e // & 0xfe
#define UPD7220_COMMAND_CCHAR           0x4b
#define UPD7220_COMMAND_START           0x6b
#define UPD7220_COMMAND_BCTRL           0x0c // & 0xfe
#define UPD7220_COMMAND_05              0x05
#define UPD7220_COMMAND_ZOOM            0x46
#define UPD7220_COMMAND_CURS            0x49
#define UPD7220_COMMAND_PRAM            0x70 // & 0xf0
#define UPD7220_COMMAND_PITCH           0x47
#define UPD7220_COMMAND_WDAT            0x20 // & 0xe4
#define UPD7220_COMMAND_MASK            0x4a
#define UPD7220_COMMAND_FIGS            0x4c
#define UPD7220_COMMAND_FIGD            0x6c
#define UPD7220_COMMAND_GCHRD           0x68
#define UPD7220_COMMAND_RDAT            0xa0 // & 0xe4
#define UPD7220_COMMAND_CURD            0xe0
#define UPD7220_COMMAND_LPRD            0xc0
#define UPD7220_COMMAND_DMAR            0xa4 // & 0xe4
#define UPD7220_COMMAND_DMAW            0x24 // & 0xe4
#define UPD7220_COMMAND_5A              0x5a

/* Status register bits */
#define UPD7220_SR_DATA_READY           0x01
#define UPD7220_SR_FIFO_FULL            0x02
#define UPD7220_SR_FIFO_EMPTY           0x04
#define UPD7220_SR_DRAWING_IN_PROGRESS  0x08
#define UPD7220_SR_DMA_EXECUTE          0x10
#define UPD7220_SR_VSYNC_ACTIVE         0x20
#define UPD7220_SR_HBLANK_ACTIVE        0x40
#define UPD7220_SR_LIGHT_PEN_DETECT     0x80

/* Mode register flags */
#define UPD7220_MODE_REFRESH_RAM        0x04
#define UPD7220_MODE_DRAW_ON_RETRACE    0x10
#define UPD7220_MODE_DISPLAY_MASK       0x22
#define UPD7220_MODE_DISPLAY_MIXED      0x00
#define UPD7220_MODE_DISPLAY_GRAPHICS   0x02
#define UPD7220_MODE_DISPLAY_CHARACTER  0x20
#define UPD7220_MODE_DISPLAY_INVALID    0x22
#define UPD7220_MODE_INTERLACE_MASK     0x09
#define UPD7220_MODE_INTERLACE_NONE     0x00
#define UPD7220_MODE_INTERLACE_INVALID  0x01
#define UPD7220_MODE_INTERLACE_REPEAT   0x08
#define UPD7220_MODE_INTERLACE_ON       0x09

#endif

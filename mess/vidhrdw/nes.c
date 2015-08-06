/***************************************************************************

  vidhrdw/nes.c

  Routines to control the unique NES video hardware/PPU.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/ppu2c03b.h"
#include "vidhrdw/generic.h"
#include "includes/nes.h"
#include "machine/nes_mmc.h"

int nes_vram_sprite[8]; /* Used only by mmc5 for now */

static void ppu_nmi(int num, int *ppu_regs)
{
	cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

static void nes_vh_reset(void)
{
	ppu2c03b_reset( 0, 1 );
}

static int nes_vh_start(int ppu_scanlines_per_frame)
{
	ppu2c03b_interface ppu_interface;

	memset(&ppu_interface, 0, sizeof(ppu_interface));
	ppu_interface.num				= 1;
	ppu_interface.vrom_region[0]	= nes.chr_chunks ? REGION_GFX1 : REGION_INVALID;
	ppu_interface.mirroring[0]		= PPU_MIRROR_NONE;
	ppu_interface.nmi_handler[0]	= ppu_nmi;

	ppu2c03b_init(&ppu_interface);
	ppu2c03b_set_vidaccess_callback(0, nes_ppu_vidaccess);
	ppu2c03b_set_scanlines_per_frame(0, ppu_scanlines_per_frame);

	if (nes.four_screen_vram)
	{
		/* TODO: figure out what to do here */
	}
	else
	{
		switch(nes.hard_mirroring) {
		case 0:
			ppu2c03b_set_mirroring(0, PPU_MIRROR_HORZ);
			break;
		case 1:
			ppu2c03b_set_mirroring(0, PPU_MIRROR_VERT);
			break;
		}
	}

	add_reset_callback(nes_vh_reset);

	/* Reset the mapper variables. Will also mark the char-gen ram as dirty */
	mapper_reset (nes.mapper);
	return 0;
}

VIDEO_START( nes_ntsc )
{
	return nes_vh_start(NTSC_SCANLINES_PER_FRAME);
}

VIDEO_START( nes_pal )
{
	return nes_vh_start(PAL_SCANLINES_PER_FRAME);
}

PALETTE_INIT( nes )
{
	ppu2c03b_init_palette(0);
}

static void draw_sight(mame_bitmap *bitmap, int playerNum, int x_center, int y_center)
{
	int x,y;
	UINT16 color;

	if (playerNum == 2)
		color = Machine->pens[0]; /* grey */
	else
		color = Machine->pens[0x30]; /* white */

	if (x_center<2)   x_center=2;
	if (x_center>253) x_center=253;

	if (y_center<2)   y_center=2;
	if (y_center>253) y_center=253;

	for(y = y_center-5; y < y_center+6; y++)
		if((y >= 0) && (y < 256))
			plot_pixel (bitmap, x_center, y, color);

	for(x = x_center-5; x < x_center+6; x++)
		if((x >= 0) && (x < 256))
			plot_pixel (bitmap, x, y_center, color);
}

/***************************************************************************

  Display refresh

***************************************************************************/
VIDEO_UPDATE( nes )
{
	int sights = 0;

	/* render the ppu */
	ppu2c03b_render( 0, bitmap, 0, 0, 0, 0 );

	/* figure out what sights to draw, and draw them */
	if ((readinputport(PORT_CONFIG1) & 0x000f) == 0x0002)
		sights |= 0x0001;
	if ((readinputport(PORT_CONFIG1) & 0x000f) == 0x0003)
		sights |= 0x0002;
	if ((readinputport(PORT_CONFIG1) & 0x00f0) == 0x0020)
		sights |= 0x0001;
	if ((readinputport(PORT_CONFIG1) & 0x00f0) == 0x0030)
		sights |= 0x0002;
	if (sights & 0x0001)
		draw_sight(bitmap, 1, readinputport(PORT_ZAPPER0_X), readinputport(PORT_ZAPPER0_Y));
	if (sights & 0x0002)
		draw_sight(bitmap, 2, readinputport(PORT_ZAPPER1_X), readinputport(PORT_ZAPPER1_Y));

	/* if this is a disk system game, check for the flip-disk key */
	if (nes.mapper == 20)
	{
		if (readinputport(PORT_FLIPKEY) & 0x01)
		{
			nes_fds.current_side++;
			if (nes_fds.current_side > nes_fds.sides)
				nes_fds.current_side = 0;

			if (nes_fds.current_side == 0)
			{
				ui_popup("No disk inserted.");
			}
			else
			{
				ui_popup("Disk set to side %d", nes_fds.current_side);
			}
		}
	}
}

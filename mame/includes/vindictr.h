// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*************************************************************************

    Atari Vindicators hardware

*************************************************************************/

/*----------- defined in vidhrdw/vindictr.c -----------*/

WRITE16_HANDLER( vindictr_paletteram_w );

VIDEO_START( vindictr );
VIDEO_UPDATE( vindictr );

void vindictr_scanline_update(int scanline);

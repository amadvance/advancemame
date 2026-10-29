// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*************************************************************************

    Raster Elite Tickee Tickats hardware

**************************************************************************/

/*----------- defined in drivers/tickee.c -----------*/

extern UINT16 *tickee_control;


/*----------- defined in vidhrdw/tickee.c -----------*/

extern UINT16 *tickee_vram;

VIDEO_START( tickee );

VIDEO_UPDATE( tickee );

extern int rapidfir_palette_blank;
extern int rapidfir_has_two_guns;
extern UINT8 rapidfir_gunx[2];
VIDEO_START( mouseatk );
VIDEO_START( rapidfir );
VIDEO_UPDATE( mouseatk );
VIDEO_UPDATE( rapidfir );

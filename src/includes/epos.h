/*************************************************************************

    Epos games

**************************************************************************/

/*----------- defined in vidhrdw/epos.c -----------*/

PALETTE_INIT( epos );

WRITE8_HANDLER( epos_videoram_w );
WRITE8_HANDLER( epos_port_1_w );

VIDEO_UPDATE( epos );

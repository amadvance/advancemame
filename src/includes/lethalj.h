/*************************************************************************

    The Game Room Lethal Justice hardware

**************************************************************************/

/*----------- defined in vidhrdw/lethalj.c -----------*/

READ16_HANDLER( lethalj_gun_r );

VIDEO_START( lethalj );

WRITE16_HANDLER( lethalj_blitter_w );

VIDEO_UPDATE( lethalj );

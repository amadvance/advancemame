/*************************************************************************

    Atari Missile Command hardware

*************************************************************************/

/*----------- defined in drivers/missile.c -----------*/

extern UINT8 *missile_video2ram;
extern UINT8 *missile_ram;

READ8_HANDLER( missile_r );
WRITE8_HANDLER( missile_w );


/*----------- defined in vidhrdw/missile.c -----------*/

VIDEO_START( missile );
VIDEO_UPDATE( missile );

WRITE8_HANDLER( missile_video_3rd_bit_w );
WRITE8_HANDLER( missile_video2_w );

READ8_HANDLER( missile_video_r );
WRITE8_HANDLER( missile_video_w );
WRITE8_HANDLER( missile_video_mult_w );

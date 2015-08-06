/***************************************************************************

 Espial hardware games

***************************************************************************/

/*----------- defined in vidhrdw/espial.c -----------*/

extern UINT8 *espial_videoram;
extern UINT8 *espial_colorram;
extern UINT8 *espial_attributeram;
extern UINT8 *espial_scrollram;
extern UINT8 *espial_spriteram_1;
extern UINT8 *espial_spriteram_2;
extern UINT8 *espial_spriteram_3;

PALETTE_INIT( espial );
VIDEO_START( espial );
VIDEO_START( netwars );
WRITE8_HANDLER( espial_videoram_w );
WRITE8_HANDLER( espial_colorram_w );
WRITE8_HANDLER( espial_attributeram_w );
WRITE8_HANDLER( espial_scrollram_w );
WRITE8_HANDLER( espial_flipscreen_w );
VIDEO_UPDATE( espial );


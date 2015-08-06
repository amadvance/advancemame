/*----------- defined in drivers/cclimber.c -----------*/

DRIVER_INIT( cclimbrj );
void cclimber_decode(const unsigned char xortable[8][16]);

/*----------- defined in vidhrdw/cclimber.c -----------*/

extern unsigned char *cclimber_bsvideoram;
extern size_t cclimber_bsvideoram_size;
extern unsigned char *cclimber_bigspriteram;
extern unsigned char *cclimber_column_scroll;

extern UINT8 *toprollr_videoram2;
extern UINT8 *toprollr_videoram3;
extern UINT8 *toprollr_videoram4;

WRITE8_HANDLER( cclimber_colorram_w );
WRITE8_HANDLER( cclimber_bigsprite_videoram_w );
PALETTE_INIT( cclimber );
VIDEO_UPDATE( cclimber );

VIDEO_UPDATE( yamato );
VIDEO_START( toprollr );
VIDEO_UPDATE( toprollr );


/*----------- defined in sndhrdw/cclimber.c -----------*/

extern struct AY8910interface cclimber_ay8910_interface;
extern struct Samplesinterface cclimber_custom_interface;
WRITE8_HANDLER( cclimber_sample_trigger_w );
WRITE8_HANDLER( cclimber_sample_rate_w );
WRITE8_HANDLER( cclimber_sample_volume_w );

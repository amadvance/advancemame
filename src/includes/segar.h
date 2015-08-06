/*************************************************************************

    Sega G-80 raster hardware

*************************************************************************/

/*----------- defined in machine/segar.c -----------*/

extern UINT8 *segar_miscram1;
extern UINT8 *segar_miscram2;
extern UINT8 *segar_monsterbram;
extern UINT8 (*sega_decrypt)(offs_t, UINT8);

void sega_security(int chip);
WRITE8_HANDLER( segar_w );


/*----------- defined in sndhrdw/segar.c -----------*/

WRITE8_HANDLER( astrob_audio_ports_w );
WRITE8_HANDLER( spaceod_audio_ports_w );
WRITE8_HANDLER( monsterb_audio_8255_w );
 READ8_HANDLER( monsterb_audio_8255_r );

 READ8_HANDLER( monsterb_sh_rom_r );
 READ8_HANDLER( monsterb_sh_t1_r );
 READ8_HANDLER( monsterb_sh_command_r );
WRITE8_HANDLER( monsterb_sh_dac_w );
WRITE8_HANDLER( monsterb_sh_busy_w );
WRITE8_HANDLER( monsterb_sh_offset_a0_a3_w );
WRITE8_HANDLER( monsterb_sh_offset_a4_a7_w );
WRITE8_HANDLER( monsterb_sh_offset_a8_a11_w );
WRITE8_HANDLER( monsterb_sh_rom_select_w );

/* sample names */
extern const char *astrob_sample_names[];
extern const char *s005_sample_names[];
extern const char *monsterb_sample_names[];
extern const char *spaceod_sample_names[];


/*----------- defined in vidhrdw/segar.c -----------*/

extern UINT8 *segar_characterram;
extern UINT8 *segar_characterram2;
extern UINT8 *segar_mem_colortable;
extern UINT8 *segar_mem_bcolortable;

WRITE8_HANDLER( segar_characterram_w );
WRITE8_HANDLER( segar_characterram2_w );
WRITE8_HANDLER( segar_colortable_w );
WRITE8_HANDLER( segar_bcolortable_w );

WRITE8_HANDLER( segar_video_port_w );

PALETTE_INIT( segar );
VIDEO_START( segar );
VIDEO_UPDATE( segar );

WRITE8_HANDLER( monsterb_back_port_w );
WRITE8_HANDLER( monster2_b9_back_port_w );
WRITE8_HANDLER( monster2_bb_back_port_w );

VIDEO_START( monsterb );
VIDEO_UPDATE( monsterb );

WRITE8_HANDLER( spaceod_back_port_w );
WRITE8_HANDLER( spaceod_backshift_w );
WRITE8_HANDLER( spaceod_backshift_clear_w );
WRITE8_HANDLER( spaceod_backfill_w );
WRITE8_HANDLER( spaceod_nobackfill_w );

VIDEO_START( spaceod );
VIDEO_UPDATE( spaceod );

WRITE8_HANDLER( pignewt_back_color_w );
WRITE8_HANDLER( pignewt_back_ports_w );

WRITE8_HANDLER( sindbadm_back_port_w );

VIDEO_UPDATE( sindbadm );

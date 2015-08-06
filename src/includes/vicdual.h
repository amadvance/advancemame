/*************************************************************************

    vicdual.h

*************************************************************************/

#include "sound/discrete.h"

/*----------- defined in drivers/vicdual.c -----------*/

extern mame_timer *croak_timer;

/*----------- defined in vidhrdw/vicdual.c -----------*/

extern unsigned char *vicdual_characterram;
PALETTE_INIT( vicdual );
WRITE8_HANDLER( vicdual_characterram_w );
READ8_HANDLER( vicdual_characterram_r );
WRITE8_HANDLER( vicdual_palette_bank_w );
VIDEO_UPDATE( vicdual );

/*----------- defined in sndhrdw/carnival.c -----------*/

extern const char *carnival_sample_names[];
WRITE8_HANDLER( carnival_sh_port1_w );
WRITE8_HANDLER( carnival_sh_port2_w );
READ8_HANDLER( carnival_music_port_t1_r );
WRITE8_HANDLER( carnival_music_port_1_w );
WRITE8_HANDLER( carnival_music_port_2_w );

/*----------- defined in sndhrdw/depthch.c -----------*/

extern const char *depthch_sample_names[];
WRITE8_HANDLER( depthch_sh_port1_w );

/*----------- defined in sndhrdw/invinco.c -----------*/

extern const char *invinco_sample_names[];
WRITE8_HANDLER( invinco_sh_port2_w );

/*----------- defined in sndhrdw/pulsar.c -----------*/

extern const char *pulsar_sample_names[];
WRITE8_HANDLER( pulsar_sh_port1_w );
WRITE8_HANDLER( pulsar_sh_port2_w );

/*----------- defined in sndhrdw/vicdual.c -----------*/

WRITE8_HANDLER( frogs_sh_port2_w );
void croak_callback(int param);

extern struct Samplesinterface frogs_samples_interface;
extern struct discrete_sound_block frogs_discrete_interface[];

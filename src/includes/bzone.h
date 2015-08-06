/*************************************************************************

    Atari Battle Zone hardware

*************************************************************************/

#include "sound/custom.h"


/*----------- defined in drivers/bzone.c -----------*/

READ8_HANDLER( bzone_IN0_r );


/*----------- defined in sndhrdw/bzone.c -----------*/

WRITE8_HANDLER( bzone_sounds_w );

void *bzone_sh_start(int clock, const struct CustomSound_interface *config);


/*----------- defined in sndhrdw/redbaron.c -----------*/

WRITE8_HANDLER( redbaron_sounds_w );
WRITE8_HANDLER( redbaron_pokey_w );

void *redbaron_sh_start(int clock, const struct CustomSound_interface *config);

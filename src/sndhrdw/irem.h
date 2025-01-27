/***************************************************************************

    Irem audio interface

****************************************************************************/
#include "sound/samples.h"

MACHINE_DRIVER_EXTERN( irem_audio );

WRITE8_HANDLER( irem_sound_cmd_w );

extern struct Samplesinterface tr606_samples_interface;

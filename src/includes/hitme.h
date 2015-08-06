/*************************************************************************

    Hitme hardware

*************************************************************************/

#include "sound/discrete.h"

/* Discrete Sound Input Nodes */
#define HITME_DOWNCOUNT_VAL		NODE_01
#define HITME_OUT0				NODE_02
#define HITME_ENABLE_VAL		NODE_03
#define HITME_OUT1				NODE_04


/*----------- defined in sndhrdw/hitme.c -----------*/

extern struct discrete_sound_block hitme_discrete_interface[];

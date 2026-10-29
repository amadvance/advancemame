// license:BSD-3-Clause
// copyright-holders:Derrick Renaud
#ifndef M79AMB_H
#define M79AMB_H

#include "sound/discrete.h"

#define M79AMB_BOOM_EN NODE_01
#define M79AMB_THUD_EN NODE_02
#define M79AMB_SHOT_EN NODE_03
#define M79AMB_MC_REV_EN NODE_04
#define M79AMB_MC_CONTROL_EN NODE_05
#define M79AMB_TANK_TRUCK_JEEP_EN NODE_06
#define M79AMB_WHISTLE_A_EN NODE_07
#define M79AMB_WHISTLE_B_EN NODE_08

extern const struct discrete_sound_block m79amb_discrete_interface[];
WRITE8_HANDLER( ramtek_mask_w );
WRITE8_HANDLER( ramtek_videoram_w );
VIDEO_UPDATE( m79amb );

#endif

// license:BSD-3-Clause
// copyright-holders:Derrick Renaud
#ifndef SKYRAID_H
#define SKYRAID_H
#include "sound/discrete.h"
#define SKYRAID_PLANE_SWEEP_EN NODE_01
#define SKYRAID_MISSILE_EN NODE_02
#define SKYRAID_EXPLOSION_EN NODE_03
#define SKYRAID_PLANE_ON_EN NODE_04
#define SKYRAID_PLANE_ON__IC_E8 SKYRAID_PLANE_ON_EN
#define SKYRAID_ATTRACT_EN NODE_05
extern struct discrete_sound_block skyraid_discrete[];
#endif

// license:BSD-3-Clause
// copyright-holders:Aaron Giles
#ifndef LASERDSC_H
#define LASERDSC_H
#include "ldv1000.h"
void laserdisc_raw_reset(void);
void laserdisc_raw_advance(int delta);
int laserdisc_raw_track(void);
int laserdisc_raw_at_limit(void);
UINT32 laserdisc_raw_code(int line);
void laserdisc_raw_field(int field_number, int video_enabled, int audio_enabled);
#endif

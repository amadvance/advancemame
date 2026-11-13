// license:BSD-3-Clause
// copyright-holders:Aaron Giles
#ifndef LASERDSC_H
#define LASERDSC_H
#include "ldv1000.h"
void laserdisc_raw_set_audio_hook(void (*hook)(int sample_rate, int samples, const INT16 *ch1));
void laserdisc_raw_reset(void);
void laserdisc_raw_advance(int delta);
int laserdisc_raw_track(void);
int laserdisc_raw_at_limit(void);
UINT32 laserdisc_raw_code(int line);
void laserdisc_raw_field_stereo(int field_number, int video_enabled, int audio_mask);
void laserdisc_raw_field(int field_number, int video_enabled, int audio_enabled);
#endif

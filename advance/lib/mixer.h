/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. 
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

/** \file
 * Mixer.
 */

#ifndef __MIXER_H
#define __MIXER_H

#include "conf.h"
#include "fz.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup Mixer */
/*@{*/

#define MIXER_CHANNEL_MAX 8 /**< Max number of channels. */

void mixer_reg(adv_conf* context);
adv_error mixer_load(adv_conf* context);

adv_error mixer_init(unsigned rate, unsigned nchannel, unsigned ndivider, double buffer_time, double latency_time);
void mixer_done(void);

void mixer_poll(void);

void mixer_volume(double volume);

adv_error mixer_play_file_wav(unsigned channel, adv_fz* file, int loop);
adv_error mixer_play_memory_wav(unsigned channel, const unsigned char* begin, const unsigned char* end, int loop);
adv_error mixer_play_file_mp3(unsigned channel, adv_fz* file, int loop);

void mixer_stop(unsigned channel);

adv_bool mixer_is_pushing(unsigned channel);
adv_bool mixer_is_playing(unsigned channel);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif

/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003 Andrea Mazzoleni
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

#ifndef __HSCRIPT_H
#define __HSCRIPT_H

#include "conf.h"

/* Scripts ID */
#define HARDWARE_SCRIPT_VIDEO 0
#define HARDWARE_SCRIPT_EMULATION 1
#define HARDWARE_SCRIPT_PLAY 2
#define HARDWARE_SCRIPT_LED1 3
#define HARDWARE_SCRIPT_LED2 4
#define HARDWARE_SCRIPT_LED3 5
#define HARDWARE_SCRIPT_COIN1 6
#define HARDWARE_SCRIPT_COIN2 7
#define HARDWARE_SCRIPT_COIN3 8
#define HARDWARE_SCRIPT_COIN4 9
#define HARDWARE_SCRIPT_START1 10
#define HARDWARE_SCRIPT_START2 11
#define HARDWARE_SCRIPT_START3 12
#define HARDWARE_SCRIPT_START4 13
#define HARDWARE_SCRIPT_TURBO 14
#define HARDWARE_SCRIPT_SAFEQUIT 15
#define HARDWARE_SCRIPT_EVENT1 16
#define HARDWARE_SCRIPT_EVENT2 17
#define HARDWARE_SCRIPT_EVENT3 18
#define HARDWARE_SCRIPT_EVENT4 19
#define HARDWARE_SCRIPT_EVENT5 20
#define HARDWARE_SCRIPT_EVENT6 21
#define HARDWARE_SCRIPT_EVENT7 22
#define HARDWARE_SCRIPT_EVENT8 23
#define HARDWARE_SCRIPT_EVENT9 24
#define HARDWARE_SCRIPT_EVENT10 25
#define HARDWARE_SCRIPT_EVENT11 26
#define HARDWARE_SCRIPT_EVENT12 27
#define HARDWARE_SCRIPT_EVENT13 28
#define HARDWARE_SCRIPT_EVENT14 29

/* Max number of scripts */
#define HARDWARE_SCRIPT_MAX 30

void hardware_script_set(int id, const char* script);

int hardware_script_config_load(adv_conf* context);

int hardware_script_init(adv_conf* context);
void hardware_script_done(void);
int hardware_script_inner_init(void);
void hardware_script_inner_done(void);

void hardware_script_abort(void);

void hardware_script_start(int id);
void hardware_script_stop(int id);
void hardware_script_terminate(int id);

void hardware_script_idle(unsigned time_to_play);

void hardware_script_info(const char* desc, const char* manufacturer, const char* year, const char* throttle);

/***************************************************************************/
/* simulate */

/* Size of the simulate buffer */
#define SIMULATE_MAX 2

struct simulate {
	int type; /* event type */
	unsigned time_to_play; /* time remaining */
};

/* Simulation for events */
extern struct simulate SIMULATE_EVENT[];

/* Simulation for keys */
extern struct simulate SIMULATE_KEY[];

void hardware_simulate_input(struct simulate* SIMULATE, int type, unsigned time_to_play);
void hardware_simulate_input_idle(struct simulate* SIMULATE, unsigned time_to_play);

static inline int hardware_is_input_simulated(const struct simulate* SIMULATE, int type)
{
	if (SIMULATE[0].time_to_play != 0 && SIMULATE[0].type == type)
		return 1;
#if SIMULATE_MAX > 1
	if (SIMULATE[1].time_to_play != 0 && SIMULATE[1].type == type)
		return 1;
#endif
#if SIMULATE_MAX > 2
	if (SIMULATE[2].time_to_play != 0 && SIMULATE[2].type == type)
		return 1;
#endif
	return 0;
}

#endif

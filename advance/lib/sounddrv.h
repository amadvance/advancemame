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
 * Sound drivers.
 */

/** \addtogroup Sound */
/*@{*/

#ifndef __SOUNDDRV_H
#define __SOUNDDRV_H

#include "device.h"
#include "conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Driver */

/** \name Sound Flags
 * Sound driver flags for volume.
 */
/*@{*/
#define SOUND_DRIVER_FLAGS_VOLUME_SAMPLE 0x1 /**< Fake volume control made changing the samples. */
/*@}*/

#define SOUND_DRIVER_FLAGS_USER_BIT0 0x10000
#define SOUND_DRIVER_FLAGS_USER_MASK 0xFFFF0000

/**
 * 16 bit signed sound sample.
 */
typedef short adv_sample;

/**
 * Sound driver.
 * This struct abstract all the driver funtionalities.
 */
typedef struct soundb_driver_struct {
	const char* name; /**< Name of the driver */
	const adv_device* device_map; /**< List of supported devices */

	/** Load the configuration options. Call before init() */
	adv_error (*load)(adv_conf* context);

	/** Register the load options. Call before load(). */
	void (*reg)(adv_conf* context);

	adv_error (*init)(int device_id, unsigned* rate, adv_bool stereo_flag, double buffer_time); /**< Initialize the driver */
	void (*done)(void); /**< Deinitialize the driver */

	unsigned (*flags)(void); /**< Get the capabilities of the driver */

	void (*play)(const adv_sample* sample_map, unsigned sample_count);
	unsigned (*buffered)(void);
	adv_error (*start)(double silence_time);
	void (*stop)(void);
	void (*volume)(double v);
} soundb_driver;

#define SOUND_DRIVER_MAX 8

struct soundb_state_struct {
	adv_bool is_initialized_flag;
	adv_bool is_active_flag;
	adv_bool is_playing_flag;
	unsigned driver_mac;
	soundb_driver* driver_map[SOUND_DRIVER_MAX];
	soundb_driver* driver_current;
	char name[DEVICE_NAME_MAX];
};

extern struct soundb_state_struct soundb_state;

void soundb_reg(adv_conf* config_context, adv_bool auto_detect);
void soundb_reg_driver(adv_conf* config_context, soundb_driver* driver);
adv_error soundb_load(adv_conf* config_context);
adv_error soundb_init(unsigned* rate, adv_bool stereo_flag, double buffer_time);
void soundb_done(void);
void soundb_abort(void);

/**
 * If the sound library is initialized.
 */
adv_bool soundb_is_active(void);

/**
 * If the sound library is playing.
 */
adv_bool soundb_is_playing(void);

/**
 * Get the capabilities of the driver.
 */
unsigned soundb_flags(void);

/**
 * Play the specified samples.
 * \param sample_map Samples to play.
 * \param sample_count Number of samples to play. If stereo is enable sample_map must contains the 2*sample_count samples.
 */
void soundb_play(const adv_sample* sample_map, unsigned sample_count);

/**
 * Return the number of buffered samples.
 */
unsigned soundb_buffered(void);

/**
 * Stop the playing.
 */
void soundb_stop(void);

/**
 * Start the playing.
 * The buffer is filled with the specified amount of silence.
 * \param silence_time Silence time.
 */
adv_error soundb_start(double silence_time);

/**
 * Set the output volume.
 * \param v Volume. 1 is the maximum. 0 is silence.
 */
void soundb_volume(double v);

/**
 * Get the driver/device name.
 * \return Pointer at a static buffer.
 */
const char* soundb_name(void);

#ifdef __cplusplus
}
#endif

#endif

/*@}*/

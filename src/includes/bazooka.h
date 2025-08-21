/***************************************************************************

    Bazooka (Taito do Brasil), 1977, TTL
    Driver by Antonio "Nino MegaDriver" Tornisiello.

**************************************************************************/

#pragma once
#ifndef __BAZOOKA_H__
#define __BAZOOKA_H__

#include "driver.h"

MACHINE_DRIVER_EXTERN(bazooka);

ADDRESS_MAP_EXTERN( bazooka_readport );

DRIVER_INIT( bazooka );
PALETTE_INIT( bazooka );
VIDEO_START( bazooka );
VIDEO_UPDATE( bazooka );
INTERRUPT_GEN( bazooka_interrupt );

extern int bazooka_effect_timer;

/* Main driver functions */
void bazooka_video_irq(void);
void bazooka_video_reset(void);

/* Sound functions */
void bazooka_sound_off(void);
void bazooka_vehicle_sound(int line, int freq, int volume);
void bazooka_vehicle_sound_off(int line);
void bazooka_tank_sound(int volume);
void bazooka_tank_sound_off(void);
void bazooka_ambulance_sound(int freq, int volume);
void bazooka_ambulance_sound_off(void);
void bazooka_effects_sound(int type);
void bazooka_noise_set_volume(int volume);

#endif /* __BAZOOKA_H__ */

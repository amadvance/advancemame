/***************************************************************************

    Bazooka (Taito do Brasil), 1977, TTL
    Driver by Antonio "Nino MegaDriver" Tornisiello.

    Simulated Audio Hardware

    Original game hardware set had it's own analog sound card for generating
    each vehicle sound and the fire and explosion FX, using simple circuitry
    basic TTLs and counters. For vehicles and fire discrete frequencies are
    generated, for the explosion it plays everything at the same time to
    simulate a noise effect.

    This implementation uses a four-chip setup for simulation:
    - SN76496 #0 @ 640kHz:  For standard vehicle tones (Jeep, Truck, etc.).
    - SN76496 #1 @ 640kHz:  For 80Hz low-frequency tank sound.
    - SN76496 #2 @ 3.57MHz: For fire and explosion noise effects.
    - SN76496 #3 @ 3.57MHz: For ambulance siren.

**************************************************************************/

#include "driver.h"
#include "sound/sn76496.h"
#include "bazooka.h"

int bazooka_effect_timer = 0;

/* Plays vehicle sound on Chip #0. Handles lines 0, 1, and 2. */
void bazooka_vehicle_sound(int line, int freq, int volume)
{

  /* Invert and clamp our 0-15 volume scale to the chip's 15-0 scale */
  int sn_volume = 15 - (volume & 0x0f);

  switch (line)
  {
    case 0: /* Line 0 -> Chip 0, Channel 0 */
      SN76496_0_w(0, 0x80 | (freq & 0x0f));
      SN76496_0_w(0, (freq >> 4) & 0x3f);
      SN76496_0_w(0, 0x90 | sn_volume);
      break;
    case 1: /* Line 1 -> Chip 0, Channel 1 */
      SN76496_0_w(0, 0xA0 | (freq & 0x0f));
      SN76496_0_w(0, (freq >> 4) & 0x3f);
      SN76496_0_w(0, 0xB0 | sn_volume);
      break;
    case 2: /* Line 2 -> Chip 0, Channel 2 */
      SN76496_0_w(0, 0xC0 | (freq & 0x0f));
      SN76496_0_w(0, (freq >> 4) & 0x3f);
      SN76496_0_w(0, 0xD0 | sn_volume);
      break;
  }
}

/* Turns off the sound for a specific vehicle line on Chip #0 */
void bazooka_vehicle_sound_off(int line)
{
  switch (line)
  {
    case 0: SN76496_0_w(0, 0x9F); break;
    case 1: SN76496_0_w(0, 0xBF); break;
    case 2: SN76496_0_w(0, 0xDF); break;
  }
}

/* Plays a pure 80 Hz tone on the Tank chip with specified volume. */
void bazooka_tank_sound(int volume)
{
  int divisor = 500;
  int sn_volume = 15 - (volume & 0x0f);

  /* Program Tone Channel 0 on Chip #1 */
  SN76496_1_w(0, 0x80 | (divisor & 0x0f));
  SN76496_1_w(0, (divisor >> 4) & 0x3f);
  SN76496_1_w(0, 0x90 | sn_volume);
}

/* Mutes the tank sound channel on Chip #1 */
void bazooka_tank_sound_off(void)
{
  SN76496_1_w(0, 0x9F);
}

/* Plays the ambulance siren on its dedicated chip (Chip #3) */
void bazooka_ambulance_sound(int freq, int volume)
{
  int sn_volume = 15 - (volume & 0x0f);
  /* Use Tone Channel 0 on Chip #3 */
  SN76496_3_w(0, 0x80 | (freq & 0x0f));
  SN76496_3_w(0, (freq >> 4) & 0x3f);
  SN76496_3_w(0, 0x90 | sn_volume);
}

/* Mutes the ambulance sound channel on Chip #3 */
void bazooka_ambulance_sound_off(void)
{
  SN76496_3_w(0, 0x9F);
}

/* Plays fire/explosion on the dedicated effects chip (Chip #2) */
void bazooka_effects_sound(int type)
{
  if (type == 0) // Fire
  {
    /* Generate a high-frequency NOISE ("hiss"), not a tone */
    SN76496_2_w(0, 0b11100000 | 0b00000111);
    /* Set the noise volume to maximum */
    bazooka_noise_set_volume(15);
  } else {// Explosion
    /* The explosion still uses low-frequency white noise */
    SN76496_2_w(0, 0xE0 | 0x06);
    SN76496_2_w(0, 0xF0 | 0);
    bazooka_effect_timer = 15;
  }
}

/* Sets the volume for the Noise channel on the effects chip */
void bazooka_noise_set_volume(int volume)
{
  int sn_volume = 15 - (volume & 0x0f);
  /* The noise channel volume is set with the 0xF_ command */
  SN76496_2_w(0, 0xF0 | sn_volume);
}

/* Mutes all channels on all chips */
void bazooka_sound_off(void)
{
    /* Mute Chip 0 (Vehicles) */
    SN76496_0_w(0, 0x9F);
    SN76496_0_w(0, 0xBF);
    SN76496_0_w(0, 0xDF);
    /* Mute Chip 1 (Tank) */
    SN76496_1_w(0, 0x9F);
    /* Mute Chip 2 (Effects) */
    /* THE FIX: Mute the tone channel used for fire, as well as the noise channel */
    SN76496_2_w(0, 0x9F);
    SN76496_2_w(0, 0xFF);
    /* Mute Chip 3 (Ambulance) */
    SN76496_3_w(0, 0x9F);
}

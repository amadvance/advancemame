/***************************************************************************

    Bazooka (Taito do Brasil), 1977, TTL
    Driver by Antonio "Nino MegaDriver" Tornisiello.

    Simulated Video Hardware and Game Logic, which
    also triggers audio events

**************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "bazooka.h"

/* Global variables for video simulation */
static int bazooka_line_position[4];
static int bazooka_line_vehicle[4];
static int bazooka_line_vehicle_width[4];
static int bazooka_line_exploding[4];
static int bazooka_frame_pol;
static int bazooka_animation_counter;
static int bazooka_bazooka_pos;
static int bazooka_play_time;
static long bazooka_score, bazooka_high_score;
static int bazooka_game_state;
static int bazooka_screen_width = 256;
static int ambulance_timer = 0;
static int ambulance_swap = 0;
static int ambulance_on_screen_last_frame = 0;
static int fire_sound_volume;
static int fire_sound_decay_counter;

/* State variables to simulate the TTL vehicle generation logic */
static int bazooka_common_vehicles[4];     /* The buffer for common vehicles: 0, 1, 2, 3 */
static int bazooka_common_vehicle_idx;     /* The index for the next common vehicle to spawn */
static int bazooka_vehicles_since_special; /* Counter for when to spawn a special vehicle */
static int bazooka_next_special_vehicle;   /* Toggles between Ambulance (5) and Stretcher (4) */

/* Bullet state controllers */
static int bazooka_bullet_active;
static int bazooka_bullet_x, bazooka_bullet_y;
static int bazooka_bonus_time_awarded;

/* Starting position for vehicles */
void bazooka_video_reset(void){
  int i;
  for(i=0;i<4; i++) bazooka_line_position[i] = rand() % 256;
  for(i=0;i<4; i++) bazooka_line_vehicle[i] =  i;
}

/* Draw a number char */
static void bazooka_draw_number(mame_bitmap * bitmap, int x, int y, int value, int length) {
  char buffer[10];
  int i;
  sprintf(buffer, "%0*d", length, value);
  for (i = 0; i < length; i++) drawgfx(bitmap, Machine -> gfx[2], buffer[i] - '0', 0, 0, 0, x + i * 7, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
}

/* Draw the "Game Over" char */
static void bazooka_draw_gameover(mame_bitmap * bitmap) {
  int x = 4;
  int game_y = bazooka_screen_width - 36;
  int over_y = bazooka_screen_width - 28;
  drawgfx(bitmap, Machine -> gfx[3], 1, 0, 0, 0, x, game_y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[3], 2, 0, 0, 0, x + 7, game_y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[3], 3, 0, 0, 0, x + 14, game_y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[3], 4, 0, 0, 0, x + 21, game_y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[3], 5, 0, 0, 0, x, over_y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[3], 6, 0, 0, 0, x + 7, over_y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[3], 7, 0, 0, 0, x + 14, over_y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[3], 8, 0, 0, 0, x + 21, over_y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
}

/* Draw a Jeep vehicle */
static void bazooka_draw_jeep(mame_bitmap * bitmap, int x, int y, int frame) {
  drawgfx(bitmap, Machine -> gfx[0], 5 + (16 * frame), 0, 0, 0, x, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[0], 8 + (16 * frame), 0, 0, 0, x + 8, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
}

/* Draw a Truck vehicle */
static void bazooka_draw_truck(mame_bitmap * bitmap, int x, int y, int frame) {
  drawgfx(bitmap, Machine -> gfx[0], 14 + (16 * frame), 0, 0, 0, x, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[0], 10 + (16 * frame), 0, 0, 0, x + 8, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[0], 12 + (16 * frame), 0, 0, 0, x + 16, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
}

/* Draw a Motorcycle */
static void bazooka_draw_bike(mame_bitmap * bitmap, int x, int y, int frame) {
  drawgfx(bitmap, Machine -> gfx[0], 15 + (16 * frame), 0, 0, 0, x, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[0], 13 + (16 * frame), 0, 0, 0, x + 8, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
}

/* Draw a Tank vehicle */
static void bazooka_draw_tank(mame_bitmap * bitmap, int x, int y, int frame) {
  drawgfx(bitmap, Machine -> gfx[0], 6 + (16 * frame), 0, 0, 0, x, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[0], 4 + (16 * frame), 0, 0, 0, x + 8, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[0], 2 + (16 * frame), 0, 0, 0, x + 16, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[0], 0 + (16 * frame), 0, 0, 0, x + 24, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
}

/* Draw an Ambulance vehicle */
static void bazooka_draw_ambulance(mame_bitmap * bitmap, int x, int y, int frame) {
  drawgfx(bitmap, Machine -> gfx[1], 6 + (16 * frame), 0, 0, 0, x, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[1], 4 + (16 * frame), 0, 0, 0, x + 8, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[1], 2 + (16 * frame), 0, 0, 0, x + 16, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[1], 0 + (16 * frame), 0, 0, 0, x + 24, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
}

/* Draws the two people carying a stretcher */
static void bazooka_draw_stretcher(mame_bitmap * bitmap, int x, int y, int frame) {
  drawgfx(bitmap, Machine -> gfx[1], 7 + (16 * frame), 0, 0, 0, x, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[1], 5 + (16 * frame), 0, 0, 0, x + 8, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[1], 3 + (16 * frame), 0, 0, 0, x + 16, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[1], 1 + (16 * frame), 0, 0, 0, x + 24, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
}

/* Draws the explosion wide sprite */
static void bazooka_draw_explosion(mame_bitmap * bitmap, int x, int y) {
  drawgfx(bitmap, Machine -> gfx[1], 30, 0, 0, 0, x, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[1], 31, 0, 0, 0, x, y + 16, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[1], 28, 0, 0, 0, x + 8, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[1], 29, 0, 0, 0, x + 8, y + 16, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[1], 26, 0, 0, 0, x + 16, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[1], 27, 0, 0, 0, x + 16, y + 16, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[1], 24, 0, 0, 0, x + 24, y, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
  drawgfx(bitmap, Machine -> gfx[1], 25, 0, 0, 0, x + 24, y + 16, & Machine -> visible_area, TRANSPARENCY_PEN, 0);
}

/* Simulated Game Logic: New Game */
static void bazooka_new_game(void)
{
	int i;

  // Reset Game Variables
  bazooka_sound_off();
	bazooka_game_state = 1;
  bazooka_play_time = 99;
  bazooka_score = 0;
	bazooka_bullet_active = 0;
  bazooka_bonus_time_awarded = 0;
  bazooka_bazooka_pos = 128;
  fire_sound_volume = 0;
  fire_sound_decay_counter = 0;

	for(i=0; i<4; i++) bazooka_line_exploding[i] = 0;

  /* 
   * Initialize the vehicle generator state
   * Equivalent to reset counters and registers on the board
   *
   * */
  bazooka_common_vehicles[0] = 0; /* Jeep */
  bazooka_common_vehicles[1] = 1; /* Truck */
  bazooka_common_vehicles[2] = 2; /* Bike */
  bazooka_common_vehicles[3] = 3; /* Tank */
  bazooka_common_vehicle_idx = 0;
  bazooka_vehicles_since_special = 0;
  
  /* 
   * 
   * Game always spawns an ambulance on start,
   * key feature when first decoding
   * 
   * */
  bazooka_next_special_vehicle = 5;
  ambulance_on_screen_last_frame = 0;

}

/* 
 * Bazooka is a 1bpp black and white game 
 * But we're creating a new color index
 * for our virtual "bazooka" on screen
 *
 * */
PALETTE_INIT( bazooka )
{
	/* Define the 4 pens (colors) we need */
	palette_set_color(0,0,0,0);         /* Pen 0: Black (for White Set) */
	palette_set_color(1,255,255,255);   /* Pen 1: White */
	palette_set_color(2,0,0,0);         /* Pen 2: Black (for Bazooka Set) */
	palette_set_color(3,50,50,50);     /* Pen 3: Bazooka Color */
	colortable[0] = 0; /* Sprite data 0 -> Use Pen 0 (Black) */
	colortable[1] = 1; /* Sprite data 1 -> Use Pen 1 (White) */
	colortable[2] = 2; /* Sprite data 0 -> Use Pen 2 (Black) */
	colortable[3] = 3; /* Sprite data 1 -> Use Pen 3 (Green) */
}

/* Check if an ambulance or stretcher is already on any other line */
static int is_special_vehicle_on_screen(int exclude_line) {
  int i;
  for (i = 0; i < 4; i++) {
    if (i == exclude_line) continue;
    if (bazooka_line_vehicle[i] == 4 || bazooka_line_vehicle[i] == 5) {
      return 1;
    }
  }
  return 0;
}

/* 
 * The game splits the first half of the screen in four lines
 * this simulates the logic of a line being processed
 *
 * */
static void bazooka_do_line(mame_bitmap * bitmap, int line) {

  int frame_step = 0;
  int vehicle_type = bazooka_line_vehicle[line];

  if (bazooka_line_exploding[line] > 0) {
    bazooka_draw_explosion(bitmap, bazooka_line_position[line], line * 24);
    bazooka_line_exploding[line]--;
    if (bazooka_line_exploding[line] == 0) bazooka_line_position[line] = -100;
    return;
  }

  // Configuring speeds
  switch (vehicle_type) {
    case 0: // Jeep
      frame_step = -3;
      bazooka_line_vehicle_width[line] = 16;
      bazooka_draw_jeep(bitmap, bazooka_line_position[line], line * 24, bazooka_frame_pol);
      break;
    case 1: // Truck
      frame_step = -2;
      bazooka_line_vehicle_width[line] = 24;
      bazooka_draw_truck(bitmap, bazooka_line_position[line], line * 24, bazooka_frame_pol);
      break;
    case 2: // Motorcycle
      frame_step = -4;
      bazooka_line_vehicle_width[line] = 16;
      bazooka_draw_bike(bitmap, bazooka_line_position[line], line * 24, bazooka_frame_pol);
      break;
    case 3:
      frame_step = -1; // Tank
      bazooka_line_vehicle_width[line] = 32;
      bazooka_draw_tank(bitmap, bazooka_line_position[line], line * 24, bazooka_frame_pol);
      break;
    case 4:
      frame_step = -2; // Stretcher
      bazooka_line_vehicle_width[line] = 32;
      bazooka_draw_stretcher(bitmap, bazooka_line_position[line], line * 24, bazooka_frame_pol);
      break;
    case 5: // Ambulance
      frame_step = -3;
      bazooka_line_vehicle_width[line] = 32;
      bazooka_draw_ambulance(bitmap, bazooka_line_position[line], line * 24, bazooka_frame_pol);
      break;
  }

  // Shifts the line;
  bazooka_line_position[line] += frame_step;

  /* Respawn vehicles when they go off-screen */
  if ((bazooka_line_position[line] < -50) || (bazooka_line_position[line] > bazooka_screen_width)) {
    if (frame_step < 0) bazooka_line_position[line] = bazooka_screen_width;
    else bazooka_line_position[line] = -32;

    /* 
     * TTL-based vehicle generation logic simulation
     *
     * "Positive Point" or "common" vehicles have a starting order
     * Ex: "A B C D"
     *
     * When a "Negative Point" or "special" vehicle is generated,
     * the game switches the "common" order.
     * Ex: "A B C D", switches into "B A D C"
     *
     * */
    int new_vehicle;
    int spawn_special = 0;

    if (bazooka_vehicles_since_special >= 4) {
        if (!is_special_vehicle_on_screen(line)) {
            spawn_special = 1;
        }
    }

    if (spawn_special)
    {
        new_vehicle = bazooka_next_special_vehicle;
        bazooka_next_special_vehicle = (bazooka_next_special_vehicle == 5) ? 4 : 5;
        bazooka_vehicles_since_special = 0;

        int temp;
        temp = bazooka_common_vehicles[0];
        bazooka_common_vehicles[0] = bazooka_common_vehicles[1];
        bazooka_common_vehicles[1] = temp;
        temp = bazooka_common_vehicles[2];
        bazooka_common_vehicles[2] = bazooka_common_vehicles[3];
        bazooka_common_vehicles[3] = temp;
    }
    else
    {
        new_vehicle = bazooka_common_vehicles[bazooka_common_vehicle_idx];
        bazooka_common_vehicle_idx = (bazooka_common_vehicle_idx + 1) % 4;
        bazooka_vehicles_since_special++;
    }
    /* Update the "line" to the resulting vehicle */
    bazooka_line_vehicle[line] = new_vehicle;
  }
}

VIDEO_UPDATE( bazooka )
{
	int i, x;
	fillbitmap(bitmap, get_black_pen(), cliprect);

  /* Sets up the fire sound fx decay effect */
	if (fire_sound_volume > 0)
	{
		fire_sound_decay_counter++;
		if (fire_sound_decay_counter >= 4)
		{
			fire_sound_decay_counter = 0;
			fire_sound_volume--;
			bazooka_noise_set_volume(fire_sound_volume);
		}
	}

  /* Check for a hard reset */
	if (readinputport(0) & 0x10)
	{
		/* Call all the initialization functions to perform a true hard reset */
		bazooka_new_game();
		bazooka_video_reset();
		/* Set the game state to 0 to show the "Game Over" / attract screen */
		bazooka_game_state = 0;
	}

  /* 
   * Check if Game Is Playing and simulates the
   * "bazooka" movement. Originally the game uses a
   * pot, we've changed to simple inputs for better
   * playability
   *
   * */
	if (bazooka_game_state == 1)
	{
    int bazooka_speed = 2;

		if (readinputport(0) & 0x04) { 
      bazooka_bazooka_pos -= bazooka_speed;
			if (bazooka_bazooka_pos < 0) bazooka_bazooka_pos = 0; 
    }

		if (readinputport(0) & 0x08) {
      bazooka_bazooka_pos += bazooka_speed;
			if (bazooka_bazooka_pos > bazooka_screen_width - 8) bazooka_bazooka_pos = bazooka_screen_width - 8;
    }
  
    if (readinputport(0) & 0x02) { /* Fire button */
		/* Only allow firing if the bullet is inactive AND the previous sound has finished fading */
		  if (!bazooka_bullet_active && fire_sound_volume == 0) {
			  bazooka_bullet_active = 1;
				bazooka_bullet_x = bazooka_bazooka_pos;
				bazooka_bullet_y = bazooka_screen_width - 32;
        bazooka_effects_sound(0);     /* Turn the sound ON at max volume */
        fire_sound_volume = 15;         /* Start the fade process */
        fire_sound_decay_counter = 0; /* Reset the decay counter */
			}
		}


	}

  bazooka_animation_counter++;
  if (bazooka_animation_counter > 15) {
    bazooka_animation_counter = 0;
    bazooka_frame_pol = 1 - bazooka_frame_pol;
  }

  /* 
   * Simulates the "bullet", which corresponds
   * of two bits travelling up the screen   *
   * */
	if (bazooka_bullet_active)
	{
		bazooka_bullet_y -= 3;
		plot_pixel(bitmap, bazooka_bullet_x, bazooka_bullet_y, 1);
		plot_pixel(bitmap, bazooka_bullet_x, bazooka_bullet_y - 1, 1);
		if (bazooka_bullet_y < 0) bazooka_bullet_active = 0;
	}

  for (i = 0; i < 4; i++)
  {
    bazooka_do_line(bitmap, i);
    if (bazooka_bullet_active && bazooka_line_exploding[i] == 0)
    {
      int line_y = i * 24;
      if (
        bazooka_bullet_y >= line_y && bazooka_bullet_y < line_y + 16 &&
        bazooka_bullet_x >= bazooka_line_position[i] &&
        bazooka_bullet_x < bazooka_line_position[i] + bazooka_line_vehicle_width[i]
      ){
          bazooka_bullet_active = 0;
          bazooka_line_exploding[i] = 30;
          bazooka_vehicle_sound_off(i);
          bazooka_tank_sound_off();
          bazooka_ambulance_sound_off();
          if (fire_sound_volume > 0) {
            fire_sound_volume = 0;
            bazooka_noise_set_volume(0);
          }
          bazooka_effects_sound(1);
          /*
           * Points configuration following original plate on the control panels
           *
           * */
          switch(bazooka_line_vehicle[i]) {
           case 0: // Jeep
              bazooka_score += 600;
              break;
            case 1: // Truck
              bazooka_score += 400;
              break;
            case 2: // Motorcycle
              bazooka_score += 800;
              break;
            case 3: // Tank
              bazooka_score += 200;
              break;
            case 4: // Stretcher
              bazooka_score -= 200;
              break;
            case 5: // Ambulance
              bazooka_score -= 200;
              break;
          }
          // Do not allow negative points;
          if (bazooka_score < 0) bazooka_score = 0;
          // Update the highscore
          if (bazooka_score > bazooka_high_score) bazooka_high_score = bazooka_score;
        }
      }
  }

  /* Sound logic - runs once per frame */
  if (bazooka_game_state == 1 && bazooka_effect_timer == 0)
  {
    int tank_on_screen = 0;
    int ambulance_on_screen = 0;

    /* Scan for global sound triggers (Tank and Ambulance) */
    for (i = 0; i < 4; i++) {
      if (bazooka_line_vehicle[i] == 3) tank_on_screen = 1;
      if (bazooka_line_vehicle[i] == 5) ambulance_on_screen = 1;
    }

    /* Handle Tank Sound */
    if (tank_on_screen)
    {
      bazooka_tank_sound(12);
    } else {
      bazooka_tank_sound_off();
    }

    /* Handle Ambulance Sound */
    if (ambulance_on_screen && !ambulance_on_screen_last_frame)
    {
      ambulance_timer = 0;
      ambulance_swap = 0;
    }
    
    ambulance_on_screen_last_frame = ambulance_on_screen;

    if (ambulance_on_screen)
    {
      ambulance_timer++;
      if (ambulance_timer > 30)
      {
        ambulance_timer = 0;
        ambulance_swap = 1 - ambulance_swap;
      }
      bazooka_ambulance_sound(ambulance_swap ? 540 : 440, 15);
    } else {
      bazooka_ambulance_sound_off();
    }

    /* Per-Line vehicle sounds (Jeep, Truck, Bike) */
    for (i = 0; i < 4; i++)
    {
      int vehicle_type = bazooka_line_vehicle[i];
      int freq = 0;
      int volume = 0;

      /* Frequency and Volume */
      switch(vehicle_type)
      {
        case 0: // Jeep
          freq = 300;
          volume = 12;
          break;
        case 1: // Truck
          freq = 400;
          volume = 12;
          break;
        case 2: // Motorcycle
          freq = 200;
          volume = 12;
          break;
        case 4: // Stretcher
          freq = 0;
          volume = 0;
          break;
      }

      if (freq > 0)
      {
        bazooka_vehicle_sound(i, freq, volume);
      } else {
        /* Mute channel if it's not a standard vehicle */
        bazooka_vehicle_sound_off(i);
        }
      }
  }

	for (x = 0; x < bazooka_screen_width; x++) {
		plot_pixel(bitmap, x, bazooka_screen_width - 17, 1);
		plot_pixel(bitmap, x, bazooka_screen_width - 18, 1);
		plot_pixel(bitmap, x, bazooka_screen_width - 2,  1);
	}

  // Draw current score
	bazooka_draw_number(bitmap, 0, bazooka_screen_width-15, bazooka_score, 5);

  // Draw highscore
	bazooka_draw_number(bitmap, bazooka_screen_width - 36, bazooka_screen_width-15, bazooka_high_score, 5);

  /*
   * If the game is running, draw a virtual bazooka for the player
   * to able to see where the game is poiting it
  * */
  if (bazooka_game_state == 1)
  {
	  bazooka_draw_number(bitmap, (bazooka_screen_width/2)-12, bazooka_screen_width-15, bazooka_play_time, 3);
    drawgfx(bitmap, Machine->gfx[2], 10, 1, 0, 0, bazooka_bazooka_pos - 4, bazooka_screen_width-32, &Machine->visible_area, TRANSPARENCY_PEN, 0);
  }

  // Check and draws if Game Over
  if (bazooka_game_state == 2 || bazooka_game_state == 0) {
    bazooka_draw_gameover(bitmap);
  }

}


VIDEO_START( bazooka )
{
	bazooka_new_game();
	bazooka_game_state = 0;
	return 0;
}

// Video Interrupct
void bazooka_video_irq(){

  if (bazooka_effect_timer > 0)
  {
    bazooka_effect_timer--;
    if (bazooka_effect_timer == 0) bazooka_noise_set_volume(0);
  }

  if (readinputport(0) & 0x01)
  {
    if (bazooka_game_state != 1) bazooka_new_game();
  }
  
  if (bazooka_game_state == 1)
  {
    static int clock_timer = 0;
    clock_timer++;
    if (clock_timer >= 60)
    {
      clock_timer = 0;
      bazooka_play_time--;
      if (bazooka_play_time < 0)
      {
        bazooka_play_time = 0;
        bazooka_game_state = 2;
        bazooka_sound_off();
      }
    }
		if (!bazooka_bonus_time_awarded && bazooka_score > 12000)
    {
      bazooka_play_time += 20;
      bazooka_bonus_time_awarded = 1;
    }
  }    
}

/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2004 Andrea Mazzoleni
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
 * Configuration.
 */

#ifndef __LCD_H
#define __LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "extra.h"

/**
 * LCD context.
 * This struct contains the status of the LCD system.
 */
typedef struct adv_lcd_struct {
	int f; /**< Socket. */

	unsigned width; /**< Display width. */
	unsigned height; /**< Display height. */

	adv_bool mute_flag; /**< Disable any output. */
} adv_lcd;

/** \addtogroup LCD */
/*@{*/

adv_lcd* adv_lcd_init(const char* address, unsigned timeout);
void adv_lcd_done(adv_lcd* context);
adv_error adv_lcd_display(adv_lcd* context, unsigned row, const char* text, int speed);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif



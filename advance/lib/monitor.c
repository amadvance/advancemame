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

#include "portable.h"

#include "monitor.h"
#include "error.h"

/**
 * Clear a monitor clock specification.
 */
void monitor_reset(adv_monitor* monitor)
{
	memset(monitor, 0, sizeof(adv_monitor));

	monitor->mode_mac = 0;
}

/**
 * Check if monitor clock specification is empty.
 */
adv_bool monitor_is_empty(const adv_monitor* monitor)
{
	return monitor->mode_mac == 0;
}

adv_bool monitor_mode_hclock_check(const adv_monitor_mode* mode, double hclock)
{
	const double monitor_hfix_error = 0.02; /* allowed error for fixed value. */
	const double monitor_hrange_error = 0.01; /* allowed error for range value. */

	if (mode->hclock.low == mode->hclock.high) {
		return mode->hclock.low * (1-monitor_hfix_error) <= hclock && hclock <= mode->hclock.high * (1+monitor_hfix_error);
	} else {
		return mode->hclock.low * (1-monitor_hrange_error) <= hclock && hclock <= mode->hclock.high * (1+monitor_hrange_error);
	}
}

adv_bool monitor_mode_vclock_check(const adv_monitor_mode* mode, double vclock)
{
	const double monitor_vfix_error = 0.02; /* allowed error for fixed value. */
	const double monitor_vrange_error = 0.01; /* allowed error for range value. */

	if (mode->vclock.low == mode->vclock.high) {
		return mode->vclock.low * (1-monitor_vfix_error) <= vclock && vclock <= mode->vclock.high * (1+monitor_vfix_error);
	} else {
		return mode->vclock.low * (1-monitor_vrange_error)<= vclock && vclock <= mode->vclock.high * (1+monitor_vrange_error);
	}
}

/**
 * Check if a pixel clock is acceptable.
 * \param mode Monitor mode specification.
 * \param pclock Horizontal clock.
 */
adv_bool monitor_mode_pclock_check(const adv_monitor_mode* mode, double pclock)
{
	if (mode->pclock.low <= pclock && pclock <= mode->pclock.high)
		return 1;

	return 0;
}

/**
 * Check if a horizontal and vertical clocks are acceptable.
 * \param mode Monitor mode specification.
 * \param hclock Horizontal clock.
 * \param vclock Vertical clock.
 */
adv_bool monitor_mode_hvclock_check(const adv_monitor_mode* mode, double hclock, double vclock)
{
	return monitor_mode_hclock_check(mode, hclock)
		&& monitor_mode_vclock_check(mode, vclock);
}

/**
 * Check if a horizontal and vertical clocks are acceptable.
 * \param mode Monitor mode specification.
 * \param hclock Horizontal clock.
 * \param vclock Vertical clock.
 */
adv_bool monitor_mode_clock_check(const adv_monitor_mode* mode, double pclock, double hclock, double vclock)
{
	return monitor_mode_pclock_check(mode, pclock)
		&& monitor_mode_hclock_check(mode, hclock)
		&& monitor_mode_vclock_check(mode, vclock);
}

/**
 * Check if a complete clock specification is acceptable.
 * \param monitor Monitor clock specification.
 * \param pclock Pixel clock.
 * \param hclock Horizontal clock.
 * \param vclock Vertical clock.
 */
adv_bool monitor_clock_check(const adv_monitor* monitor, double pclock, double hclock, double vclock)
{
	unsigned i;

	for(i=0;i<monitor->mode_mac;++i)
		if (monitor_mode_clock_check(&monitor->mode_map[i], pclock, hclock, vclock))
			return 1;

	error_nolog_set("Pixel/Horizontal/Vertical clocks of %.2f/%.2f/%.2f MHz/kHz/Hz are out of range of your monitor", (double)pclock / 1E6, (double)hclock / 1E3, (double)vclock);
	return 0;
}

/**
 * Check if a horizontal and vertical clocks are acceptable.
 * \param monitor Monitor clock specification.
 * \param hclock Horizontal clock.
 * \param vclock Vertical clock.
 */
adv_bool monitor_hvclock_check(const adv_monitor* monitor, double hclock, double vclock)
{
	unsigned i;

	for(i=0;i<monitor->mode_mac;++i)
		if (monitor_mode_hvclock_check(&monitor->mode_map[i], hclock, vclock))
			return 1;

	error_nolog_set("Horizontal/Vertical clocks of %.2f/%.2f Hz/kHz are out of range of your monitor", (double)hclock / 1E3, (double)vclock);
	return 0;
}


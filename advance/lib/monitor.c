/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999-2002 Andrea Mazzoleni
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

#include "monitor.h"

#include <string.h>

void monitor_reset(video_monitor* monitor) {
	memset(monitor, 0, sizeof(video_monitor));
}

video_bool monitor_is_empty(const video_monitor* monitor) {
	int i;
	for(i=0;i<VIDEO_MONITOR_RANGE_MAX;++i)
		if (monitor->hclock[i].low != 0 || monitor->hclock[i].high != 0)
			return 0;
	for(i=0;i<VIDEO_MONITOR_RANGE_MAX;++i)
		if (monitor->vclock[i].low != 0 || monitor->vclock[i].high != 0)
			return 0;
	return 1;
}

/**
 * Check if a video mode is acceptable for the specified monitor.
 */
video_bool monitor_hclock_check(const video_monitor* monitor, double hclock) {
	const double monitor_hfix_error = 0.02;
	const double monitor_hrange_error = 0.01;
	unsigned i;
	for(i=0;i<VIDEO_MONITOR_RANGE_MAX;++i) {
		if (monitor->hclock[i].low != 0 && monitor->hclock[i].high != 0) {
			if (monitor->hclock[i].low == monitor->hclock[i].high) {
				/* use a % error */
				if (monitor->hclock[i].low * (1-monitor_hfix_error) <= hclock && hclock <= monitor->hclock[i].high * (1+monitor_hfix_error))
					break;
			} else {
				/* use 1 unit error */
				if (monitor->hclock[i].low * (1-monitor_hrange_error) <= hclock && hclock <= monitor->hclock[i].high * (1+monitor_hrange_error))
					break;
			}
		}
	}
	if (i==VIDEO_MONITOR_RANGE_MAX) {
		video_error_description_nolog_set("Horizontal clock of %.2f kHz is out of range of your monitor", (double)hclock / 1E3);
		return 0;
	}
	return 1;
}

video_bool monitor_vclock_check(const video_monitor* monitor, double vclock) {
	const double monitor_vfix_error = 0.02;
	const double monitor_vrange_error = 0.01;
	unsigned i;
	for(i=0;i<VIDEO_MONITOR_RANGE_MAX;++i) {
		if (monitor->vclock[i].low != 0 && monitor->vclock[i].high != 0) {
			if (monitor->vclock[i].low == monitor->vclock[i].high) {
				if (monitor->vclock[i].low * (1-monitor_vfix_error) <= vclock && vclock <= monitor->vclock[i].high * (1+monitor_vfix_error))
					break;
			} else {
				if (monitor->vclock[i].low * (1-monitor_vrange_error)<= vclock && vclock <= monitor->vclock[i].high * (1+monitor_vrange_error))
					break;
			}
		}
	}
	if (i==VIDEO_MONITOR_RANGE_MAX) {
		video_error_description_nolog_set("Vertical clock of %.2f Hz is out of range of your monitor", (double)vclock);
		return 0;
	}
	return 1;
}

video_bool monitor_pclock_check(const video_monitor* monitor, double pclock) {
	if (monitor->pclock.low <= pclock && pclock <= monitor->pclock.high)
		return 1;

	video_error_description_nolog_set("Pixel clock of %.2f Hz is out of range of your video board", (double)pclock);
	return 0;
}

video_bool monitor_clock_check(const video_monitor* monitor, double pclock, double hclock, double vclock) {
	return monitor_pclock_check(monitor,pclock)
		&& monitor_hclock_check(monitor,hclock)
		&& monitor_vclock_check(monitor,vclock);
}

video_bool monitor_hvclock_check(const video_monitor* monitor, double hclock, double vclock) {
	return monitor_hclock_check(monitor,hclock)
		&& monitor_vclock_check(monitor,vclock);
}

double monitor_hclock_min(const video_monitor* monitor) {
	double min = 0;
	unsigned i;
	for(i=0;i<VIDEO_MONITOR_RANGE_MAX;++i)
		if (monitor->hclock[i].low != 0 && (min == 0 || monitor->hclock[i].low < min))
			min = monitor->hclock[i].low;
	return min;
}

double monitor_hclock_max(const video_monitor* monitor) {
	double max = 0;
	unsigned i;
	for(i=0;i<VIDEO_MONITOR_RANGE_MAX;++i)
		if (monitor->hclock[i].high != 0 && (max == 0 || monitor->hclock[i].high > max))
			max = monitor->hclock[i].high;
	return max;
}

double monitor_vclock_min(const video_monitor* monitor) {
	double min = 0;
	unsigned i;
	for(i=0;i<VIDEO_MONITOR_RANGE_MAX;++i)
		if (monitor->vclock[i].low != 0 && (min == 0 || monitor->vclock[i].low < min))
			min = monitor->vclock[i].low;
	return min;
}

double monitor_vclock_max(const video_monitor* monitor) {
	double max = 0;
	unsigned i;
	for(i=0;i<VIDEO_MONITOR_RANGE_MAX;++i)
		if (monitor->vclock[i].high != 0 && (max == 0 || monitor->vclock[i].high > max))
			max = monitor->vclock[i].high;
	return max;
}

double monitor_pclock_min(const video_monitor* monitor) {
	return monitor->pclock.low;
}

double monitor_pclock_max(const video_monitor* monitor) {
	return monitor->pclock.high;
}

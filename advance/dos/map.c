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

#include "map.h"

#include <dpmi.h>

/**
 * Maps a physical address range into linear memory.
 */
int map_create_linear_mapping(unsigned long *linear, unsigned long physaddr, unsigned long size)
{
	if (physaddr >= 0x100000) {
		__dpmi_meminfo meminfo;

		/* map into linear memory */
		meminfo.address = physaddr;
		meminfo.size = size;
		if (__dpmi_physical_address_mapping(&meminfo) != 0)
			return -1;

		*linear = meminfo.address;

		/* lock the linear memory range */
		if (__dpmi_lock_linear_region(&meminfo) != 0)
			return -1;
	} else {
		/* exploit 1 -> 1 physical to linear mapping in low megabyte */
		*linear = physaddr;
	}

	return 0;
}

/**
 * Frees the DPMI resources being used to map a linear address range.
 */
void map_remove_linear_mapping(unsigned long physaddr, unsigned long size)
{
	if (physaddr) {
		if (physaddr >= 0x100000) {
			__dpmi_meminfo meminfo;
			meminfo.address = physaddr;
			meminfo.size = size;

			__dpmi_unlock_linear_region(&meminfo);
			__dpmi_free_physical_address_mapping(&meminfo);
		}
	}
}

/**
 * Allocates a selector to access a region of linear memory.
 */
int map_create_selector(unsigned *segment, unsigned long linear, unsigned long size)
{
	unsigned long ul;

	/* allocate an ldt descriptor */
	int des = __dpmi_allocate_ldt_descriptors(1);
	if (des < 0) {
		*segment = 0;
		return -1;
	}
	*segment = des;

	/* set the descriptor base and limit */
	if (__dpmi_set_segment_base_address(*segment, linear) != 0)
		return -1;

	if (__dpmi_get_segment_base_address(*segment, &ul)!=0)
		return -1;

	if (ul != linear)
		return -1;

	--size;
	if (size < 0xFFFF)
		size = 0xFFFF;
	size |= 0xFFF;
	if (__dpmi_set_segment_limit(*segment, size) != 0)
		return -1;
	if (__dpmi_get_segment_limit(*segment) != size)
		return -1;

	return 0;
}

/**
 * Frees a DPMI segment selector.
 */
void map_remove_selector(unsigned *segment)
{
	if (*segment) {
		__dpmi_free_ldt_descriptor(*segment);
		*segment = 0;
	}
}



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

#include "pcimap.h"
#include "map.h"
#include "pci.h"

#include "compil.h"

#define log_std(a) do { } while (0)

/***************************************************************************/
/* BIOS */

/* pci BIOS address */
static unsigned pci_BIOS_selector;
static unsigned long pci_BIOS_linear_address;
static unsigned long pci_BIOS_physical_address;
static unsigned long pci_BIOS_physical_size;

/* Map the specified address */
static int pci_BIOS_address_map_plain(unsigned long phys_address)
{
	pci_BIOS_physical_address = phys_address;
	pci_BIOS_physical_size = 0x10000;
	if (map_create_linear_mapping(&pci_BIOS_linear_address, pci_BIOS_physical_address, pci_BIOS_physical_size) != 0) {
		return -1;
	}
	if (map_create_selector(&pci_BIOS_selector, pci_BIOS_linear_address, pci_BIOS_physical_size) != 0) {
		map_remove_linear_mapping(pci_BIOS_physical_address, pci_BIOS_physical_size);
		return -1;
	}
	return 0;
}

/* Map the specified address and check the signature */
static int pci_BIOS_address_map_check(unsigned long phys_address)
{
	unsigned char tmp[2];
	log_std(("pci: check bios addr %08lx\n", (unsigned long)phys_address));
	if (!phys_address) {
		log_std(("pci: null bios pointer\n"));
		return -1;
	}

	tmp[0] = 0;
	tmp[1] = 0;

	if (pci_BIOS_address_map_plain(phys_address)!=0) {
		log_std(("pci: error mapping bios\n"));
		return -1;
	}

	pci_BIOS_read(tmp, 0, 2);

	if (tmp[0]!=0x55 || tmp[1]!=0xAA) {
		pci_BIOS_address_unmap();
		log_std(("pci: invalid bios signature\n"));
		return -1;
	}

	log_std(("pci: bios found at %08lx\n", (unsigned long)phys_address));
	return 0;
}

/* Detect and map the BIOS address */
int pci_BIOS_address_map(unsigned bus_device_func)
{
	DWORD orig;
	DWORD addr;
	if (pci_read_dword(bus_device_func, 0x30, &orig) != 0)
		return -1;

	log_std(("pci: bios reported addr %08lx\n", (unsigned long)orig));
	addr = orig & 0xfffe0000;

	if (pci_BIOS_address_map_check(addr)!=0) {
		addr = 0x000c0000;
		if (pci_BIOS_address_map_check(addr)!=0) {
			log_std(("pci: bios not found\n"));
			return -1;
		}
	}
	return 0;
}

void pci_BIOS_address_unmap(void)
{
	map_remove_selector(&pci_BIOS_selector);
	map_remove_linear_mapping(pci_BIOS_physical_address, pci_BIOS_physical_size);
	pci_BIOS_selector = 0;
	pci_BIOS_linear_address = 0;
	pci_BIOS_physical_address = 0;
	pci_BIOS_physical_size = 0;
}

void pci_BIOS_read(void* dest, unsigned offset, unsigned len)
{
	if (!pci_BIOS_selector)
		log_std(("pci: BUG! BIOS selector out of order\n"));

	movedata(pci_BIOS_selector, offset, _my_ds(), (unsigned long)dest, len);
}

/***************************************************************************/
/* MMIO */

/* pci MMIO address */
static unsigned pci_MMIO_selector;
static unsigned long pci_MMIO_linear_address;
static unsigned long pci_MMIO_physical_address;
static unsigned long pci_MMIO_physical_size;
static unsigned pci_MMIO_register;
static unsigned pci_MMIO_bus_device_func;

int pci_MMIO_address_map(unsigned bus_device_func, unsigned reg, unsigned long mask)
{
	DWORD addr;
	DWORD orig;
	pci_MMIO_register = reg;
	pci_MMIO_bus_device_func = bus_device_func;
	if (pci_read_dword(pci_MMIO_bus_device_func, pci_MMIO_register, &orig) != 0)
		return -1;
	if (!orig) {
		log_std(("pci: MMIO address null\n"));
		return -1;
	}
	log_std(("pci: MMIO orig %08lx\n", (unsigned long)orig));
	addr = orig & (0xFFFFFFF0 & mask);
	log_std(("pci: MMIO addr %08lx\n", (unsigned long)addr));
	pci_MMIO_physical_address = addr;
	pci_MMIO_physical_size = 0x10000;
	if (map_create_linear_mapping(&pci_MMIO_linear_address, pci_MMIO_physical_address, pci_MMIO_physical_size) != 0) {
		return -1;
	}
	if (map_create_selector(&pci_MMIO_selector, pci_MMIO_linear_address, pci_MMIO_physical_size) != 0) {
		map_remove_linear_mapping(pci_MMIO_physical_address, pci_MMIO_physical_size);
		return -1;
	}
	return 0;
}

void pci_MMIO_address_unmap(void)
{
	map_remove_selector(&pci_MMIO_selector);
	map_remove_linear_mapping(pci_MMIO_physical_address, pci_MMIO_physical_size);
	pci_MMIO_selector = 0;
	pci_MMIO_linear_address = 0;
	pci_MMIO_physical_address = 0;
	pci_MMIO_physical_size = 0;
}

unsigned pci_MMIO_selector_get(void)
{
	if (!pci_MMIO_selector)
		log_std(("pci: BUG! MMIO selector out of order\n"));
	return pci_MMIO_selector;
}



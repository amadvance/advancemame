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

#ifndef __PCI_H
#define __PCI_H

#include "compil.h"

int pci_detect(void);
int pci_find_device(unsigned vendor, unsigned device, unsigned subindex, unsigned STACK_PTR* bus_device_func);
int pci_read_dword(unsigned bus_device_func, unsigned reg, DWORD STACK_PTR* value);
int pci_write_dword(unsigned bus_device_func, unsigned reg, DWORD value);
int pci_read_word(unsigned bus_device_func, unsigned reg, WORD STACK_PTR* value);
int pci_write_word(unsigned bus_device_func, unsigned reg, WORD value);
int pci_read_byte(unsigned bus_device_func, unsigned reg, BYTE STACK_PTR* value);
int pci_write_byte(unsigned bus_device_func, unsigned reg, BYTE value);
int pci_bus_max(unsigned* bus_max);
int pci_scan_device(int (*callback)(unsigned bus_device_func, unsigned vendor, unsigned device, void* arg), void* arg);
int pci_read_dword_aperture_len(unsigned bus_device_func, unsigned reg, DWORD STACK_PTR* value);

int pci_BIOS_address_map(unsigned bus_device_func);
void pci_BIOS_address_unmap(void);
void pci_BIOS_read(void STACK_PTR* dest, unsigned offset, unsigned len);

int pci_MMIO_address_map(unsigned bus_device_func, unsigned reg, unsigned long mask);
void pci_MMIO_address_unmap(void);
unsigned pci_MMIO_selector_get(void);

#endif

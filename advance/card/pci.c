/*
 * This file is part of the AdvanceMAME project.
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

#include "pci.h"

#include "map.h"
#include "card.h"

/***************************************************************************/
/* PCI */

#define CF 0x1 /* carry flag */

int pci_detect(void) {
	__dpmi_regs r;
	r.d.eax = 0x0000b101;
	r.d.edi = 0x00000000;
	__dpmi_int(0x1a, &r);
	if (r.d.edx != 0x20494350)
		return -1;
	return 0;
}

int pci_find_device(unsigned vendor, unsigned device, unsigned index, unsigned STACK_PTR * bus_device_func) {
	__dpmi_regs r;
	r.d.eax = 0x0000b102;
	r.d.ecx = device;
	r.d.edx = vendor;
	r.d.esi = index;
	__dpmi_int(0x1a, &r);
	if ((r.x.flags & CF) != 0)
		return -1;
	if (r.h.ah != 0)
		return -1;
	*bus_device_func = r.d.ebx;
	return 0;
}

int pci_read_byte(unsigned bus_device_func, unsigned reg, BYTE STACK_PTR* value) {
	__dpmi_regs r;
	r.d.eax = 0x0000b108;
	r.d.ebx = bus_device_func;
	r.d.edi = reg;
	__dpmi_int(0x1a, &r);
	if ((r.x.flags & CF) != 0)
		return -1;
	if (r.h.ah != 0)
		return -1;
	*value = (BYTE)r.d.ecx;
	return 0;
}

int pci_read_word(unsigned bus_device_func, unsigned reg, WORD STACK_PTR* value) {
	__dpmi_regs r;
	r.d.eax = 0x0000b109;
	r.d.ebx = bus_device_func;
	r.d.edi = reg;
	__dpmi_int(0x1a, &r);
	if ((r.x.flags & CF) != 0)
		return -1;
	if (r.h.ah != 0)
		return -1;
	*value = (WORD)r.d.ecx;
	return 0;
}

int pci_read_dword(unsigned bus_device_func, unsigned reg, DWORD STACK_PTR* value) {
	__dpmi_regs r;
	r.d.eax = 0x0000b10a;
	r.d.ebx = bus_device_func;
	r.d.edi = reg;
	__dpmi_int(0x1a, &r);
	if ((r.x.flags & CF) != 0)
		return -1;
	if (r.h.ah != 0)
		return -1;
	*value = (DWORD)r.d.ecx;
	return 0;
}

int pci_write_byte(unsigned bus_device_func, unsigned reg, BYTE value) {
	__dpmi_regs r;
	r.d.eax = 0x0000b10b;
	r.d.ebx = bus_device_func;
	r.d.edi = reg;
	r.d.ecx = value;
	__dpmi_int(0x1a, &r);
	if ((r.x.flags & CF) != 0)
		return -1;
	if (r.h.ah != 0)
		return -1;
	return 0;
}

int pci_write_word(unsigned bus_device_func, unsigned reg, WORD value) {
	__dpmi_regs r;
	r.d.eax = 0x0000b10c;
	r.d.ebx = bus_device_func;
	r.d.edi = reg;
	r.d.ecx = value;
	__dpmi_int(0x1a, &r);
	if ((r.x.flags & CF) != 0)
		return -1;
	if (r.h.ah != 0)
		return -1;
	return 0;
}

int pci_write_dword(unsigned bus_device_func, unsigned reg, DWORD value) {
	__dpmi_regs r;
	r.d.eax = 0x0000b10d;
	r.d.ebx = bus_device_func;
	r.d.edi = reg;
	r.d.ecx = value;
	__dpmi_int(0x1a, &r);
	if ((r.x.flags & CF) != 0)
		return -1;
	if (r.h.ah != 0)
		return -1;
	return 0;
}

int pci_scan_device(int (*callback)(unsigned bus_device_func,unsigned vendor,unsigned device, void* arg), void* arg) {
	__dpmi_regs r;
	unsigned bus;
	unsigned i,j;

	r.d.eax = 0x0000b101;
	r.d.edi = 0x00000000;
	__dpmi_int(0x1a, &r);
	if (r.d.edx != 0x20494350)
		return -1;

	bus = r.h.cl + 1;

	for(i=0;i<bus;++i) {
		for(j=0;j<32;++j) {
			int r;
			DWORD dw;
			unsigned bus_device_func = (i << 8) | (j << 3);
			unsigned device;
			unsigned vendor;

			if (pci_read_dword(bus_device_func,0,&dw)!=0)
				continue;

			vendor = dw & 0xFFFF;
			device = (dw >> 16) & 0xFFFF;

			r = callback(bus_device_func,vendor,device,arg);
			if (r!=0)
				return r;
		}
	}

	return 0;
}

/***************************************************************************/
/* BIOS */

#ifndef __PCIREAL__

/* pci BIOS address */
static unsigned pci_BIOS_selector;
static unsigned long pci_BIOS_linear_address;
static unsigned long pci_BIOS_physical_address;
static unsigned long pci_BIOS_physical_size;

/* Map the specified address */
static int pci_BIOS_address_map_plain(unsigned long phys_address) {
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
static int pci_BIOS_address_map_check(unsigned long phys_address) {
	unsigned char tmp[2];
	CARD_LOG(("pci: check bios addr %08lx\n",(unsigned long)phys_address));
	if (!phys_address) {
		CARD_LOG(("pci: null bios pointer\n"));
		return -1;
	}

	tmp[0] = 0;
	tmp[1] = 0;

	if (pci_BIOS_address_map_plain(phys_address)!=0) {
		CARD_LOG(("pci: error mapping bios\n"));
		return -1;
	}

	pci_BIOS_read(tmp,0,2);

	if (tmp[0]!=0x55 || tmp[1]!=0xAA) {
		pci_BIOS_address_unmap();
		CARD_LOG(("pci: invalid bios signature\n"));
		return -1;
	}

	CARD_LOG(("pci: bios found at %08lx\n",(unsigned long)phys_address));
	return 0;
}

/* Detect and map the BIOS address */
int pci_BIOS_address_map(unsigned bus_device_func) {
	DWORD orig;
	DWORD addr;
	if (pci_read_dword(bus_device_func,0x30,&orig) != 0)
		return -1;

	CARD_LOG(("pci: bios reported addr %08lx\n",(unsigned long)orig));
	addr = orig & 0xfffe0000;

	if (pci_BIOS_address_map_check(addr)!=0) {
		addr = 0x000c0000;
		if (pci_BIOS_address_map_check(addr)!=0) {
			CARD_LOG(("pci: bios not found\n"));
			return -1;
		}
	}
	return 0;
}

void pci_BIOS_address_unmap(void) {
	map_remove_selector(&pci_BIOS_selector);
	map_remove_linear_mapping(pci_BIOS_physical_address,pci_BIOS_physical_size);
	pci_BIOS_selector = 0;
	pci_BIOS_linear_address = 0;
	pci_BIOS_physical_address = 0;
	pci_BIOS_physical_size = 0;
}

void pci_BIOS_read(void STACK_PTR* dest, unsigned offset, unsigned len) {
	if (!pci_BIOS_selector)
		CARD_LOG(("pci: BUG! BIOS selector out of order\n"));

	movedata(pci_BIOS_selector,offset,_my_ds(),(unsigned long)dest,len);
}

#endif

/***************************************************************************/
/* MMIO */

#ifndef __PCIREAL__

/* pci MMIO address */
static unsigned pci_MMIO_selector;
static unsigned long pci_MMIO_linear_address;
static unsigned long pci_MMIO_physical_address;
static unsigned long pci_MMIO_physical_size;
static unsigned pci_MMIO_register;
static unsigned pci_MMIO_bus_device_func;

int pci_MMIO_address_map(unsigned bus_device_func, unsigned reg, unsigned long mask) {
	DWORD addr;
	DWORD orig;
	pci_MMIO_register = reg;
	pci_MMIO_bus_device_func = bus_device_func;
	if (pci_read_dword(pci_MMIO_bus_device_func,pci_MMIO_register,&orig) != 0)
		return -1;
	if (!orig) {
		CARD_LOG(("pci: MMIO address null\n"));
		return -1;
	}
	CARD_LOG(("pci: MMIO orig %08lx\n",(unsigned long)orig));
	addr = orig & (0xFFFFFFF0 & mask);
	CARD_LOG(("pci: MMIO addr %08lx\n",(unsigned long)addr));
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

void pci_MMIO_address_unmap(void) {
	map_remove_selector(&pci_MMIO_selector);
	map_remove_linear_mapping(pci_MMIO_physical_address,pci_MMIO_physical_size);
	pci_MMIO_selector = 0;
	pci_MMIO_linear_address = 0;
	pci_MMIO_physical_address = 0;
	pci_MMIO_physical_size = 0;
}

unsigned pci_MMIO_selector_get(void) {
	if (!pci_MMIO_selector)
		CARD_LOG(("pci: BUG! MMIO selector out of order\n"));
	return pci_MMIO_selector;
}

#endif

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

#include "pci.h"

/***************************************************************************/
/* PCI */

#define CF 0x1 /* carry flag */

int pci_detect(void)
{
	__dpmi_regs r;
	r.d.eax = 0x0000b101;
	r.d.edi = 0x00000000;
	__dpmi_int(0x1a, &r);
	if (r.d.edx != 0x20494350)
		return -1;
	return 0;
}

int pci_find_device(unsigned vendor, unsigned device, unsigned index, unsigned STACK_PTR * bus_device_func)
{
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

int pci_read_byte(unsigned bus_device_func, unsigned reg, BYTE STACK_PTR* value)
{
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

int pci_read_word(unsigned bus_device_func, unsigned reg, WORD STACK_PTR* value)
{
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

int pci_read_dword(unsigned bus_device_func, unsigned reg, DWORD STACK_PTR* value)
{
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

int pci_read_dword_aperture_len(unsigned bus_device_func, unsigned reg, DWORD STACK_PTR* value)
{
	DWORD ori;
	DWORD mask;
	DWORD len;

	if (pci_read_dword(bus_device_func, reg, &ori) != 0)
		return -1;
	if (!ori)
		return -1;
	if (pci_write_dword(bus_device_func, reg, 0xffffffff) != 0)
		return -1;
	if (pci_read_dword(bus_device_func, reg, &mask) != 0)
		return -1;
	if (pci_write_dword(bus_device_func, reg, ori) != 0)
		return -1;

	len = ~(mask & ~0xf) + 1;

	if (!len)
		return -1;

	*value = len;

	return 0;
}

int pci_write_byte(unsigned bus_device_func, unsigned reg, BYTE value)
{
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

int pci_write_word(unsigned bus_device_func, unsigned reg, WORD value)
{
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

int pci_write_dword(unsigned bus_device_func, unsigned reg, DWORD value)
{
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

int pci_bus_max(unsigned* bus_max)
{
	__dpmi_regs r;

	r.d.eax = 0x0000b101;
	r.d.edi = 0x00000000;
	__dpmi_int(0x1a, &r);
	if (r.d.edx != 0x20494350)
		return -1;

	*bus_max = r.h.cl + 1;

	return 0;
}

int pci_scan_device(int (*callback)(unsigned bus_device_func, unsigned vendor, unsigned device, void* arg), void* arg)
{
	__dpmi_regs r;
	unsigned bus;
	unsigned i, j;

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

			if (pci_read_dword(bus_device_func, 0, &dw)!=0)
				continue;

			vendor = dw & 0xFFFF;
			device = (dw >> 16) & 0xFFFF;

			r = callback(bus_device_func, vendor, device, arg);
			if (r!=0)
				return r;
		}
	}

	return 0;
}


#include "svgalib.h"

#include "map.h"
#include "pci.h"

#include <unistd.h>
#include <pc.h>
#include <stdlib.h>
#include <crt0.h>
#include <sys/nearptr.h>
#include <dos.h>

/**************************************************************************/
/* misc */

void adv_svgalib_enable(void) {
	enable();
}

void adv_svgalib_disable(void) {
	disable();
}

void adv_svgalib_usleep(unsigned n) {
	unsigned i;
	n = n / 2;
	for(i=0;i<n;++i) {
		inportb(0x80);
	}
}

/**************************************************************************/
/* port */

unsigned char adv_svgalib_inportb(unsigned port)
{
	return inportb(port);
}

unsigned short adv_svgalib_inportw(unsigned port)
{
	return inportw(port);
}

unsigned adv_svgalib_inportl(unsigned port)
{
	return inportl(port);
}

void adv_svgalib_outportb(unsigned port, unsigned char data)
{
	outportb(port,data);
}

void adv_svgalib_outportw(unsigned port, unsigned short data)
{
	outportw(port,data);
}

void adv_svgalib_outportl(unsigned port, unsigned data)
{
	outportl(port,data);
}

/**************************************************************************/
/* pci */

int adv_svgalib_pci_read_byte(unsigned bus_device_func, unsigned reg, unsigned char* value) {
	return pci_read_byte(bus_device_func, reg, value);
}

int adv_svgalib_pci_read_word(unsigned bus_device_func, unsigned reg, unsigned short* value) {
	return pci_read_word(bus_device_func, reg, value);
}

int adv_svgalib_pci_read_dword(unsigned bus_device_func, unsigned reg, unsigned* value) {
	return pci_read_dword(bus_device_func, reg, (DWORD*)value);
}

int adv_svgalib_pci_write_byte(unsigned bus_device_func, unsigned reg, unsigned char value) {
	return pci_write_byte(bus_device_func, reg, value);
}

int adv_svgalib_pci_write_word(unsigned bus_device_func, unsigned reg, unsigned short value) {
	return pci_write_word(bus_device_func, reg, value);
}

int adv_svgalib_pci_write_dword(unsigned bus_device_func, unsigned reg, unsigned value) {
	return pci_write_dword(bus_device_func, reg, value);
}

/**************************************************************************/
/* device */

int adv_svgalib_open(void) {
	return 0;
}

void adv_svgalib_close(void) {
}

/**************************************************************************/
/* printf */

void adv_svgalib_fprintf(void* file, const char* format, ...) {
}

void* adv_svgalib_stderr() {
	return 0;
}

/**************************************************************************/
/* mmap */

void* adv_svgalib_mmap(void* start, unsigned length, int prot, int flags, int fd, unsigned offset) {
	unsigned long linear;

	(void)prot;
	(void)fd;

	if ((flags & MAP_FIXED) != 0)
		return MAP_FAILED;

	if ((_crt0_startup_flags & _CRT0_FLAG_NEARPTR) == 0) {
		if (!__djgpp_nearptr_enable())
			return MAP_FAILED;
		_crt0_startup_flags |= _CRT0_FLAG_NEARPTR;
	}

	if (map_create_linear_mapping(&linear, offset, length)!=0)
		return MAP_FAILED;

	linear += __djgpp_conventional_base;

	return (void*)linear;
}

int adv_svgalib_munmap(void* start, unsigned length) {
	unsigned long offset;

	offset = (unsigned)start;

	offset -= __djgpp_conventional_base;

	map_remove_linear_mapping(offset,length);

	return 0;
}

/**************************************************************************/
/* iopl */

int adv_svgalib_iopl(int perm) {
	(void)perm;
	return 0;
}

/***************************************************************************/
/* vga */

void __svgalib_delay(void) {
	__asm__ __volatile__ (
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		"xorl %%eax,%%eax\n"
		:
		:
		: "cc", "%eax"
	);
}


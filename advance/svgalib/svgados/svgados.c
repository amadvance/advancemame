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

/**************************************************************************/
/* vga_help.c */

void port_rep_outb(unsigned char* string, int length, int port)
{
	while (length) {
		port_out(*string, port);
		++string;
		--length;
	}
}

void port_out(int value, int port)
{
	outportb(port,value);
}

void port_outw(int value, int port)
{
	outportw(port,value);
}

void port_outl(int value, int port)
{
	outportl(port,value);
}

int port_in(int port)
{
	return inportb(port);
}

int port_inw(int port)
{
	return inportw(port);
}

int port_inl(int port)
{
	return inportl(port);
}

/**************************************************************************/
/* vgapci.c */

struct pci_find {
	unsigned vendor;
	unsigned cont;
	unsigned bus_device_func;
};

static int pci_scan_device_callback(unsigned bus_device_func, unsigned vendor, unsigned device, void* _arg) {
	DWORD dw;
	unsigned base_class;
	struct pci_find* arg = (struct pci_find*)_arg;
	(void)device;

	if (vendor != arg->vendor)
		return 0;

	if (pci_read_dword(bus_device_func,0x8,&dw)!=0)
		return 0;

	base_class = (dw >> 16) & 0xFFFF;
	if (base_class != 0x300 /* DISPLAY | VGA */)
		return 0;

	if (arg->cont) {
		--arg->cont;
		return 0;
	}

	arg->bus_device_func = bus_device_func;

	return 1;
}

int __svgalib_pci_find_vendor_vga(unsigned int vendor, unsigned long *conf, int cont)
{
	int r;
	int i;
	struct pci_find find;
	find.vendor = vendor;
	find.cont = cont;

	r = pci_scan_device(pci_scan_device_callback,&find);
	if (r!=1)
		return 1; /* not found */

	for(i=0;i<64;++i) {
		DWORD v;
		pci_read_dword(find.bus_device_func,i*4,&v);
		conf[i] = v;
	}

	return 0;
}

int __svgalib_pci_find_vendor_vga_pos(unsigned int vendor, unsigned long *conf, int cont)
{
	int r;
	int i;
	struct pci_find find;
	find.vendor = vendor;
	find.cont = cont;

	r = pci_scan_device(pci_scan_device_callback,&find);
	if (r!=1)
		return 0; /* not found */

	for(i=0;i<64;++i) {
		DWORD v;
		pci_read_dword(find.bus_device_func,i*4,&v);
		conf[i] = v;
	}

	return find.bus_device_func;
}

int __svgalib_pci_read_config_byte(int pos, int address)
{
	BYTE r;
	pci_read_byte(pos,address,&r);
	return r;
}

int __svgalib_pci_read_config_word(int pos, int address)
{
	WORD r;
	pci_read_word(pos,address,&r);
	return r;
}

int __svgalib_pci_read_config_dword(int pos, int address)
{
	DWORD r;
	pci_read_dword(pos,address,&r);
	return r;
}

int __svgalib_pci_read_aperture_len(int pos, int reg)
{
	DWORD r;
	unsigned address;
	address = 16+4*reg; /* this is the memory register number */
	if (pci_read_dword_aperture_len(pos,address,&r) != 0)
		return 0;
	else
		return r;
}

void __svgalib_pci_write_config_byte(int pos, int address, unsigned char data)
{
	pci_write_byte(pos,address,data);
}

void __svgalib_pci_write_config_word(int pos, int address, unsigned short data)
{
	pci_write_word(pos,address,data);
}

void __svgalib_pci_write_config_dword(int pos, int address, unsigned int data)
{
	pci_write_dword(pos,address,data);
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


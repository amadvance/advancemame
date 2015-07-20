#include "svgalib.h"

#include <sys/io.h>
#include <unistd.h>

/**************************************************************************/
/* misc */

void adv_svgalib_enable(void) 
{
}

void adv_svgalib_disable(void) 
{
}

void adv_svgalib_usleep(unsigned n) 
{
	unsigned i;
	n = n / 2;
	for(i=0;i<n;++i) {
		inb(0x80);
	}
}

void adv_svgalib_abort(void) 
{
	abort();
}

/**************************************************************************/
/* port */

unsigned char adv_svgalib_inportb(unsigned port)
{
	return inb(port);
}

unsigned short adv_svgalib_inportw(unsigned port)
{
	return inw(port);
}

unsigned adv_svgalib_inportl(unsigned port)
{
	return inl(port);
}

void adv_svgalib_outportb(unsigned port, unsigned char data)
{
	outb(data, port);
}

void adv_svgalib_outportw(unsigned port, unsigned short data)
{
	outw(data, port);
}

void adv_svgalib_outportl(unsigned port, unsigned data)
{
	outl(data, port);
}

/**************************************************************************/
/* pci */

int adv_svgalib_pci_bus_max(unsigned* bus_max)
{
	*bus_max = 4;
	return 0;
}

int adv_svgalib_pci_read_byte(unsigned bus_device_func, unsigned reg, unsigned char* value) 
{
	adv_svgalib_log("svgalib: pci_read_byte failed\n");
	return -1;
}

int adv_svgalib_pci_read_word(unsigned bus_device_func, unsigned reg, unsigned short* value) 
{
	adv_svgalib_log("svgalib: pci_read_word failed\n");
	return -1;
}

int adv_svgalib_pci_read_dword(unsigned bus_device_func, unsigned reg, unsigned* value) 
{
	int f;
	unsigned char n[4];
	int bus, device, func;
	char filename[256];
	
	bus = (bus_device_func & 0xff00) >> 8;
	device = (bus_device_func & 0xf8) >> 3;
	func = bus_device_func & 0x07;
	sprintf(filename, "/proc/bus/pci/%02i/%02x.%i", bus, device, func);

	f = open(filename, O_RDONLY);
	if (f == -1) {
		adv_svgalib_log("svgalib: adv_svgalib_pci_read_dword failed\n");
		return -1;
	}

	if (lseek(f, address, SEEK_SET) == -1) {
		adv_svgalib_log("svgalib: adv_svgalib_pci_read_dword failed\n");
		close(f);
		return -1;
	}

	if (read(f, &n, 4) != 4) {
		adv_svgalib_log("svgalib: adv_svgalib_pci_read_dword failed\n");
		close(f);
		return -1;
	}

	close(f);

	*value = (unsigned)n[0] | (unsigned)n[1] << 8 | (unsigned)n[2] << 16 | (unsigned)n[3] << 24;

	return 0;
}

int adv_svgalib_pci_read_dword_nolog(unsigned bus_device_func, unsigned reg, unsigned* value)
{
	int f;
	unsigned char n[4];
	int bus, device, func;
	char filename[256];
	
	bus = (bus_device_func & 0xff00) >> 8;
	device = (bus_device_func & 0xf8) >> 3;
	func = bus_device_func & 0x07;
	sprintf(filename, "/proc/bus/pci/%02i/%02x.%i", bus, device, func);

	f = open(filename, O_RDONLY);
	if (f == -1) {
		return -1;
	}

	if (lseek(f, address, SEEK_SET) == -1) {
		close(f);
		return -1;
	}

	if (read(f, &n, 4) != 4) {
		close(f);
		return -1;
	}

	close(f);

	*value = (unsigned)n[0] | (unsigned)n[1] << 8 | (unsigned)n[2] << 16 | (unsigned)n[3] << 24;

	return 0;
}

int adv_svgalib_pci_write_byte(unsigned bus_device_func, unsigned reg, unsigned char value) {
	adv_svgalib_log("svgalib: adv_svgalib_pci_write_byte failed\n");
	return -1;
}

int adv_svgalib_pci_write_word(unsigned bus_device_func, unsigned reg, unsigned short value) {
	adv_svgalib_log("svgalib: adv_svgalib_pci_write_word failed\n");
	return -1;
}

int adv_svgalib_pci_write_dword(unsigned bus_device_func, unsigned reg, unsigned value) {
	adv_svgalib_log("svgalib: adv_svgalib_pci_write_dword failed\n");
	return -1;
}

/**************************************************************************/
/* device */

int adv_svgalib_open(void)
{
	return 0;
}

void adv_svgalib_close(void)
{
}

/**************************************************************************/
/* mmap */

void* adv_svgalib_mmap(void* start, unsigned length, int prot, int flags, int fd, unsigned offset)
{
	void* p;

	adv_svgalib_log("svgalib: mapping address %08x, size %d\n", offset, length);

	if ((flags & MAP_FIXED) != 0)
		return MAP_FAILED;

	p = mmap(start, length, prot, flags, fd, offset);

	if (p == MAP_FAILED) {
		adv_svgalib_log("svgalib: mmap() failed\n");
		return MAP_FAILED;
	}

	adv_svgalib_log("svgalib: mapped pointer %08x\n", (unsigned)p);

	return p;
}

int adv_svgalib_munmap(void* start, unsigned length)
{
	adv_svgalib_log("svgalib: unmapping pointer %08x, size %d\n", (unsigned)start, length);

	return munmap(start, length);
}

/**************************************************************************/
/* iopl */

int adv_svgalib_iopl(int perm)
{
	(void)perm;
	return 0;
}

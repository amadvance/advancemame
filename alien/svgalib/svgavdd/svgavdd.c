#include "svgalib.h"

/* for windows ioctl */
#define ULONG unsigned
#define CTL_CODE( DeviceType, Function, Method, Access ) ( \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)
#define METHOD_BUFFERED 0
#define FILE_ANY_ACCESS 0
typedef struct _PHYSICAL_ADDRESS {
    long long  QuadPart;
} PHYSICAL_ADDRESS;
#define VOID void
#define HANDLE unsigned

/* ioctl commands */
#include "../svgawin/driver/svgacode.h"

#include <dpmi.h>
#include <go32.h>
#include <dos.h>
#include <assert.h>
#include <crt0.h>
#include <sys/nearptr.h>

/**
 * If defined use the GIVEIO mode to access the IO port.
 */
/* #define USE_GIVEIO */ 
/* GIVE IO is NOT USABLE with a VDD, because the PID of the VDD is possibly different than the PID of NTVD */

/**
 * If defined use the TOTALIO mode to access the IO port.
 */
/* #define USE_TOTALIO */

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
}

void adv_svgalib_abort(void) 
{
	abort();
}

/**************************************************************************/
/* vdd */

static const char vdd_dll[]  = "SVGAVDD.DLL";
static const char vdd_init[] = "VDDRegisterInit";
static const char vdd_dispatch[] = "VDDDispatch";

static unsigned short vdd_handle;
static int vdd_memory_selector;
static int vdd_memory_segment;

static const char vdd_register_module[] = 
{
0xC4, 0xC4, 0x58, 0x0, /* RegisterModule(); */
0xCB /* retf */
};

static const char vdd_unregister_module[] = 
{
0xC4, 0xC4, 0x58, 0x1, /* UnRegisterModule(); */
0xCB /* retf */
};

static const char vdd_dispatch_call[] = 
{
0xC4, 0xC4, 0x58, 0x2, /* DispatchCall */
0xCB /* retf */
};

unsigned vdd_allocate(unsigned* address, const void* data, unsigned size) 
{
	unsigned off = *address;

	if (size) {
		dosmemput(data, size, off + vdd_memory_segment * 16);
	}

	*address += size;
	
	*address = (*address + 0xF) & ~0xF;

	return off;
}

static int vdd_register(void)
{
	__dpmi_regs r;
	unsigned address = 0;

	vdd_memory_segment = __dpmi_allocate_dos_memory(16, &vdd_memory_selector);

	if (vdd_memory_segment == -1) {
		return -1;
	}

	memset(&r, 0, sizeof(r));

	r.x.cs = vdd_memory_segment;
	r.x.ds = vdd_memory_segment;
	r.x.es = vdd_memory_segment;

	r.x.ip = vdd_allocate(&address, vdd_register_module, sizeof(vdd_register_module));
	r.x.si = vdd_allocate(&address, vdd_dll, sizeof(vdd_dll));
	r.x.di = vdd_allocate(&address, vdd_init, sizeof(vdd_init));
	r.x.bx = vdd_allocate(&address, vdd_dispatch, sizeof(vdd_dispatch));

	if (__dpmi_simulate_real_mode_procedure_retf(&r) != 0) {
		adv_svgalib_log("svgalib: __dpmi_simulate_real_mode_procedure_retf failed\n");
		return -1;
	}

	if (r.x.flags & 1) {
		adv_svgalib_log("svgalib: vdd_register failed, ax = 0x%x\n", (unsigned)r.x.ax);
		return -1;
	}

	vdd_handle = r.x.ax;

	return 0;
}

static void vdd_unregister(void)
{
	__dpmi_regs r;
	unsigned address = 0;
	
	memset(&r, 0, sizeof(r));
	
	r.x.cs = vdd_memory_segment;
	
	r.x.ip = vdd_allocate(&address, vdd_unregister_module, sizeof(vdd_unregister_module));
	
	r.x.ax = vdd_handle;

	if (__dpmi_simulate_real_mode_procedure_retf(&r) != 0) {
		adv_svgalib_log("svgalib: __dpmi_simulate_real_mode_procedure_retf failed. ignore.\n");
	}
}

static int vdd_ioctl(unsigned ioctl, void* in, unsigned in_size, void* out, unsigned out_size, unsigned* out_returned)
{
	__dpmi_regs r;
	unsigned address = 0;
	unsigned out_address;

	memset(&r, 0, sizeof(r));

	r.x.cs = vdd_memory_segment;
	r.x.ds = vdd_memory_segment;
	r.x.es = vdd_memory_segment;

	r.x.ip = vdd_allocate(&address, vdd_dispatch_call, sizeof(vdd_dispatch_call));

	r.x.ax = vdd_handle;
	r.x.bx = ioctl;
	r.x.cx = in_size;
	r.x.dx = out_size;
	r.x.si = vdd_allocate(&address, in, in_size);
	r.x.di = out_address = vdd_allocate(&address, out, out_size);
	
	if (__dpmi_simulate_real_mode_procedure_retf(&r) != 0) {
		adv_svgalib_log("svgalib: __dpmi_simulate_real_mode_procedure_retf failed\n");
		return -1;
	}

	if (r.x.flags & 1) {
		adv_svgalib_log("svgalib: vdd_dispatch_call failed, ioctl 0x%x, func 0x%x, in %d, out %d\n", ioctl, (ioctl >> 2) & 0xFFF, in_size, out_size);
		return -1;
	}

	*out_returned = r.x.cx;

	if (*out_returned && *out_returned <= out_size)
		dosmemget(out_address + vdd_memory_segment * 16, *out_returned, out);

	return 0;
}

/**************************************************************************/
/* ioctl */

int adv_svgalib_ioctl(unsigned code, void* input, unsigned input_size, void* output, unsigned output_size)
{
	unsigned output_returned;

	if (vdd_ioctl(code, input, input_size, output, output_size, &output_returned) != 0) {
		return -1;
	}

	if (output_returned != output_size) {
		return -1;
	}

	return 0;
}

/**************************************************************************/
/* port */

#if defined(USE_GIVEIO) || defined(USE_TOTALIO)
unsigned char adv_svgalib_inportb(unsigned port)
{
	unsigned char rv;
	__asm__ __volatile__ ("inb %%dx, %0"
		: "=a" (rv)
		: "d" (port));
	return rv;
}

unsigned short adv_svgalib_inportw(unsigned port)
{
	unsigned short rv;
	__asm__ __volatile__ ("inw %%dx, %0"
		: "=a" (rv)
		: "d" (port));
	return rv;
}

unsigned adv_svgalib_inportl(unsigned port)
{
	unsigned rv;
	__asm__ __volatile__ ("inl %%dx, %0"
		: "=a" (rv)
		: "d" (port));
	return rv;
}

void adv_svgalib_outportb(unsigned port, unsigned char data)
{
	__asm__ __volatile__ ("outb %1, %%dx"
		:
		: "d" (port),
		"a" (data));
}

void adv_svgalib_outportw(unsigned port, unsigned short data)
{
	__asm__ __volatile__ ("outw %1, %%dx"
		:
		: "d" (port),
		"a" (data));
}

void adv_svgalib_outportl(unsigned port, unsigned data)
{
	__asm__ __volatile__ ("outl %1, %%dx"
		:
		: "d" (port),
		"a" (data));
}

#else

void adv_svgalib_outportb(unsigned port, unsigned char data)
{
	SVGALIB_PORT_WRITE_IN in;

	in.port = port;
	in.size = 1;
	in.data = data;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PORT_WRITE, &in, sizeof(in), 0, 0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PORT_WRITE failed\n");
	}
}

void adv_svgalib_outportw(unsigned port, unsigned short data)
{
	SVGALIB_PORT_WRITE_IN in;

	in.port = port;
	in.size = 2;
	in.data = data;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PORT_WRITE, &in, sizeof(in), 0, 0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PORT_WRITE failed\n");
	}
}

void adv_svgalib_outportl(unsigned port, unsigned data)
{
	SVGALIB_PORT_WRITE_IN in;

	in.port = port;
	in.size = 4;
	in.data = data;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PORT_WRITE, &in, sizeof(in), 0, 0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PORT_WRITE failed\n");
	}
}

unsigned char adv_svgalib_inportb(unsigned port)
{
	SVGALIB_PORT_READ_IN in;
	SVGALIB_PORT_READ_OUT out;

	in.port = port;
	in.size = 1;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PORT_READ, &in, sizeof(in), &out, sizeof(out)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PORT_READ failed\n");
		return 0;
	}

	return out.data;
}

unsigned short adv_svgalib_inportw(unsigned port)
{
	SVGALIB_PORT_READ_IN in;
	SVGALIB_PORT_READ_OUT out;

	in.port = port;
	in.size = 2;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PORT_READ, &in, sizeof(in), &out, sizeof(out)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PORT_READ failed\n");
		return 0;
	}

	return out.data;
}

unsigned adv_svgalib_inportl(unsigned port)
{
	SVGALIB_PORT_READ_IN in;
	SVGALIB_PORT_READ_OUT out;

	in.port = port;
	in.size = 4;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PORT_READ, &in, sizeof(in), &out, sizeof(out)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PORT_READ failed\n");
		return 0;
	}

	return out.data;
}
#endif

/**************************************************************************/
/* pci */

int adv_svgalib_pci_bus_max(unsigned* bus_max)
{
	SVGALIB_PCI_BUS_OUT out;
	
	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PCI_BUS, 0, 0, &out, sizeof(out)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PCI_BUS failed\n");
		return -1;
	}

	*bus_max = out.bus;
	return 0;
}

int adv_svgalib_pci_read_byte(unsigned bus_device_func, unsigned reg, unsigned char* value) 
{
	SVGALIB_PCI_READ_IN in;
	SVGALIB_PCI_READ_OUT out;

	in.bus_device_func = bus_device_func;
	in.offset = reg;
	in.size = 1;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PCI_READ, &in, sizeof(in), &out, sizeof(out)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PCI_READ failed\n");
		return -1;
	}

	*value = out.data;
	return 0;
}

int adv_svgalib_pci_read_word(unsigned bus_device_func, unsigned reg, unsigned short* value) {
	SVGALIB_PCI_READ_IN in;
	SVGALIB_PCI_READ_OUT out;

	in.bus_device_func = bus_device_func;
	in.offset = reg;
	in.size = 2;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PCI_READ, &in, sizeof(in), &out, sizeof(out)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PCI_READ failed\n");
		return -1;
	}

	*value = out.data;
	return 0;
}

int adv_svgalib_pci_read_dword(unsigned bus_device_func, unsigned reg, unsigned* value) {
	SVGALIB_PCI_READ_IN in;
	SVGALIB_PCI_READ_OUT out;

	in.bus_device_func = bus_device_func;
	in.offset = reg;
	in.size = 4;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PCI_READ, &in, sizeof(in), &out, sizeof(out)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PCI_READ failed\n");
		return -1;
	}

	*value = out.data;
	return 0;
}

int adv_svgalib_pci_read_dword_nolog(unsigned bus_device_func, unsigned reg, unsigned* value) {
	SVGALIB_PCI_READ_IN in;
	SVGALIB_PCI_READ_OUT out;

	in.bus_device_func = bus_device_func;
	in.offset = reg;
	in.size = 4;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PCI_READ, &in, sizeof(in), &out, sizeof(out)) != 0) {
		return -1;
	}

	*value = out.data;
	return 0;
}

int adv_svgalib_pci_write_byte(unsigned bus_device_func, unsigned reg, unsigned char value) {
	SVGALIB_PCI_WRITE_IN in;

	in.bus_device_func = bus_device_func;
	in.offset = reg;
	in.size = 1;
	in.data = value;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PCI_WRITE, &in, sizeof(in), 0, 0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PCI_WRITE failed\n");
		return -1;
	}

	return 0;
}

int adv_svgalib_pci_write_word(unsigned bus_device_func, unsigned reg, unsigned short value) {
	SVGALIB_PCI_WRITE_IN in;

	in.bus_device_func = bus_device_func;
	in.offset = reg;
	in.size = 2;
	in.data = value;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PCI_WRITE, &in, sizeof(in), 0, 0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PCI_WRITE failed\n");
		return -1;
	}

	return 0;
}

int adv_svgalib_pci_write_dword(unsigned bus_device_func, unsigned reg, unsigned value) {
	SVGALIB_PCI_WRITE_IN in;

	in.bus_device_func = bus_device_func;
	in.offset = reg;
	in.size = 4;
	in.data = value;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_PCI_WRITE, &in, sizeof(in), 0, 0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_PCI_WRITE failed\n");
		return -1;
	}

	return 0;
}

/**************************************************************************/
/* device */

/**
 * The video board bus number.
 */
static unsigned the_bus;

static int bus_callback(unsigned bus_device_func, unsigned vendor, unsigned device, void* _arg) {
	unsigned dw;
	unsigned base_class;

	if (adv_svgalib_pci_read_dword(bus_device_func,0x8,&dw)!=0)
		return 0;

	base_class = (dw >> 16) & 0xFFFF;
	if (base_class != 0x300 /* DISPLAY | VGA */)
		return 0;

	*(unsigned*)_arg = bus_device_func;

	return 1;
}

int adv_svgalib_open(void) {
	int r;
	unsigned bus_device_func;
	SVGALIB_VERSION_OUT version;

	if (vdd_register() != 0) {
		adv_svgalib_log("svgalib: error opening the SVGAWIN device\n");
		return -1;
	}

	/* check the version */
	if (adv_svgalib_ioctl(IOCTL_SVGALIB_VERSION,0,0,&version,sizeof(version)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_VERSION failed\n");
		vdd_unregister();
		return -1;
	}
	if (version.version < SVGALIB_VERSION) {
		adv_svgalib_log("svgalib: invalid SVGALIB version %08x. Minimun required is %08x.\n", (unsigned)version.version, (unsigned)SVGALIB_VERSION);
		vdd_unregister();
		return -1;
	}

#ifdef USE_TOTALIO
	/* get the permission on all the IO port */
	if (adv_svgalib_ioctl(IOCTL_SVGALIB_TOTALIO_ON,0,0,0,0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_TOTALIO_ON failed\n");
		vdd_unregister();
		return -1;
	}
#endif

#ifdef USE_GIVEIO
	/* get the permission on all the IO port */
	if (adv_svgalib_ioctl(IOCTL_SVGALIB_GIVEIO_ON,0,0,0,0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_GIVEIO_ON failed\n");
		vdd_unregister();
		return -1;
	}
#endif

	/* search the bus on which is the first video board */
	r = adv_svgalib_pci_scan_device(bus_callback,&bus_device_func);
	if (r != 0) {
		/* found */
		the_bus = (bus_device_func >> 8) & 0xFF;
	} else {
		adv_svgalib_log("svgalib: adv_svgalib_pci_scan_device has not found a video board\n");
		the_bus = 0;
	}

	return 0;
}

void adv_svgalib_close(void) {
#ifdef USE_GIVEIO
	if (adv_svgalib_ioctl(IOCTL_SVGALIB_GIVEIO_OFF,0,0,0,0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_GIVEIO_OFF failed\n");
	}
#endif

#ifdef USE_TOTALIO
	if (adv_svgalib_ioctl(IOCTL_SVGALIB_TOTALIO_OFF,0,0,0,0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_TOTALIO_OFF failed\n");
	}
#endif

	vdd_unregister();
}

/**************************************************************************/
/* mmap */

void* adv_svgalib_mmap(void* start, unsigned length, int prot, int flags, int fd, unsigned offset) {
	SVGALIB_MAP_IN in;
	SVGALIB_MAP_OUT out;
	unsigned long linear;

	(void)prot;
	(void)fd;

	if ((flags & MAP_FIXED) != 0)
		return MAP_FAILED;

#warning TODO
	/* The  __djgpp_nearptr_enable() call always fails. An interaction with the lower driver */
	/* is required to change the segment limits. */
	return MAP_FAILED;

	if ((_crt0_startup_flags & _CRT0_FLAG_NEARPTR) == 0) {
		if (!__djgpp_nearptr_enable())
			return MAP_FAILED;
		_crt0_startup_flags |= _CRT0_FLAG_NEARPTR;
	}

	in.address.QuadPart = offset;
	in.bus = the_bus;
	in.size = length;

	adv_svgalib_log("svgalib: mapping address %08x, size %d, bus %d\n", offset, length, the_bus);

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_MAP, &in, sizeof(in), &out, sizeof(out)) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_MAP failed\n");
		return MAP_FAILED;
	}

	linear = (unsigned)out.address;

	linear += __djgpp_conventional_base;

	adv_svgalib_log("svgalib: mapped pointer %08x\n", (unsigned)linear);

	return (void*)linear;
}

int adv_svgalib_munmap(void* start, unsigned length) {
	SVGALIB_UNMAP_IN in;
	unsigned long offset;

	adv_svgalib_log("svgalib: unmapping pointer %08x, size %d\n", (unsigned)start, length);

	offset = (unsigned)start;

	offset -= __djgpp_conventional_base;

	in.address = (void*)offset;

	if (adv_svgalib_ioctl(IOCTL_SVGALIB_UNMAP, &in, sizeof(in), 0, 0) != 0) {
		adv_svgalib_log("svgalib: ioctl IOCTL_SVGALIB_UNMAP failed\n");	
		return -1;
	}

	return 0;
}

/**************************************************************************/
/* iopl */

int adv_svgalib_iopl(int perm) {
	(void)perm;
	return 0;
}

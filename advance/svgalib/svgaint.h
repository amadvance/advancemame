#ifndef __SVGAINT_H
#define __SVGAINT_H

/**************************************************************************/
/* os */

#define MAP_SHARED 0x1
#define MAP_FIXED 0x2
#define MAP_FAILED ((void*)(-1))

#ifndef __WIN32__ /* TODO */
#include <sys/types.h>
#include <sys/mman.h>
#else
#ifdef TEXT
#undef TEXT
#define TEXT 0
#endif
#define PROT_READ 0
#define PROT_WRITE 0
#define sigemptyset(a)
#define sigaddset(a,b)
#define sigprocmask(a,b,c)
#endif

/**************************************************************************/
/* internal */

#include "driver.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

/* Override of os functions. */
#ifndef USE_SVGALIB_EXTERNAL
#define malloc adv_svgalib_malloc
#define calloc adv_svgalib_calloc
#define free adv_svgalib_free
#define mmap adv_svgalib_mmap
#define munmap adv_svgalib_munmap
#define iopl adv_svgalib_iopl
#define fprintf adv_svgalib_fprintf
#ifdef stderr
#undef stderr
#endif
#define stderr adv_svgalib_stderr()
#define usleep adv_svgalib_usleep
#endif

void* adv_svgalib_mmap(void* start, unsigned length, int prot, int flags, int fd, unsigned offset);
int adv_svgalib_munmap(void* start, unsigned length);
int adv_svgalib_iopl(int perm);
void* adv_svgalib_malloc(unsigned size);
void* adv_svgalib_calloc(unsigned n, unsigned size);
void adv_svgalib_free(void*);
void adv_svgalib_fprintf(void*, const char* format, ...);
void* adv_svgalib_stderr();
int adv_svgalib_open(void);
void adv_svgalib_close(void);
void adv_svgalib_usleep(unsigned);
void adv_svgalib_mode_init(unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, int doublescan, int interlace, int hsync, int vsync, unsigned bits_per_pixel, int tvpal, int tvntsc);
void adv_svgalib_mode_done(void);
void adv_svgalib_enable(void);
void adv_svgalib_disable(void);
unsigned char adv_svgalib_inportb(unsigned port);
unsigned short adv_svgalib_inportw(unsigned port);
unsigned adv_svgalib_inportl(unsigned port);
void adv_svgalib_outportb(unsigned port, unsigned char data);
void adv_svgalib_outportw(unsigned port, unsigned short data);
void adv_svgalib_outportl(unsigned port, unsigned data);
int adv_svgalib_pci_read_byte(unsigned bus_device_func, unsigned reg, unsigned char* value);
int adv_svgalib_pci_read_word(unsigned bus_device_func, unsigned reg, unsigned short* value);
int adv_svgalib_pci_read_dword(unsigned bus_device_func, unsigned reg, unsigned* value);
int adv_svgalib_pci_write_byte(unsigned bus_device_func, unsigned reg, unsigned char value);
int adv_svgalib_pci_write_word(unsigned bus_device_func, unsigned reg, unsigned short value);
int adv_svgalib_pci_write_dword(unsigned bus_device_func, unsigned reg, unsigned value);
int adv_svgalib_pci_read_dword_aperture_len(unsigned bus_device_func, unsigned reg, unsigned* value);
int adv_svgalib_pci_scan_device(int (*callback)(unsigned bus_device_func,unsigned vendor,unsigned device, void* arg), void* arg);

/**************************************************************************/
/* driver */

#define INCLUDE_NV3_DRIVER
#define INCLUDE_TRIDENT_DRIVER
#define INCLUDE_RENDITION_DRIVER
#define INCLUDE_G400_DRIVER
#define INCLUDE_PM2_DRIVER
#define INCLUDE_SAVAGE_DRIVER
#define INCLUDE_MILLENNIUM_DRIVER
#define INCLUDE_R128_DRIVER
#define INCLUDE_BANSHEE_DRIVER
#define INCLUDE_SIS_DRIVER
#define INCLUDE_I740_DRIVER
/* #define INCLUDE_I810_DRIVER */ /* TODO No linear mode from 1.9.15. It requires special kernel support. */
#define INCLUDE_LAGUNA_DRIVER
#define INCLUDE_RAGE_DRIVER
#define INCLUDE_MX_DRIVER
/* #define INCLUDE_NEO_DRIVER */ /* Don't support the external screen, only the LCD */
/* #define INCLUDE_CHIPS_DRIVER */ /* Not imported */
/* #define INCLUDE_MACH64_DRIVER */ /* Not imported */
/* #define INCLUDE_MACH32_DRIVER */ /* Not imported */
/* #define INCLUDE_EGA_DRIVER */ /* Not a modeline driver */
#define INCLUDE_ET6000_DRIVER
/* #define INCLUDE_ET4000_DRIVER */ /* Not imported */
/* #define INCLUDE_TVGA_DRIVER */ /* Not a modeline driver */
/* #define INCLUDE_CIRRUS_DRIVER */ /* Not a modeline driver */
/* #define INCLUDE_OAK_DRIVER */ /* Not a modeline driver */
/* #define INCLUDE_PARADISE_DRIVER */ /* Not a modeline driver */
#define INCLUDE_S3_DRIVER
/* #define INCLUDE_ET3000_DRIVER */ /* Not a modeline driver */
#define INCLUDE_ARK_DRIVER
/* #define INCLUDE_GVGA6400_DRIVER */ /* Not a modeline driver */
/* #define INCLUDE_ATI_DRIVER */ /* Not a modeline driver */
/* #define INCLUDE_ALI_DRIVER */ /* Not a modeline driver */
#define INCLUDE_APM_DRIVER

/**************************************************************************/
/* ramdac */

#define INCLUDE_NORMAL_DAC
#define INCLUDE_S3_SDAC_DAC
#define INCLUDE_S3_GENDAC_DAC
#define INCLUDE_S3_TRIO64_DAC
#define INCLUDE_SIERRA_DAC
#define INCLUDE_SC15025_DAC
#define INCLUDE_ATT20C490_DAC
#define INCLUDE_ATT20C498_DAC
#define INCLUDE_ICW_DAC
#define INCLUDE_IBMRGB52x_DAC
#define INCLUDE_SC1148X_DAC
#define INCLUDE_ICS_GENDAC_DAC

/**************************************************************************/
/* ramdac test */

#define INCLUDE_NORMAL_DAC_TEST
#define INCLUDE_S3_SDAC_DAC_TEST
#define INCLUDE_S3_GENDAC_DAC_TEST
#define INCLUDE_S3_TRIO64_DAC_TEST
#define INCLUDE_SIERRA_DAC_TEST
#define INCLUDE_SC15025_DAC_TEST
#define INCLUDE_ATT20C490_DAC_TEST
#define INCLUDE_ATT20C498_DAC_TEST
#define INCLUDE_ICW_DAC_TEST
#define INCLUDE_IBMRGB52x_DAC_TEST
#define INCLUDE_SC1148X_DAC_TEST
#define INCLUDE_ICS_GENDAC_DAC_TEST

/**************************************************************************/
/* adv_svgalib */

struct adv_svgalib_crtc_struct {
	unsigned hde, hrs, hre, ht;
	unsigned vde, vrs, vre, vt;

	int nhsync;
	int nvsync;
	int doublescan;
	int interlace;
	unsigned pixelclock;
};

struct adv_svgalib_mode_struct {
	unsigned width; /* Size of the mode */
	unsigned height;

	unsigned bits_per_pixel; /* Bits per pixel */
	unsigned bytes_per_pixel; /* Bytes per pixel */
	unsigned bytes_per_scanline; /* Bytes per scanline */

	/* Rgb format */
	unsigned red_len;
	unsigned red_pos;
	unsigned green_len;
	unsigned green_pos;
	unsigned blue_len;
	unsigned blue_pos;
};

struct adv_svgalib_chipset_struct {
	DriverSpecs* drv;
	int chipset;
	const char* name;
	unsigned cap;
};

struct adv_svgalib_state_struct {
	struct adv_svgalib_chipset_struct* driver;

	struct adv_svgalib_crtc_struct crtc;
	struct adv_svgalib_mode_struct mode;
	unsigned char* pointer;

	int mode_number;
	int mode_tvpal;
	int mode_tvntsc;

	int divide_clock; /**< If set uses the VGA sequencer register to divide the dot clock by 2. */

	int has_interlace;
	int has_bit8;
	int has_bit15;
	int has_bit16;
	int has_bit24;
	int has_bit32;
	int has_tvntsc;
	int has_tvpal;
};

extern struct adv_svgalib_state_struct adv_svgalib_state;

#ifdef USE_SVGALIB_EXTERNAL

int adv_svgalib_init(int divide_clock_with_sequencer);
void adv_svgalib_done(void);

int adv_svgalib_detect(const char* name);

int adv_svgalib_set(unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, int doublescan, int interlace, int hsync, int vsync, unsigned bits_per_pixel, int tvpal, int tvntsc);
void adv_svgalib_unset(void);

void adv_svgalib_save(unsigned char* regs);
void adv_svgalib_restore(unsigned char* regs);

void adv_svgalib_linear_map(void);
void adv_svgalib_linear_unmap(void);

void adv_svgalib_scroll_set(unsigned offset);
void adv_svgalib_scanline_set(unsigned byte_length);
void adv_svgalib_palette_set(unsigned index, unsigned r, unsigned g, unsigned b);
void adv_svgalib_wait_vsync(void);

void adv_svgalib_on(void);
void adv_svgalib_off(void);

static inline const char* adv_svgalib_driver_get() {
	return adv_svgalib_state.driver->name;
}

static inline unsigned char* adv_svgalib_linear_pointer_get(void) {
	return __svgalib_linear_pointer;
}

static inline unsigned adv_svgalib_linear_base_get(void) {
	return __svgalib_linear_mem_base;
}

static inline unsigned adv_svgalib_linear_size_get(void) {
	return __svgalib_linear_mem_size;
}

static inline unsigned char* adv_svgalib_mmio_pointer_get(void) {
	return __svgalib_mmio_pointer;
}

static inline unsigned adv_svgalib_mmio_base_get(void) {
	return __svgalib_mmio_base;
}

static inline unsigned adv_svgalib_mmio_size_get(void) {
	return __svgalib_mmio_size;
}

static inline unsigned adv_svgalib_bytes_per_scanline_get(void) {
	return adv_svgalib_state.mode.bytes_per_scanline;
}

static inline unsigned adv_svgalib_bytes_per_pixel_get(void) {
	return adv_svgalib_state.mode.bytes_per_pixel;
}

#endif

#endif

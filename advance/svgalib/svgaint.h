/** \file
 * SVGALIB user interface definition.
 */

#ifndef __SVGAINT_H
#define __SVGAINT_H

/**************************************************************************/
/* os and compiler */

#if !defined(__GNUC__) 
#define inline
#define __attribute__()
#endif

#define MAP_SHARED 0x1
#define MAP_FIXED 0x2
#define MAP_FAILED ((void*)(-1))

#if defined(USE_ADV_SVGALIB_WINK)

#define ADV_SVGALIB_CALL __stdcall
#define ADV_SVGALIB_CALL_VARARGS __cdecl
#define PROT_READ 0
#define PROT_WRITE 0
#define sigemptyset(a) do { (void)(a); } while (0)
#define sigaddset(a,b)
#define sigprocmask(a,b,c)

#elif defined(USE_ADV_SVGALIB_WIN)

#define ADV_SVGALIB_CALL
#define ADV_SVGALIB_CALL_VARARGS
#ifdef TEXT
#undef TEXT
#define TEXT 0
#endif
#define PROT_READ 0
#define PROT_WRITE 0
#define sigemptyset(a) do { (void)(a); } while (0)
#define sigaddset(a,b)
#define sigprocmask(a,b,c)

#elif defined(USE_ADV_SVGALIB_DOS)

#define ADV_SVGALIB_CALL
#define ADV_SVGALIB_CALL_VARARGS
#include <sys/types.h>
#include <sys/mman.h>

#elif defined(USE_ADV_SVGALIB_DOSK)

#define ADV_SVGALIB_INTERNAL_HEAP
#define ADV_SVGALIB_CALL
#define ADV_SVGALIB_CALL_VARARGS
#include <sys/types.h>
#include <sys/mman.h>

#else
#error No USE_ADV_SVGALIB_* defined
#endif

/**************************************************************************/
/* internal */

#include <stdint.h>

#include "svgadriv.h"

void ADV_SVGALIB_CALL adv_svgalib_log_va(const char *text, va_list arg);

void ADV_SVGALIB_CALL_VARARGS adv_svgalib_log(const char *text, ...) __attribute__((format(printf,1,2)));

/**
 * Map a memory region.
 * Same calling convetion of map().
 * \return
 *  - ==MAP_FAILED on error (MAP_FAILED is !=0)
 *  - !=MAP_FAILED the pointer.
 */
void* ADV_SVGALIB_CALL adv_svgalib_mmap(void* start, unsigned length, int prot, int flags, int fd, unsigned offset);

/**
 * UnMap a memory region.
 * Same calling convetion of unmap().
 */
int ADV_SVGALIB_CALL adv_svgalib_munmap(void* start, unsigned length);

int ADV_SVGALIB_CALL adv_svgalib_iopl(int perm);
void* ADV_SVGALIB_CALL adv_svgalib_malloc(unsigned size);
void* ADV_SVGALIB_CALL adv_svgalib_calloc(unsigned n, unsigned size);
void ADV_SVGALIB_CALL adv_svgalib_free(void*);
void ADV_SVGALIB_CALL adv_svgalib_fprintf(void*, const char* format, ...);
void ADV_SVGALIB_CALL adv_svgalib_printf(const char* format, ...);
void* ADV_SVGALIB_CALL adv_svgalib_stderr();
int ADV_SVGALIB_CALL adv_svgalib_open(void);
void ADV_SVGALIB_CALL adv_svgalib_close(void);
void ADV_SVGALIB_CALL adv_svgalib_usleep(unsigned);
void ADV_SVGALIB_CALL adv_svgalib_enable(void);
void ADV_SVGALIB_CALL adv_svgalib_disable(void);
unsigned char ADV_SVGALIB_CALL adv_svgalib_inportb(unsigned port);
unsigned short ADV_SVGALIB_CALL adv_svgalib_inportw(unsigned port);
unsigned ADV_SVGALIB_CALL adv_svgalib_inportl(unsigned port);
void ADV_SVGALIB_CALL adv_svgalib_outportb(unsigned port, unsigned char data);
void ADV_SVGALIB_CALL adv_svgalib_outportw(unsigned port, unsigned short data);
void ADV_SVGALIB_CALL adv_svgalib_outportl(unsigned port, unsigned data);
int ADV_SVGALIB_CALL adv_svgalib_pci_bus_max(unsigned* bus_max);
int ADV_SVGALIB_CALL adv_svgalib_pci_read_byte(unsigned bus_device_func, unsigned reg, unsigned char* value);
int ADV_SVGALIB_CALL adv_svgalib_pci_read_word(unsigned bus_device_func, unsigned reg, unsigned short* value);
int ADV_SVGALIB_CALL adv_svgalib_pci_read_dword(unsigned bus_device_func, unsigned reg, unsigned* value);
int ADV_SVGALIB_CALL adv_svgalib_pci_read_dword_nolog(unsigned bus_device_func, unsigned reg, unsigned* value);
int ADV_SVGALIB_CALL adv_svgalib_pci_write_byte(unsigned bus_device_func, unsigned reg, unsigned char value);
int ADV_SVGALIB_CALL adv_svgalib_pci_write_word(unsigned bus_device_func, unsigned reg, unsigned short value);
int ADV_SVGALIB_CALL adv_svgalib_pci_write_dword(unsigned bus_device_func, unsigned reg, unsigned value);
int ADV_SVGALIB_CALL adv_svgalib_pci_read_dword_aperture_len(unsigned bus_device_func, unsigned reg, unsigned* value);
int ADV_SVGALIB_CALL adv_svgalib_pci_scan_device(int (*callback)(unsigned bus_device_func,unsigned vendor,unsigned device, void* arg), void* arg);
void ADV_SVGALIB_CALL adv_svgalib_exit(int code);
char* ADV_SVGALIB_CALL adv_svgalib_strtok(const char* s, const char* t);
double ADV_SVGALIB_CALL adv_svgalib_atof(const char* s);
int ADV_SVGALIB_CALL adv_svgalib_strcasecmp(const char* s1, const char* s2);
void ADV_SVGALIB_CALL adv_svgalib_abort(void);

/* Override of os functions. */
#ifndef USE_SVGALIB_EXTERNAL
#define malloc adv_svgalib_malloc
#define calloc adv_svgalib_calloc
#define free adv_svgalib_free
#define mmap adv_svgalib_mmap
#define munmap adv_svgalib_munmap
#define iopl adv_svgalib_iopl
#define printf adv_svgalib_printf
#define fprintf adv_svgalib_fprintf
#ifdef stderr
#undef stderr
#endif
#define stderr adv_svgalib_stderr()
#define usleep adv_svgalib_usleep
#define exit adv_svgalib_exit
#define strtok adv_svgalib_strtok
#define atof adv_svgalib_atof
#define strcasecmp adv_svgalib_strcasecmp
#endif

/**************************************************************************/
/* driver */

#define INCLUDE_NV3_DRIVER
#define INCLUDE_TRIDENT_DRIVER
#define INCLUDE_RENDITION_DRIVER
#define INCLUDE_G400_DRIVER
#define INCLUDE_PM2_DRIVER
#define INCLUDE_UNICHROME_DRIVER
#define INCLUDE_SAVAGE_DRIVER
#define INCLUDE_MILLENNIUM_DRIVER
#define INCLUDE_R128_DRIVER
#define INCLUDE_BANSHEE_DRIVER
#define INCLUDE_SIS_DRIVER
/* #define INCLUDE_I740_DRIVER */ /* Not imported */
/* #define INCLUDE_I810_DRIVER */ /* Requires special support */
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
	int skip_board; /**< Number of boards to skip. Normally 0. */
	unsigned last_bus_device_func; /**< Last device found. */

	int has_interlace;
	int has_bit8;
	int has_bit15;
	int has_bit16;
	int has_bit24;
	int has_bit32;
	int has_tvntsc;
	int has_tvpal;
};

/**
 * Global SVGALIB state.
 */
extern struct adv_svgalib_state_struct adv_svgalib_state;

/**
 * Size in bytes of the video board state used by adv_svgalib_save() and adv_svgalib_restore().
 */
#define ADV_SVGALIB_STATE_SIZE MAX_REGS

#ifdef USE_SVGALIB_EXTERNAL

int ADV_SVGALIB_CALL adv_svgalib_init(int divide_clock_with_sequencer, int skipboard);
void ADV_SVGALIB_CALL adv_svgalib_done(void);

int ADV_SVGALIB_CALL adv_svgalib_detect(const char* name);

int ADV_SVGALIB_CALL adv_svgalib_set(unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, int doublescan, int interlace, int hsync, int vsync, unsigned bits_per_pixel, int tvpal, int tvntsc);
void ADV_SVGALIB_CALL adv_svgalib_unset(void);
int ADV_SVGALIB_CALL adv_svgalib_check(unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, int doublescan, int interlace, int hsync, int vsync, unsigned bits_per_pixel, int tvpal, int tvntsc);

void ADV_SVGALIB_CALL adv_svgalib_save(unsigned char* regs);
void ADV_SVGALIB_CALL adv_svgalib_restore(unsigned char* regs);

void ADV_SVGALIB_CALL adv_svgalib_linear_map(void);
void ADV_SVGALIB_CALL adv_svgalib_linear_unmap(void);

void ADV_SVGALIB_CALL adv_svgalib_scroll_set(unsigned offset);
void ADV_SVGALIB_CALL adv_svgalib_scanline_set(unsigned byte_length);
void ADV_SVGALIB_CALL adv_svgalib_palette_set(unsigned index, unsigned r, unsigned g, unsigned b);
void ADV_SVGALIB_CALL adv_svgalib_wait_vsync(void);

void ADV_SVGALIB_CALL adv_svgalib_on(void);
void ADV_SVGALIB_CALL adv_svgalib_off(void);

void ADV_SVGALIB_CALL adv_svgalib_cursor_on(void);
void ADV_SVGALIB_CALL adv_svgalib_cursor_off(void);

static inline const char* ADV_SVGALIB_CALL adv_svgalib_driver_get() {
	if (adv_svgalib_state.driver)
		return adv_svgalib_state.driver->name;
	else
		return 0;
}

static inline unsigned char* ADV_SVGALIB_CALL adv_svgalib_linear_pointer_get(void) {
	return __svgalib_linear_pointer;
}

static inline unsigned ADV_SVGALIB_CALL adv_svgalib_linear_base_get(void) {
	return __svgalib_linear_mem_base;
}

static inline unsigned ADV_SVGALIB_CALL adv_svgalib_linear_size_get(void) {
	return __svgalib_linear_mem_size;
}

static inline unsigned char* ADV_SVGALIB_CALL adv_svgalib_mmio_pointer_get(void) {
	return __svgalib_mmio_pointer;
}

static inline unsigned ADV_SVGALIB_CALL adv_svgalib_mmio_base_get(void) {
	return __svgalib_mmio_base;
}

static inline unsigned ADV_SVGALIB_CALL adv_svgalib_mmio_size_get(void) {
	return __svgalib_mmio_size;
}

static inline unsigned ADV_SVGALIB_CALL adv_svgalib_scanline_get(void) {
	return adv_svgalib_state.mode.bytes_per_scanline;
}

static inline unsigned ADV_SVGALIB_CALL adv_svgalib_pixel_get(void) {
	return adv_svgalib_state.mode.bytes_per_pixel;
}

#endif

#endif

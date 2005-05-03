/** \file
 * SVGALIB user interface implementation.
 */

#include "svgalib.h"

#include "libvga.h"
#include "timing.h"
#include "svgadriv.h"
#include "vga.h"
#include "vgaio.h"
#include "ramdac/ramdac.h"

struct adv_svgalib_state_struct adv_svgalib_state;

/**************************************************************************/
/* heap */

#ifdef ADV_SVGALIB_INTERNAL_HEAP

#define HEAP_SIZE 16384

static unsigned char heap[HEAP_SIZE];

struct heap_slot {
	struct heap_slot* prev;
	struct heap_slot* next;
	int used;
	unsigned size;
};

#define SLOT_SIZE sizeof(struct heap_slot)

struct heap_slot* heap_list;

static struct heap_slot* heap_slot_from(void* p) {
	unsigned char* raw = (unsigned char*)p - SLOT_SIZE;
	return (struct heap_slot*)raw;
}

static unsigned char* heap_slot_to(struct heap_slot* h) {
	unsigned char* raw = (unsigned char*)h + SLOT_SIZE;
	return raw;
}

static unsigned heap_slot_begin(struct heap_slot* h) {
	return (unsigned char*)h - (unsigned char*)heap;
}

static unsigned heap_slot_size(struct heap_slot* h) {
	return h->size + SLOT_SIZE;
}

static unsigned heap_slot_end(struct heap_slot* h) {
	return heap_slot_begin(h) + heap_slot_size(h);
}

static void heap_init(void) {
	heap_list = (struct heap_slot*)heap;
	heap_list->prev = heap_list;
	heap_list->next = heap_list;
	heap_list->used = 0;
	heap_list->size = HEAP_SIZE - SLOT_SIZE;
}

void* ADV_SVGALIB_CALL adv_svgalib_malloc(unsigned size) {
	struct heap_slot* h = heap_list;
	struct heap_slot* n;
	void* r;

	if (size % SLOT_SIZE)
		size = size + SLOT_SIZE - size % SLOT_SIZE;

	while (h->used || h->size <= size + SLOT_SIZE) {
		h = h->next;
		if (h == heap_list) {
			adv_svgalib_log("ERROR:svgalib: heap full on alloc %d\n", size);
			return 0;
		}
	}

	n = (struct heap_slot*)((unsigned char*)h + size + SLOT_SIZE);
	n->prev = h;
	n->next = h->next;
	n->next->prev = n;
	n->prev->next = n;
	n->used = 0;
	n->size = h->size - size - SLOT_SIZE;
	h->used = 1;
	h->size = size;

	r = heap_slot_to(h);

	/* ensure a constant behaviour */
	memset(r, 0, size);

	return r;
}

void ADV_SVGALIB_CALL adv_svgalib_free(void* ptr) {
	struct heap_slot* h = heap_slot_from(ptr);

	if (h->prev->used == 0 && heap_slot_end(h->prev) == heap_slot_begin(h)
		&& h->next->used == 0 && heap_slot_end(h) == heap_slot_begin(h->next)
	) {
		/* cat to prev and next */
		h->prev->size += heap_slot_size(h) + heap_slot_size(h->next);
		h->next->next->prev = h->prev;
		h->prev->next = h->next->next;
	} else if (h->prev->used == 0 && heap_slot_end(h->prev) == heap_slot_begin(h)) {
		/* cat to prev */
		h->prev->size += heap_slot_size(h);
		h->prev->next = h->next;
		h->next->prev = h->prev;
	} else if (h->next->used == 0 && heap_slot_end(h) == heap_slot_begin(h->next)) {
		/* cat to next */
		h->used = 0;
		h->size += heap_slot_size(h->next);
		h->next->next->prev = h;
		h->next = h->next->next;
	} else {
		/* keep in list */
		h->used = 0;
	}
}

#else

#ifdef malloc
#error malloc must be the real malloc
#endif

void* ADV_SVGALIB_CALL adv_svgalib_malloc(unsigned size) {
	void* r;

	r = malloc(size);

	if (!r) {
		adv_svgalib_log("ERROR:svgalib: heap full on alloc %d\n", size);
		return 0;
	}

	memset(r, 0, size);

	return r;
}

void ADV_SVGALIB_CALL adv_svgalib_free(void* ptr) {
	free(ptr);
}

#endif

void* ADV_SVGALIB_CALL adv_svgalib_calloc(unsigned n, unsigned size) {
	void* r = adv_svgalib_malloc(n * size);
	if (r)
		memset(r, 0, n*size);
	return r;
}

/**************************************************************************/
/* compatibility */

void ADV_SVGALIB_CALL adv_svgalib_printf(const char* format, ...) {
	adv_svgalib_log("svgalib: ignored adv_svgalib_printf(\"%s\",...) call\n", format);
}

void ADV_SVGALIB_CALL adv_svgalib_fprintf(void* file, const char* format, ...) {
	adv_svgalib_log("svgalib: ignored adv_svgalib_fprintf(%p,\"%s\",...) call\n", file, format);
}

void* ADV_SVGALIB_CALL adv_svgalib_stderr() {
	adv_svgalib_log("svgalib: ignored adv_svgalib_stderr() call\n");
	return 0;
}

void ADV_SVGALIB_CALL adv_svgalib_exit(int code) {
	adv_svgalib_log("svgalib: invalid adv_svgalib_exit() call\n");
	adv_svgalib_abort();
}

char* ADV_SVGALIB_CALL adv_svgalib_strtok(const char* s, const char* t) {
	adv_svgalib_log("svgalib: invalid adv_svgalib_strtok() call\n");
	adv_svgalib_abort();
	return 0;
}

double ADV_SVGALIB_CALL adv_svgalib_atof(const char* s) {
	adv_svgalib_log("svgalib: invalid adv_svgalib_atof() call\n");
	adv_svgalib_abort();
	return 0;
}

int ADV_SVGALIB_CALL adv_svgalib_strcasecmp(const char* s1, const char* s2) {
	adv_svgalib_log("svgalib: invalid adv_svgalib_strcasecmp() call\n");
	adv_svgalib_abort();
	return 0;
}

/**************************************************************************/
/* pci */

int ADV_SVGALIB_CALL adv_svgalib_pci_read_dword_aperture_len(unsigned bus_device_func, unsigned reg, unsigned* value) {
	unsigned ori;
	unsigned mask;
	unsigned len;

	if (adv_svgalib_pci_read_dword(bus_device_func, reg, &ori) != 0)
		return -1;
	if (!ori)
		return -1;
	if (adv_svgalib_pci_write_dword(bus_device_func, reg, 0xffffffff) != 0)
		return -1;
	if (adv_svgalib_pci_read_dword(bus_device_func, reg, &mask) != 0)
		return -1;
	if (adv_svgalib_pci_write_dword(bus_device_func, reg, ori) != 0)
		return -1;

	len = ~(mask & ~0xf) + 1;

	if (!len)
		return -1;

	*value = len;

	return 0;
}

int ADV_SVGALIB_CALL adv_svgalib_pci_scan_device(int (*callback)(unsigned bus_device_func,unsigned vendor,unsigned device, void* arg), void* arg)
{
	unsigned i,j;
	unsigned bus_max;

	if (adv_svgalib_pci_bus_max(&bus_max) != 0) {
		return -1;
	}

	for(i=0;i<bus_max;++i) {
		for(j=0;j<32;++j) {
			int r;
			unsigned dw;
			unsigned bus_device_func = (i << 8) | (j << 3);
			unsigned device;
			unsigned vendor;

			if (adv_svgalib_pci_read_dword_nolog(bus_device_func,0,&dw)!=0)
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
	adv_svgalib_outportb(port, value);
}

void port_outw(int value, int port)
{
	adv_svgalib_outportw(port, value);
}

void port_outl(int value, int port)
{
	adv_svgalib_outportl(port, value);
}

int port_in(int port)
{
	return adv_svgalib_inportb(port);
}

int port_inw(int port)
{
	return adv_svgalib_inportw(port);
}

int port_inl(int port)
{
	return adv_svgalib_inportl(port);
}

/**************************************************************************/
/* vgapci.c */

struct pci_find {
	unsigned vendor;
	unsigned cont;
	unsigned bus_device_func;
};

static int pci_scan_device_callback(unsigned bus_device_func, unsigned vendor, unsigned device, void* _arg) {
	unsigned dw;
	unsigned base_class;
	struct pci_find* arg = (struct pci_find*)_arg;
	(void)device;

	if (adv_svgalib_pci_read_dword_nolog(bus_device_func,0x8,&dw)!=0)
		return 0;

	base_class = (dw >> 16) & 0xFFFF;
	if (base_class != 0x300 /* DISPLAY | VGA */)
		return 0;

	if (arg->cont) {
		--arg->cont;
		return 0;
	}

	/* The original SVGALIB code tests the vendor before the count check */
	/* It is moved to allow a consistent use of the count on different vendors */
	/* and not limiting it at only the current vendor */
	if (vendor != arg->vendor)
		return 0;

	arg->bus_device_func = bus_device_func;

	return 1;
}

int __svgalib_pci_find_vendor_vga(unsigned int vendor, unsigned long *conf, int cont)
{
	int r;
	int i;
	struct pci_find find;
	find.vendor = vendor;
	find.cont = cont + adv_svgalib_state.skip_board;

	r = adv_svgalib_pci_scan_device(pci_scan_device_callback,&find);
	if (r!=1)
		return 1; /* not found */

	adv_svgalib_state.last_bus_device_func = find.bus_device_func;

	for(i=0;i<64;++i) {
		unsigned v;
		adv_svgalib_pci_read_dword(find.bus_device_func,i*4,&v);
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
	find.cont = cont + adv_svgalib_state.skip_board;

	r = adv_svgalib_pci_scan_device(pci_scan_device_callback,&find);
	if (r!=1)
		return -1; /* not found */

	adv_svgalib_state.last_bus_device_func = find.bus_device_func;

	for(i=0;i<64;++i) {
		unsigned v;
		adv_svgalib_pci_read_dword(find.bus_device_func,i*4,&v);
		conf[i] = v;
	}

	return find.bus_device_func;
}

int __svgalib_pci_read_config_byte(int pos, int address)
{
	unsigned char r;
	adv_svgalib_pci_read_byte(pos,address,&r);
	return r;
}

int __svgalib_pci_read_config_word(int pos, int address)
{
	unsigned short r;
	adv_svgalib_pci_read_word(pos,address,&r);
	return r;
}

int __svgalib_pci_read_config_dword(int pos, int address)
{
	unsigned r;
	adv_svgalib_pci_read_dword(pos,address,&r);
	return r;
}

int __svgalib_pci_read_aperture_len(int pos, int reg)
{
	unsigned r;
	unsigned address;
	address = 16+4*reg; /* this is the memory register number */
	if (adv_svgalib_pci_read_dword_aperture_len(pos,address,&r) != 0)
		return 0;
	else
		return r;
}

void __svgalib_pci_write_config_byte(int pos, int address, unsigned char data)
{
	adv_svgalib_pci_write_byte(pos,address,data);
}

void __svgalib_pci_write_config_word(int pos, int address, unsigned short data)
{
	adv_svgalib_pci_write_word(pos,address,data);
}

void __svgalib_pci_write_config_dword(int pos, int address, unsigned int data)
{
	adv_svgalib_pci_write_dword(pos,address,data);
}


/**************************************************************************/
/* memory */

#if 0
int memorytest(unsigned char *m, int max_mem) {
    unsigned char sav[256];
    int i, j;

    max_mem*=4;
    for(i=0;i<max_mem;i++) {
        sav[i]=*(m+256*1024*i);
    }
    for(i=max_mem-1;i>=0;i--) {
        *(m+256*1024*i)=i;
    }
    for(i=0;i<max_mem;i++) {
        if(*(m+256*1024*i)!=i) break;
    }
    for(j=0;j<i;j++) {
        *(m+256*1024*j)=sav[j];
    }
    return i*256;
}
#else
int memorytest(unsigned char* m, int max_mem) {
	int i; /* number of 256 kByte block */

	for(i=1;i<=max_mem*4;i*=2) {
		/* last byte of the memory */
		unsigned char* p = m + 256*1024*i - 1;
		unsigned char save = *p;

		/* check the memory functionality */
		*p = 0xff;
		if (*p != 0xff)
			break;

		*p = 0x00;
		if (*p != 0x00)
			break;

		*p = save;
	}

	return (i/2) * 256;
}
#endif

/***************************************************************************/
/* vga */

int  (*__svgalib_inmisc)(void);
void (*__svgalib_outmisc)(int);
int  (*__svgalib_incrtc)(int);
void (*__svgalib_outcrtc)(int,int);
int  (*__svgalib_inseq)(int);
void (*__svgalib_outseq)(int,int);
int  (*__svgalib_ingra)(int);
void (*__svgalib_outgra)(int,int);
int  (*__svgalib_inatt)(int);
void (*__svgalib_outatt)(int,int);
void (*__svgalib_attscreen)(int);
void (*__svgalib_inpal)(int,int*,int*,int*);
void (*__svgalib_outpal)(int,int,int,int);
int  (*__svgalib_inis1)(void);

int __svgalib_CRT_I = CRT_IC; /* fix */
int __svgalib_CRT_D = CRT_DC; /* fix */
int __svgalib_IS1_R = IS1_RC; /* fix */
int __svgalib_driver_report = 0; /* report driver used after chipset detection */

unsigned char* __svgalib_banked_pointer = (unsigned char*)0x80000000; /* UNUSED */
unsigned long int __svgalib_banked_mem_base, __svgalib_banked_mem_size; /* UNUSED */
unsigned char* __svgalib_linear_pointer = MAP_FAILED; /* aka LINEAR_POINTER */
unsigned long int __svgalib_linear_mem_base, __svgalib_linear_mem_size;
unsigned char* __svgalib_mmio_pointer = MAP_FAILED; /* aka MMIO_POINTER */
unsigned long int __svgalib_mmio_base, __svgalib_mmio_size;

int inrestore; /* UNUSED */

DriverSpecs* __svgalib_driverspecs;
struct vgainfo __svgalib_infotable[16];
int __svgalib_cur_mode; /* Current mode */
struct vgainfo __svgalib_cur_info; /* Current mode info */
unsigned char __svgalib_novga = 0; /* fix to 0 */
int __svgalib_chipset;
int __svgalib_mem_fd = 0; /* fix to 0 */
unsigned char __svgalib_secondary = 0; /* fix to 0 */
unsigned char __svgalib_emulatepage = 0; /* fix to 0 */
unsigned char __svgalib_ragedoubleclock = 0; /* fix to 0 */
unsigned char* __svgalib_graph_mem = (unsigned char*)0x80000000; /* fix, UNUSED */
int __svgalib_modeinfo_linearset = IS_LINEAR; /* fix */
int __svgalib_screenon = 1;

char *__svgalib_token(char **ptr) 
{
    char *p;
    p=*ptr;
    while(*p==' ')p++;
    
    if(*p != '\0' ) {
        char *t;
        t=p;
        while((*t != '\0') && (*t != ' '))t++;
        if(*t==' ') {
            *t='\0';
            t++;
        }
        *ptr=t;
        return p;
    } else {
        *ptr=NULL;
        return NULL; 
    }
}

int __svgalib_saveregs(unsigned char *regs)
{
    int i;

    if (__svgalib_chipset == EGA || __svgalib_novga) {
	/* Special case: Don't save standard VGA registers. */
	return chipset_saveregs(regs);
    }
    /* save VGA registers */
    for (i = 0; i < CRT_C; i++) {
        regs[CRT + i] = __svgalib_incrtc(i);
    }
    for (i = 0; i < ATT_C; i++) {
	regs[ATT + i] = __svgalib_inatt(i);
    }
    for (i = 0; i < GRA_C; i++) {
	regs[GRA + i] = __svgalib_ingra(i);
    }
    for (i = 0; i < SEQ_C; i++) {
	regs[SEQ + i] = __svgalib_inseq(i);
    }
    regs[MIS] = __svgalib_inmisc();

    i = chipset_saveregs(regs);	/* save chipset-specific registers */
    /* i : additional registers */
    if (!SCREENON) {		/* We turned off the screen */
        __svgalib_attscreen(0x20);
    }
    return CRT_C + ATT_C + GRA_C + SEQ_C + 1 + i;
}

int __svgalib_setregs(const unsigned char *regs)
{
    int i;

    if(__svgalib_novga) return 1;

    if (__svgalib_chipset == EGA) {
	/* Enable graphics register modification */
	port_out(0x00, GRA_E0);
	port_out(0x01, GRA_E1);
    }
    /* update misc output register */
    __svgalib_outmisc(regs[MIS]);

    /* synchronous reset on */
    __svgalib_outseq(0x00,0x01);

    /* write sequencer registers */
    if (adv_svgalib_state.divide_clock)
		__svgalib_outseq(0x01,regs[SEQ + 1] | 0x20 | 0x8);
	else
		__svgalib_outseq(0x01,regs[SEQ + 1] | 0x20);
    for (i = 2; i < SEQ_C; i++) {
       __svgalib_outseq(i,regs[SEQ + i]);
    }

    /* synchronous reset off */
    __svgalib_outseq(0x00,0x03);

    if (__svgalib_chipset != EGA) {
	/* deprotect CRT registers 0-7 */
        __svgalib_outcrtc(0x11,__svgalib_incrtc(0x11)&0x7f);
    }
    /* write CRT registers */
    for (i = 0; i < CRT_C; i++) {
        __svgalib_outcrtc(i,regs[CRT + i]);
    }

    /* write graphics controller registers */
    for (i = 0; i < GRA_C; i++) {
        __svgalib_outgra(i,regs[GRA+i]);
    }

    /* write attribute controller registers */
    for (i = 0; i < ATT_C; i++) {
        __svgalib_outatt(i,regs[ATT+i]);
    }

    return 0;
}

void __svgalib_read_options(char **commands, char *(*func) (int ind, int mode, char **nptr)) {
	/* no configuration supported */
	(void)commands;
	(void)func;
}

int vga_lastmodenumber(void) {
	return TEXT;
}

int vga_screenoff(void)
{
    int tmp = 0;

    SCREENON = 0;

    if(__svgalib_novga) return 0; 

    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->screenoff) {
	tmp = __svgalib_driverspecs->emul->screenoff();
    } else {
	/* turn off screen for faster VGA memory acces */
	if (CHIPSET != EGA) {
            __svgalib_outseq(0x01,__svgalib_inseq(0x01) | 0x20);
	}
	/* Disable video output */
#ifdef DISABLE_VIDEO_OUTPUT
	__svgalib_attscreen(0);
#endif
    }

    return tmp;
}

int vga_screenon(void)
{
    int tmp = 0;

    SCREENON = 1;
    if(__svgalib_novga) return 0; 
    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->screenon) {
	tmp = __svgalib_driverspecs->emul->screenon();
    } else {
	/* turn screen back on */
	if (CHIPSET != EGA) {
            __svgalib_outseq(0x01,__svgalib_inseq(0x01) & 0xdf);
	}
/* #ifdef DISABLE_VIDEO_OUTPUT */
	/* enable video output */
	__svgalib_attscreen(0x20);
/* #endif */
    }

    return 0;
}

int vga_getxdim(void) {
	return __svgalib_cur_info.xdim;
}

int vga_getydim(void) {
	return __svgalib_cur_info.ydim;
}

int vga_getcolors(void) {
	return __svgalib_cur_info.colors;
}

int vga_getcurrentmode(void) {
	return __svgalib_cur_mode;
}

vga_modeinfo *vga_getmodeinfo(int mode)
{
	static vga_modeinfo modeinfo;
	memset(&modeinfo,0,sizeof(modeinfo));

	modeinfo.linewidth = infotable[mode].xbytes;
	modeinfo.width = infotable[mode].xdim;
	modeinfo.height = infotable[mode].ydim;
	modeinfo.bytesperpixel = infotable[mode].bytesperpixel;
	modeinfo.colors = infotable[mode].colors;

	if (mode == TEXT) {
		modeinfo.flags = HAVE_EXT_SET;
		return &modeinfo;
	}

	modeinfo.flags = 0;

	chipset_getmodeinfo(mode, &modeinfo);

	/* If all needed info is here, signal if linear support has been enabled */
	if ((modeinfo.flags & (CAPABLE_LINEAR | EXT_INFO_AVAILABLE)) ==
		(CAPABLE_LINEAR | EXT_INFO_AVAILABLE)) {
		modeinfo.flags |= __svgalib_modeinfo_linearset;
	}

	return &modeinfo;
}

void __svgalib_emul_setpage(int page) {
	/* used only for banked modes */
}

void map_mmio(void) {
	if (MMIO_POINTER != MAP_FAILED)
		return;

	if (!__svgalib_mmio_size)
		return;

	MMIO_POINTER = adv_svgalib_mmap(0, __svgalib_mmio_size, PROT_READ | PROT_WRITE, MAP_SHARED, __svgalib_mem_fd, __svgalib_mmio_base);
}

void unmap_mmio(void)
{
	if (MMIO_POINTER == MAP_FAILED)
		return;

	adv_svgalib_munmap(MMIO_POINTER, __svgalib_mmio_size);

	MMIO_POINTER = MAP_FAILED;
}

void map_linear(unsigned long base, unsigned long size) {
	if (LINEAR_POINTER != MAP_FAILED)
		return;
		
	LINEAR_POINTER = adv_svgalib_mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, __svgalib_mem_fd, base);
}

void unmap_linear(unsigned long size) {
	if (LINEAR_POINTER == MAP_FAILED)
		return;

	adv_svgalib_munmap(LINEAR_POINTER, size);

	LINEAR_POINTER = MAP_FAILED;
}

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

/***************************************************************************/
/* vgamisc */

void vga_waitretrace(void)
{
	if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->waitretrace) {
		__svgalib_driverspecs->emul->waitretrace();
	} else {
		while ((__svgalib_inis1() & 8)==0)
			;
		while ((__svgalib_inis1() & 8)!=0)
			;
	}
}

/***************************************************************************/
/* vgadraw */

int vga_drawscansegment(unsigned char *colors, int x, int y, int length) {
	(void)colors;
	(void)x;
	(void)y;
	(void)length;
	/* used only for cursor */
	adv_svgalib_log("svgalib: invalid vga_drawscansegment() call\n");
	adv_svgalib_abort();
	return 0;
}

/***************************************************************************/
/* vgapal */

int vga_getpalvec(int start, int num, int *pal) {
	(void)start;
	(void)num;
	(void)pal;
	/* used only for cursor */
	adv_svgalib_log("svgalib: invalid vga_getpalvec() call\n");
	adv_svgalib_abort();
	return num;
}

int vga_setpalette(int index, int red, int green, int blue) {
	/* correct wrong RGB values */
	red &= 0x3F;
	green &= 0x3F;
	blue &= 0x3F;

	if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->setpalette) {
		return __svgalib_driverspecs->emul->setpalette(index, red, green, blue);
	} else {
		__svgalib_outpal(index,red,green,blue);
	}
	return 0;
}

/***************************************************************************/
/* vgadrv */

int vgadrv_saveregs(unsigned char regs[]) {
	(void)regs;
	adv_svgalib_log("svgalib: invalid vgadrv_saveregs() call\n");
	adv_svgalib_abort();
	return 0;
}

void vgadrv_setregs(const unsigned char regs[], int mode) {
	(void)regs;
	(void)mode;
	adv_svgalib_log("svgalib: invalid vgadrv_setregs() call\n");
	adv_svgalib_abort();
}

void vgadrv_unlock(void) {
	adv_svgalib_log("svgalib: invalid vgadrv_unlock() call\n");
	adv_svgalib_abort();
}

void vgadrv_lock(void) {
	adv_svgalib_log("svgalib: invalid vgadrv_lock() call\n");
	adv_svgalib_abort();
}

int vgadrv_test(void) {
	adv_svgalib_log("svgalib: invalid vgadrv_test() call\n");
	adv_svgalib_abort();
	return 0;
}

int vgadrv_init(int force, int par1, int par2) {
	(void)force;
	(void)par1;
	(void)par2;
	adv_svgalib_log("svgalib: invalid vgadrv_init() call\n");
	adv_svgalib_abort();
	return 0;
}

void vgadrv_setpage(int page) {
	(void)page;
	adv_svgalib_log("svgalib: invalid vgadrv_setpage() call\n");
	adv_svgalib_abort();
}

void vgadrv_setrdpage(int page) {
	(void)page;
	adv_svgalib_log("svgalib: invalid vgadrv_setrdpage() call\n");
	adv_svgalib_abort();
}

void vgadrv_setwrpage(int page) {
	(void)page;
	adv_svgalib_log("svgalib: invalid vgadrv_setwrpage() call\n");
	adv_svgalib_abort();
}

int vgadrv_setmode(int mode, int prv_mode) {
	(void)mode;
	(void)prv_mode;
	adv_svgalib_log("svgalib: invalid vgadrv_setmode() call\n");
	adv_svgalib_abort();
	return 0;
}

int vgadrv_modeavailable(int mode) {
	(void)mode;
	adv_svgalib_log("svgalib: invalid vgadrv_modeavailable() call\n");
	adv_svgalib_abort();
	return 0;
}

void vgadrv_setdisplaystart(int address) {
	(void)address;
	adv_svgalib_log("svgalib: invalid vgadrv_setdisplaystart() call\n");
	adv_svgalib_abort();
}

void vgadrv_setlogicalwidth(int width) {
	(void)width;
	adv_svgalib_log("svgalib: invalid vgadrv_setlogicalwidth() call\n");
	adv_svgalib_abort();
}

void vgadrv_getmodeinfo(int mode, vga_modeinfo * modeinfo) {
	(void)mode;
	(void)modeinfo;
	adv_svgalib_abort();
}

DriverSpecs __svgalib_vga_driverspecs = {
	vgadrv_saveregs,
	vgadrv_setregs,
	vgadrv_unlock,
	vgadrv_lock,
	vgadrv_test,
	vgadrv_init,
	vgadrv_setpage,
	vgadrv_setrdpage,
	vgadrv_setwrpage,
	vgadrv_setmode,
	vgadrv_modeavailable,
	vgadrv_setdisplaystart,
	vgadrv_setlogicalwidth,
	vgadrv_getmodeinfo,
	0, /* bitblt */
	0, /* imageblt */
	0, /* fillblt */
	0, /* hlinelistblt */
	0, /* bltwait */
	0, /* extset */
	0, /* accel */
	0, /* linear */
	NULL, /* accelspecs */
	NULL, /* emulation */
	0 /* cursor */
};

/***************************************************************************/
/* interface */

void __svgalib_clear_accelspecs(AccelSpecs * accelspecs)
{
    memset(accelspecs, 0, sizeof(AccelSpecs));
}

ModeInfo* __svgalib_createModeInfoStructureForSvgalibMode(int mode)
{
	ModeInfo* modeinfo;

	if (mode != adv_svgalib_state.mode_number)
		return 0;

	modeinfo = adv_svgalib_malloc(sizeof(ModeInfo));
	
	memset(modeinfo,0,sizeof(ModeInfo));

	modeinfo->width = adv_svgalib_state.mode.width;
	modeinfo->height = adv_svgalib_state.mode.height;
	modeinfo->bytesPerPixel = adv_svgalib_state.mode.bytes_per_pixel;

	switch (adv_svgalib_state.mode.bits_per_pixel) {
		case 8 :
			modeinfo->colorBits = 8;
			modeinfo->bitsPerPixel = 8;
			break;
		case 15 :
			modeinfo->colorBits = 15;
			modeinfo->bitsPerPixel = 16;
			break;
		case 16 :
			modeinfo->colorBits = 16;
			modeinfo->bitsPerPixel = 16;
			break;
		case 24 :
			modeinfo->colorBits = 24;
			modeinfo->bitsPerPixel = 24;
			break;
		case 32 :
			modeinfo->colorBits = 24;
			modeinfo->bitsPerPixel = 32;
			break;
	}

	modeinfo->redWeight = adv_svgalib_state.mode.red_len;
	modeinfo->greenWeight = adv_svgalib_state.mode.green_len;
	modeinfo->blueWeight = adv_svgalib_state.mode.blue_len;
	modeinfo->redOffset = adv_svgalib_state.mode.red_pos;
	modeinfo->greenOffset = adv_svgalib_state.mode.green_pos;
	modeinfo->blueOffset = adv_svgalib_state.mode.blue_pos;
	modeinfo->redMask = 0; /* TODO */
	modeinfo->blueMask = 0; /* TODO */
	modeinfo->greenMask = 0; /* TODO */

	modeinfo->lineWidth = adv_svgalib_state.mode.bytes_per_scanline;
	modeinfo->realWidth = adv_svgalib_state.mode.width;
	modeinfo->realHeight = adv_svgalib_state.mode.height;
	modeinfo->flags = 0; /* TODO */

	return modeinfo;
}

/*
 * This function converts a number of significant color bits to a matching
 * DAC mode type as defined in the RAMDAC interface.
 */
int __svgalib_colorbits_to_colormode(int bpp, int colorbits)
{
	if (colorbits == 8)
		return CLUT8_6;
	if (colorbits == 15)
		return RGB16_555;
	if (colorbits == 16)
		return RGB16_565;
	if (colorbits == 24) {
		if (bpp == 24)
			return RGB24_888_B;
		else
			return RGB32_888_B;
	}
	return CLUT8_6;
}

/*
 * findclock is an auxilliary function that checks if a close enough
 * pixel clock is provided by the card. Returns clock number if
 * succesful (a special number if a programmable clock must be used), -1
 * otherwise.
 */

/*
 * Clock allowance in 1/1000ths. 10 (1%) corresponds to a 250 kHz
 * deviation at 25 MHz, 1 MHz at 100 MHz
 */
#define CLOCK_ALLOWANCE 10

#define PROGRAMMABLE_CLOCK_MAGIC_NUMBER 0x1234

static int findclock(int clock, CardSpecs * cardspecs)
{
    int i;
    /* Find a clock that is close enough. */
    for (i = 0; i < cardspecs->nClocks; i++) {
	int diff;
	diff = cardspecs->clocks[i] - clock;
	if (diff < 0)
	    diff = -diff;
	if (diff * 1000 / clock < CLOCK_ALLOWANCE)
	    return i;
    }
    /* Try programmable clocks if available. */
    if (cardspecs->flags & CLOCK_PROGRAMMABLE) {
	int diff;
	diff = cardspecs->matchProgrammableClock(clock) - clock;
	if (diff < 0)
	    diff = -diff;
	if (diff * 1000 / clock < CLOCK_ALLOWANCE)
	    return PROGRAMMABLE_CLOCK_MAGIC_NUMBER;
    }
    /* No close enough clock found. */
    return -1;
}

int __svgalib_getmodetiming(ModeTiming* modetiming, ModeInfo* modeinfo, CardSpecs* cardspecs)
{
	int maxclock, desiredclock;

	/* Get the maximum pixel clock for the depth of the requested mode. */
	if (modeinfo->bitsPerPixel == 4)
		maxclock = cardspecs->maxPixelClock4bpp;
	else if (modeinfo->bitsPerPixel == 8)
		maxclock = cardspecs->maxPixelClock8bpp;
	else if (modeinfo->bitsPerPixel == 16) {
		/* rgb 16 bit mode check */
		if ((cardspecs->flags & NO_RGB16_565)!=0 && modeinfo->greenWeight==6)
			return 1;
		maxclock = cardspecs->maxPixelClock16bpp;
	} else if (modeinfo->bitsPerPixel == 24)
		maxclock = cardspecs->maxPixelClock24bpp;
	else if (modeinfo->bitsPerPixel == 32)
		maxclock = cardspecs->maxPixelClock32bpp;
	else
		return 1;

	/* clock check */
	if (adv_svgalib_state.crtc.pixelclock / 1000 > maxclock)
		return 1;

	/* interlace check */
	if ((cardspecs->flags & NO_INTERLACE)!=0 && adv_svgalib_state.crtc.interlace)
		return 1;

	/*
	 * Copy the selected timings into the result, which may
	 * be adjusted for the chipset.
	 */
	modetiming->flags = 0;
	if (adv_svgalib_state.crtc.doublescan)
		modetiming->flags |= DOUBLESCAN;
	if (adv_svgalib_state.crtc.interlace)
		modetiming->flags |= INTERLACED;
	if (adv_svgalib_state.crtc.nhsync)
		modetiming->flags |= NHSYNC;
	else
		modetiming->flags |= PHSYNC;
	if (adv_svgalib_state.crtc.nvsync)
		modetiming->flags |= NVSYNC;
	else
		modetiming->flags |= PVSYNC;
	if (adv_svgalib_state.mode_tvpal)
		modetiming->flags |= TVMODE | TVPAL;
	if (adv_svgalib_state.mode_tvntsc)
		modetiming->flags |= TVMODE | TVNTSC;

	modetiming->pixelClock = adv_svgalib_state.crtc.pixelclock / 1000;

	/*
	 * We know a close enough clock is available; the following is the
	 * exact clock that fits the mode. This is probably different
	 * from the best matching clock that will be programmed.
	 */
	desiredclock = cardspecs->mapClock(modeinfo->bitsPerPixel,adv_svgalib_state.crtc.pixelclock / 1000);

	/* Fill in the best-matching clock that will be programmed. */
	modetiming->selectedClockNo = findclock(desiredclock, cardspecs);
	if (modetiming->selectedClockNo < 0)
		return 1;
	if (modetiming->selectedClockNo == PROGRAMMABLE_CLOCK_MAGIC_NUMBER) {
		modetiming->programmedClock = cardspecs->matchProgrammableClock(desiredclock);
		if (!modetiming->programmedClock)
			return 1;
		modetiming->flags |= USEPROGRCLOCK;
	} else
		modetiming->programmedClock = cardspecs->clocks[modetiming->selectedClockNo];

	modetiming->HDisplay = adv_svgalib_state.crtc.hde;
	modetiming->HSyncStart = adv_svgalib_state.crtc.hrs;
	modetiming->HSyncEnd = adv_svgalib_state.crtc.hre;
	modetiming->HTotal = adv_svgalib_state.crtc.ht;
	if (cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel, modetiming->programmedClock, adv_svgalib_state.crtc.ht) != adv_svgalib_state.crtc.ht) {
		/* Horizontal CRTC timings are scaled in some way. */
		modetiming->CrtcHDisplay = cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel, modetiming->programmedClock, adv_svgalib_state.crtc.hde);
		modetiming->CrtcHSyncStart = cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel, modetiming->programmedClock, adv_svgalib_state.crtc.hrs);
		modetiming->CrtcHSyncEnd = cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel, modetiming->programmedClock, adv_svgalib_state.crtc.hre);
		modetiming->CrtcHTotal = cardspecs->mapHorizontalCrtc(modeinfo->bitsPerPixel, modetiming->programmedClock, adv_svgalib_state.crtc.ht);
		modetiming->flags |= HADJUSTED;
	} else {
		modetiming->CrtcHDisplay = adv_svgalib_state.crtc.hde;
		modetiming->CrtcHSyncStart = adv_svgalib_state.crtc.hrs;
		modetiming->CrtcHSyncEnd = adv_svgalib_state.crtc.hre;
		modetiming->CrtcHTotal = adv_svgalib_state.crtc.ht;
	}
	modetiming->VDisplay = adv_svgalib_state.crtc.vde;
	modetiming->VSyncStart = adv_svgalib_state.crtc.vrs;
	modetiming->VSyncEnd = adv_svgalib_state.crtc.vre;
	modetiming->VTotal = adv_svgalib_state.crtc.vt;
	if (modetiming->flags & DOUBLESCAN){
		modetiming->VDisplay <<= 1;
		modetiming->VSyncStart <<= 1;
		modetiming->VSyncEnd <<= 1;
		modetiming->VTotal <<= 1;
	}
	modetiming->CrtcVDisplay = modetiming->VDisplay;
	modetiming->CrtcVSyncStart = modetiming->VSyncStart;
	modetiming->CrtcVSyncEnd = modetiming->VSyncEnd;
	modetiming->CrtcVTotal = modetiming->VTotal;
	if (((modetiming->flags & INTERLACED) && (cardspecs->flags & INTERLACE_DIVIDE_VERT))
		|| (modetiming->VTotal >= 1024 && (cardspecs->flags & GREATER_1024_DIVIDE_VERT))) {
		/*
		 * Card requires vertical CRTC timing to be halved for
		 * interlaced modes, or for all modes with vertical
		 * timing >= 1024.
		 */
		modetiming->CrtcVDisplay /= 2;
		modetiming->CrtcVSyncStart /= 2;
		modetiming->CrtcVSyncEnd /= 2;
		modetiming->CrtcVTotal /= 2;
		modetiming->flags |= VADJUSTED;
	}

	return 0;
}

/***************************************************************************/
/* adv_svgalib */

/* Short names for the most common flags */
#define FLAGS_NONE 0
#define FLAGS_INTERLACE 1
#define FLAGS_TV 2

static struct adv_svgalib_chipset_struct cards[] = {
#ifdef INCLUDE_NV3_DRIVER
	{ &__svgalib_nv3_driverspecs, NV3, "nv3", FLAGS_INTERLACE | FLAGS_TV },
#endif
#ifdef INCLUDE_NV3_DRIVER
	{ &__svgalib_nv3_19_driverspecs, NV3, "nv3_leg", FLAGS_INTERLACE | FLAGS_TV },
#endif
#ifdef INCLUDE_TRIDENT_DRIVER
	{ &__svgalib_trident_driverspecs, TRIDENT, "trident", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_RENDITION_DRIVER
	{ &__svgalib_rendition_driverspecs, RENDITION, "rendition", FLAGS_NONE },
#endif
#ifdef INCLUDE_G400_DRIVER
	{ &__svgalib_g400_driverspecs, G400, "g400", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_PM2_DRIVER
	{ &__svgalib_pm2_driverspecs, PM2, "pm2", FLAGS_NONE },
#endif
#ifdef INCLUDE_UNICHROME_DRIVER
	{ &__svgalib_unichrome_driverspecs, UNICHROME, "unichrome", FLAGS_NONE },
#endif
#ifdef INCLUDE_SAVAGE_DRIVER
	{ &__svgalib_savage_driverspecs, SAVAGE, "savage", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_SAVAGE_DRIVER
	{ &__svgalib_savage_18_driverspecs, SAVAGE, "savage_leg", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_MILLENNIUM_DRIVER
	{ &__svgalib_mil_driverspecs, MILLENNIUM, "millenium", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_R128_DRIVER
	{ &__svgalib_r128_driverspecs, R128, "r128", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_BANSHEE_DRIVER
	{ &__svgalib_banshee_driverspecs, BANSHEE, "banshee", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_SIS_DRIVER
	{ &__svgalib_sis_driverspecs, SIS, "sis", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_LAGUNA_DRIVER
	{ &__svgalib_laguna_driverspecs, LAGUNA, "laguna", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_RAGE_DRIVER
	{ &__svgalib_rage_driverspecs, RAGE, "rage", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_MX_DRIVER
	{ &__svgalib_mx_driverspecs, MX, "mx", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_ET6000_DRIVER
	{ &__svgalib_et6000_driverspecs, ET6000, "et6000", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_S3_DRIVER
	{ &__svgalib_s3_driverspecs, S3, "s3", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_ARK_DRIVER
	{ &__svgalib_ark_driverspecs, ARK, "ark", FLAGS_INTERLACE },
#endif
#ifdef INCLUDE_APM_DRIVER
	/* The driver doesn't check the INTERLACED flags */
	/* On certain cards this may toggle the video signal on/off which is ugly. Hence we test this last. */
	{ &__svgalib_apm_driverspecs, APM, "apm", FLAGS_NONE },
#endif
	{ 0, 0, 0, 0 }
};

static void mode_init(unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, int doublescan, int interlace, int hsync, int vsync, unsigned bits_per_pixel, int tvpal, int tvntsc)
{
	/* mode */
	memset(&adv_svgalib_state.mode,0,sizeof(adv_svgalib_state.mode));
	adv_svgalib_state.mode.width = hde;
	adv_svgalib_state.mode.height = vde;
	adv_svgalib_state.mode.bits_per_pixel = bits_per_pixel;
	adv_svgalib_state.mode.bytes_per_pixel = (adv_svgalib_state.mode.bits_per_pixel + 7) / 8;
	adv_svgalib_state.mode.bytes_per_scanline = (adv_svgalib_state.mode.bytes_per_pixel * adv_svgalib_state.mode.width + 0x3) & ~0x3;

	switch (bits_per_pixel) {
		case 15 :
			adv_svgalib_state.mode.red_len = 5;
			adv_svgalib_state.mode.red_pos = 10;
			adv_svgalib_state.mode.green_len = 5;
			adv_svgalib_state.mode.green_pos = 5;
			adv_svgalib_state.mode.blue_len = 5;
			adv_svgalib_state.mode.blue_pos = 0;
			break;
		case 16 :
			adv_svgalib_state.mode.red_len = 5;
			adv_svgalib_state.mode.red_pos = 11;
			adv_svgalib_state.mode.green_len = 6;
			adv_svgalib_state.mode.green_pos = 5;
			adv_svgalib_state.mode.blue_len = 5;
			adv_svgalib_state.mode.blue_pos = 0;
			break;
		case 24 :
		case 32 :
			adv_svgalib_state.mode.red_len = 8;
			adv_svgalib_state.mode.red_pos = 16;
			adv_svgalib_state.mode.green_len = 8;
			adv_svgalib_state.mode.green_pos = 8;
			adv_svgalib_state.mode.blue_len = 8;
			adv_svgalib_state.mode.blue_pos = 0;
			break;
	}

	/* mode */
	adv_svgalib_state.mode_tvpal = tvpal;
	adv_svgalib_state.mode_tvntsc = tvntsc;

	/* crtc */
	adv_svgalib_state.crtc.hde = hde;
	adv_svgalib_state.crtc.hrs = hrs;
	adv_svgalib_state.crtc.hre = hre;
	adv_svgalib_state.crtc.ht = ht;

	adv_svgalib_state.crtc.vde = vde;
	adv_svgalib_state.crtc.vrs = vrs;
	adv_svgalib_state.crtc.vre = vre;
	adv_svgalib_state.crtc.vt = vt;

	/* the SVGALIB interface already divide and double the vertical value for doublescan and interlace */

	adv_svgalib_state.crtc.nhsync = hsync;
	adv_svgalib_state.crtc.nvsync = vsync;
	adv_svgalib_state.crtc.doublescan = doublescan;
	adv_svgalib_state.crtc.interlace = interlace;

	adv_svgalib_state.crtc.pixelclock = pixelclock;
	adv_svgalib_state.mode_number = 15;

	__svgalib_infotable[adv_svgalib_state.mode_number].xdim = adv_svgalib_state.mode.width;
	__svgalib_infotable[adv_svgalib_state.mode_number].ydim = adv_svgalib_state.mode.height;
	__svgalib_infotable[adv_svgalib_state.mode_number].colors = adv_svgalib_state.mode.bits_per_pixel;
	__svgalib_infotable[adv_svgalib_state.mode_number].xbytes = adv_svgalib_state.mode.bytes_per_scanline;
	__svgalib_infotable[adv_svgalib_state.mode_number].bytesperpixel = adv_svgalib_state.mode.bytes_per_pixel;

	__svgalib_cur_mode = adv_svgalib_state.mode_number;
	__svgalib_cur_info = __svgalib_infotable[__svgalib_cur_mode];
}

static void mode_done(void)
{
	__svgalib_cur_mode = 0;
	__svgalib_cur_info = __svgalib_infotable[__svgalib_cur_mode];
}

/**
 * Initialize the svgalib library.
 * \param divide_clock_with_sequencer Use the VGA sequencer to middle the clock. It doesn't work very well. Set it to 0.
 * \return
 *  - ==0 on success
 *  - !=0 on error
 */
int ADV_SVGALIB_CALL adv_svgalib_init(int divide_clock_with_sequencer, int skip_board)
{
	adv_svgalib_log("svgalib: adv_svgalib_init()\n");

	memset(&adv_svgalib_state, 0, sizeof(adv_svgalib_state));

	if (adv_svgalib_open() != 0)
		return -1;

#ifdef ADV_SVGALIB_INTERNAL_HEAP
	heap_init();
#endif

	adv_svgalib_state.divide_clock = divide_clock_with_sequencer;
	adv_svgalib_state.skip_board = skip_board;
	adv_svgalib_state.last_bus_device_func = 0;

	__svgalib_chipset = UNDEFINED;
	__svgalib_driverspecs = &__svgalib_vga_driverspecs;
	__svgalib_inmisc = __svgalib_vga_inmisc;
	__svgalib_outmisc = __svgalib_vga_outmisc;
	__svgalib_incrtc = __svgalib_vga_incrtc;
	__svgalib_outcrtc = __svgalib_vga_outcrtc;
	__svgalib_inseq = __svgalib_vga_inseq;
	__svgalib_outseq = __svgalib_vga_outseq;
	__svgalib_ingra = __svgalib_vga_ingra;
	__svgalib_outgra = __svgalib_vga_outgra;
	__svgalib_inatt = __svgalib_vga_inatt;
	__svgalib_outatt = __svgalib_vga_outatt;
	__svgalib_attscreen = __svgalib_vga_attscreen;
	__svgalib_inpal = __svgalib_vga_inpal;
	__svgalib_outpal = __svgalib_vga_outpal;
	__svgalib_inis1 = __svgalib_vga_inis1;

	return 0;
}

/**
 * Deinitialize the SVGALIB library.
 * You must call this function if adv_svgalib_init() complete with success.
 */
void ADV_SVGALIB_CALL adv_svgalib_done(void)
{
	adv_svgalib_log("svgalib: adv_svgalib_done()\n");

	unmap_mmio();

	adv_svgalib_close();
}

/**
 * Detect the SVGALIB driver.
 * You must call this function immediatly after adv_svgalib_init().
 * \param name Name of the video driver, or "auto" for an anutomatic detection.
 * \return
 *  - ==0 on success
 *  - !=0 on error
 */
int ADV_SVGALIB_CALL adv_svgalib_detect(const char* name) {
	unsigned bit_map[5] = { 8,15,16,24,32 };
	unsigned i;

	adv_svgalib_log("svgalib: adv_svgalib_detect()\n");

	adv_svgalib_state.driver = 0;
	for(i=0;cards[i].name;++i) {
		if (strcmp(name,"auto")==0 || strcmp(name,cards[i].name)==0) {
			adv_svgalib_log("svgalib: testing for driver %s\n", cards[i].name);
			if (cards[i].drv->test()) {
				adv_svgalib_state.driver = &cards[i];
				__svgalib_driverspecs = adv_svgalib_state.driver->drv;
				__svgalib_chipset = adv_svgalib_state.driver->chipset;
				break;
			}
		}
	}

	if (adv_svgalib_state.driver == 0) {
		adv_svgalib_log("svgalib: no driver found\n");
		return -1;
	}

	if (__svgalib_linear_mem_size == 0) {
		adv_svgalib_log("svgalib: invalid linear size\n");
		return -1;
	}

	if (__svgalib_linear_mem_base == 0) {
		adv_svgalib_log("svgalib: invalid linear base\n");
		return -1;
	}

	if (!adv_svgalib_state.driver->drv->saveregs
		|| !adv_svgalib_state.driver->drv->setregs
		|| !adv_svgalib_state.driver->drv->test
		|| !adv_svgalib_state.driver->drv->init
		|| !adv_svgalib_state.driver->drv->setmode
		|| !adv_svgalib_state.driver->drv->modeavailable
		|| !adv_svgalib_state.driver->drv->linear) {
		adv_svgalib_log("svgalib: invalid driver function table\n");
		return -1;
	}

	adv_svgalib_state.has_bit8 = 1;
	adv_svgalib_state.has_bit15 = 1;
	adv_svgalib_state.has_bit16 = 1;
	adv_svgalib_state.has_bit24 = 1;
	adv_svgalib_state.has_bit32 = 1;

	/* test the bit depths */
	for(i=0;i<5;++i) {
		unsigned bit = bit_map[i];

		mode_init(25200000/2,640/2,656/2,752/2,800/2,480,490,492,525,0,0,1,1,bit,0,0);

		if (adv_svgalib_state.driver->drv->modeavailable(adv_svgalib_state.mode_number) == 0) {
			switch (bit) {
				case 8 : adv_svgalib_state.has_bit8 = 0; break;
				case 15 : adv_svgalib_state.has_bit15 = 0; break;
				case 16 : adv_svgalib_state.has_bit16 = 0; break;
				case 24 : adv_svgalib_state.has_bit24 = 0; break;
				case 32 : adv_svgalib_state.has_bit32 = 0; break;
			}
		}

		mode_done();
	}

	if (adv_svgalib_state.has_bit8 == 0 && adv_svgalib_state.has_bit15 == 0 && adv_svgalib_state.has_bit16 == 0 && adv_svgalib_state.has_bit24 == 0 && adv_svgalib_state.has_bit32 == 0) {
		adv_svgalib_log("svgalib: invalid color depth\n");
		return -1;
	}

	/* test interlace */
	adv_svgalib_state.has_interlace = 0;
	if ((adv_svgalib_state.driver->cap & FLAGS_INTERLACE) != 0) {
		adv_svgalib_state.has_interlace = 1;
		mode_init(40280300,1024,1048,1200,1280,768,784,787,840,0,1,1,1,8,0,0);

		if (adv_svgalib_state.driver->drv->modeavailable(adv_svgalib_state.mode_number) == 0) {
			adv_svgalib_state.has_interlace = 0;
		}

		mode_done();
	}

	if ((adv_svgalib_state.driver->cap & FLAGS_TV) != 0) {
		adv_svgalib_state.has_tvntsc = 1;
		adv_svgalib_state.has_tvpal = 1;
	} else {
		adv_svgalib_state.has_tvntsc = 0;
		adv_svgalib_state.has_tvpal = 0;
	}

	return 0;
}

/**
 * Check a video mode.
 * \return
 *  - ==0 on success
 *  - !=0 on error
 */
int ADV_SVGALIB_CALL adv_svgalib_check(unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, int doublescan, int interlace, int hsync, int vsync, unsigned bits_per_pixel, int tvpal, int tvntsc)
{
	if (!(pixelclock>0
		&& hde <= hrs && hrs < hre && hre <= ht
		&& vde <= vrs && vrs < vre && vre <= vt))
		return -1;

	mode_init(pixelclock, hde, hrs, hre, ht, vde, vrs, vre, vt, doublescan, interlace, hsync, vsync, bits_per_pixel, tvpal, tvntsc);

	if (adv_svgalib_state.driver->drv->modeavailable(adv_svgalib_state.mode_number) == 0) {
		mode_done();
		return -1;
	}

	mode_done();

	return 0;
}

/**
 * Set a video mode.
 * \return
 *  - ==0 on success
 *  - !=0 on error
 */
int ADV_SVGALIB_CALL adv_svgalib_set(unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, int doublescan, int interlace, int hsync, int vsync, unsigned bits_per_pixel, int tvpal, int tvntsc)
{
	adv_svgalib_log("svgalib: adv_svgalib_set()\n");

	mode_init(pixelclock, hde, hrs, hre, ht, vde, vrs, vre, vt, doublescan, interlace, hsync, vsync, bits_per_pixel, tvpal, tvntsc);

	if (adv_svgalib_state.driver->drv->unlock)
		adv_svgalib_state.driver->drv->unlock();

#ifdef NDEBUG
	adv_svgalib_disable();
	vga_screenoff();
#endif

	if (adv_svgalib_state.driver->drv->setmode(adv_svgalib_state.mode_number, TEXT)) {
		adv_svgalib_log("svgalib: setmode() failed\n");
		adv_svgalib_enable();
		vga_screenon();
		mode_done();
		return -1;
	}
	adv_svgalib_enable();

	adv_svgalib_usleep(10000);

	vga_screenon();

	if (adv_svgalib_state.driver->drv->linear(LINEAR_ENABLE, __svgalib_linear_mem_base)!=0) {
		adv_svgalib_log("svgalib: linear() failed\n");
		mode_done();
		return -1;
	}

	return 0;
}

/**
 * Unset a video mode.
 * You must call this function if adv_svgalib_set() complete with success.
 * \return
 *  - ==0 on success
 *  - !=0 on error
 */
void ADV_SVGALIB_CALL adv_svgalib_unset(void) 
{
	adv_svgalib_log("svgalib: adv_svgalib_unset()\n");

	if (adv_svgalib_state.driver->drv->unlock)
		adv_svgalib_state.driver->drv->unlock();

	adv_svgalib_state.driver->drv->linear(LINEAR_DISABLE, __svgalib_linear_mem_base);

	mode_done();
}

/**
 * Save the video board state.
 * \param regs Destination of the state. It must be ADV_SVGALIB_STATE_SIZE bytes long.
 */
void ADV_SVGALIB_CALL adv_svgalib_save(unsigned char* regs) 
{
	adv_svgalib_log("svgalib: adv_svgalib_save()\n");

	memset(regs, ADV_SVGALIB_STATE_SIZE, 0);

	if (adv_svgalib_state.driver->drv->unlock)
		adv_svgalib_state.driver->drv->unlock();

	adv_svgalib_disable();
	__svgalib_saveregs(regs);
	adv_svgalib_enable();
}

/**
 * Restore the video board state.
 * \param regs Source of the state. It must be previous obtained by adv_svgalib_save.
 */
void ADV_SVGALIB_CALL adv_svgalib_restore(unsigned char* regs) 
{
	adv_svgalib_log("svgalib: adv_svgalib_restore()\n");

	if (adv_svgalib_state.driver->drv->unlock)
		adv_svgalib_state.driver->drv->unlock();

#ifdef NDEBUG
	adv_svgalib_disable();
	vga_screenoff();
#endif
	__svgalib_setregs(regs);
	adv_svgalib_state.driver->drv->setregs(regs, TEXT);
	adv_svgalib_enable();

	adv_svgalib_usleep(10000);

	vga_screenon();
}

/**
 * Map the video board linear memory.
 */
void ADV_SVGALIB_CALL adv_svgalib_linear_map(void) 
{
	adv_svgalib_log("svgalib: adv_svgalib_linear_map()\n");

	map_linear(__svgalib_linear_mem_base, __svgalib_linear_mem_size);
}

/**
 * Unmap the video board linear memory.
 */
void ADV_SVGALIB_CALL adv_svgalib_linear_unmap(void) 
{
	adv_svgalib_log("svgalib: adv_svgalib_linear_unmap()\n");
	
	unmap_linear(__svgalib_linear_mem_size);
}

/**
 * Set the display start offset in the video memory.
 * \param offset Display start offset in bytes.
 */
void ADV_SVGALIB_CALL adv_svgalib_scroll_set(unsigned offset) 
{
	if (adv_svgalib_state.driver->drv->setdisplaystart)
		adv_svgalib_state.driver->drv->setdisplaystart(offset);
}

/**
 * Set the scanline length.
 * \param offset Scanline length in bytes.
 */
void ADV_SVGALIB_CALL adv_svgalib_scanline_set(unsigned byte_length) {
	adv_svgalib_state.mode.bytes_per_scanline = byte_length;

	if (adv_svgalib_state.driver->drv->setlogicalwidth)
		adv_svgalib_state.driver->drv->setlogicalwidth(byte_length);
}

/**
 * Set a palette entry.
 * \param index Index of the palette entry. From 0 to 255.
 * \param r,g,b RGB values of 8 bits.
 */
void ADV_SVGALIB_CALL adv_svgalib_palette_set(unsigned index, unsigned r, unsigned g, unsigned b) {
	vga_setpalette(index, r >> 2, g >> 2, b >> 2);
}

/**
 * Wait a vertical retrace.
 */
void ADV_SVGALIB_CALL adv_svgalib_wait_vsync(void) {
	vga_waitretrace();
}

/**
 * Enable the hardware video output.
 */
void ADV_SVGALIB_CALL adv_svgalib_on(void) {
	vga_screenon();
}

/**
 * Disable the hardware video output.
 */
void ADV_SVGALIB_CALL adv_svgalib_off(void) {
	vga_screenoff();
}

/**
 * Enable the hardware mouse pointer.
 */
void ADV_SVGALIB_CALL adv_svgalib_cursor_on(void) {
	if (adv_svgalib_state.driver->drv->cursor)
		adv_svgalib_state.driver->drv->cursor(CURSOR_SHOW, 0, 0, 0, 0, 0);
}

/**
 * Disable the hardware mosue pointer.
 */
void ADV_SVGALIB_CALL adv_svgalib_cursor_off(void) {
	if (adv_svgalib_state.driver->drv->cursor)
		adv_svgalib_state.driver->drv->cursor(CURSOR_HIDE, 0, 0, 0, 0, 0);
}

void ADV_SVGALIB_CALL_VARARGS adv_svgalib_log(const char *text, ...) {
	va_list arg;
	va_start(arg, text);
	adv_svgalib_log_va(text, arg);
	va_end(arg);
}


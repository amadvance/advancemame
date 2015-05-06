/*
 * This file is part of the Advance project
 *
 * Copyright (C) 2002, 2003 Shigeaki Sakamaki
 * Copyright (C) 2002 Andrea Mazzoleni
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

/*
 * This file is an adaption of the VSyncMAME audio drivers written by
 * Shigeaki Sakamaki. It also contains some snipsets from the projects
 * Allegro (Allegro License) and ALSA (GPL/LGPL License) which have a
 * GPL compatible license.
 */

/*****************************************************************************/
/* Advance glue code */

#include "log.h"
#include "target.h"

#define cycles_t target_clock_t
#define osd_cycles() target_clock()
#define osd_cycles_per_second() TARGET_CLOCKS_PER_SEC

__attribute__((format(printf, 1, 0))) static void logerror_(const char *text, ...)
{
	va_list arg;
	va_start(arg, text);
	log_va(text, arg);
	va_end(arg);
}

/* unused */
#define W_WATERMARK_UNDERRUN 0
#define W_WATERMARK_FULL 0
#define W_WATERMARK_GOOD 0
#define W_WATERMARK_OK 0
#define W_WATERMARK_ON_THE_EDGE 0
#define W_WATERMARK_UNDERRUN 0

/*****************************************************************************/
/* wss.c from the VSyncMAME source */

#include <stdio.h>
#include <stdlib.h>
#include <go32.h>
#include <dpmi.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <sys/farptr.h>
#include <pc.h>
#include <ctype.h>
#include <stdarg.h>
#include "wss.h"

/***********************  COMMON  *************************/


#define _8BITMONO		1
#define _8BITSTEREO 	2
#define _16BITSTEREO	3


typedef struct {
	int 	initialized;
	void	(*device_exit)(void);
	void	(*pcm_upload)(void);
	DWORD	(*get_current_pos)(void);
	DWORD	playback_rate;
	int 	pcm_format;
	char	*device_name;
	int 	irq;
	int 	isa_port;
	int 	isa_dma;
	int 	isa_hdma;
} WAVEDEVICE;

static WAVEDEVICE wd = { FALSE, NULL, NULL, NULL, 0, _16BITSTEREO, NULL, -1, -1, -1, -1};

static void wavedevice_struct_init(void)
{
	wd.initialized = FALSE;
	wd.device_exit = NULL;
	wd.pcm_upload  = NULL;
	wd.get_current_pos = NULL;
	wd.playback_rate = 0;
	wd.pcm_format  = _16BITSTEREO;
	wd.device_name = NULL;
	wd.irq		= -1;
	wd.isa_port = -1;
	wd.isa_dma	= -1;
	wd.isa_hdma = -1;
}


static void common_pcm_upload_func(void);
static DWORD common_dma_current_pos(void);



static BOOL _dma_allocate_mem4k(int *sel, unsigned long *phys);
static int _dma_allocate_mem(int *sel, unsigned long *phys);
static void _dma_start(int channel, unsigned long addr, int size, int auto_init, int input);
static void _dma_stop(int channel);
static DWORD _dma_todo(int channel);
static int _dma_count;
static DWORD _dma_counter(void);

#define SAMPLECNT	  16384 		// the buffer size has to be smaller than 64KBytes, for compatibility sake.
#define SAMPLECNTMASK 0x3FFF		// 16bitPCM * stereo * 16384 = 64KBytes


static DWORD g_prev_play_cursor = 0;
static DWORD g_write_cursor = 0;
static DWORD g_latency	= 48000/30;
static float g_latencym = 2.2;
static DWORD g_dma_average_cnt = (48000/60) << 8;
static int	 g_dma_dt = 0;
static int	 g_dma_overflow  = 0;
static int	 g_dma_underflow = 0;

#define DMA_AVERAGE 			256
#define DMA_AVERAGE_MASK		0xFF
#define DMA_AVERAGE_SHIFT_COUNT 8

static DWORD g_dma_remainder = 0;
static long  mixing_buff[SAMPLECNT*2];
static DWORD g_current_req = 0;
static DWORD g_next_req = 0;
static int	 g_master_volume = 256;
static DWORD g_samples_per_frame = 0;



/* dos memory 4k bytes for work area */
static BOOL allocate_dosmem4k(void);
static void free_dosmem4k(void);
static DWORD get_address_dosmem4k(void);
static int g_dosmem4k_sel = NULL;
static unsigned long g_dosmem4k_addr = NULL;
static DWORD g_dosmem4k_phys_table[1];
//static __dpmi_meminfo info4k;

/* dos memory 64k cyclic playing buffer */
static BOOL allocate_dosmem64k_for_dma(int format);
static void free_dosmem64k_for_dma(void);
static DWORD get_address_dosmem64k_for_dma(void);
static int g_wss_dma_sel = NULL;
static unsigned long g_wss_dma_addr = NULL;
static DWORD g_dosmem64k_phys_table[16];
static int g_dma_buff_size;
static int g_dmacnt_shift_count;
static __dpmi_meminfo info64k;



static _go32_dpmi_seginfo old_handler, new_handler;
static int old_handler_irq_vector = -1;

static int tick = 0;

static DWORD actual_sample_rate = 0;
static int sound_device_master_volume  = -1;	// 0:max - 31:min, or -1(default)

static int stereo_reverse_flag = FALSE;




/*	function  */

static DWORD get_played_sample_count(void);
static void calc_next_req(void);


/* error message func */

#define ERROR_MESSAGE_BUFFER_SIZE	1024
static int error_message_pointer = 0;
static char error_message[ERROR_MESSAGE_BUFFER_SIZE] = "\n";

static void set_error_message(char *message)
{
	logerror_("wss:error %s", message);

	while(1){
		if(error_message_pointer >= (ERROR_MESSAGE_BUFFER_SIZE - 1)) break;
		if(*message == '\0') break;
		error_message[error_message_pointer++] = *message++;
	}
	error_message[error_message_pointer] = '\0';
}

static char* get_error_message(void)
{
	return &error_message[0];
}

static void clear_error_message(void)
{
	error_message[0] = '\0';
	error_message_pointer = 0;
}


/***********************  COMMON END  *********************/














/********************************************************************
	PCI BIOS helper funtions
 ********************************************************************/

#define PCI_ANY_ID	((WORD)(~0))

typedef struct {
	WORD vender_id;
	WORD device_id;
	WORD sub_vender_id;
	WORD sub_device_id;
	WORD device_bus_number;
	DWORD base0;
	DWORD base1;
	DWORD base2;
	DWORD base3;
	DWORD base4;
	DWORD base5;
} PCI_DEV;

static PCI_DEV g_pci;


static BOOL pci_read_config_byte(PCI_DEV *pci, int idx, BYTE *data)
{
	__dpmi_regs r;
	BOOL result = TRUE;

	r.d.eax = 0x0000B108;						/* read config byte */
	r.d.ebx = (DWORD)pci->device_bus_number;
	r.d.edi = (DWORD)idx;
	__dpmi_int(0x1a, &r);
	if( r.h.ah != 0 ){
		logerror_("pci read config byte failed\n");
		result = FALSE;
		r.d.ecx = 0;
	}
	*data = (BYTE)r.d.ecx;
	return result;
}

static BOOL pci_read_config_word(PCI_DEV *pci, int idx, WORD *data)
{
	__dpmi_regs r;
	BOOL result = TRUE;

	r.d.eax = 0x0000B109;						/* read config word */
	r.d.ebx = (DWORD)pci->device_bus_number;
	r.d.edi = (DWORD)idx;
	__dpmi_int(0x1a, &r);
	if( r.h.ah != 0 ){
		logerror_("pci read config word failed\n");
		result = FALSE;
		r.d.ecx = 0;
	}
	*data = (WORD)r.d.ecx;
	return result;
}

static BOOL pci_read_config_dword(PCI_DEV *pci, int idx, DWORD *data)
{
	__dpmi_regs r;
	BOOL result = TRUE;

	r.d.eax = 0x0000B10A;						/* read config dword */
	r.d.ebx = (DWORD)pci->device_bus_number;
	r.d.edi = (DWORD)idx;
	__dpmi_int(0x1a, &r);
	if( r.h.ah != 0 ){
		logerror_("pci read config dword failed\n");
		result = FALSE;
		r.d.ecx = 0;
	}
	*data = (DWORD)r.d.ecx;
	return result;
}

static BOOL pci_write_config_byte(PCI_DEV *pci, int idx, BYTE data)
{
	__dpmi_regs r;
	BOOL result = TRUE;

	r.d.eax = 0x0000B10B;						/* write config byte */
	r.d.ebx = (DWORD)pci->device_bus_number;
	r.d.ecx = (DWORD)data;
	r.d.edi = (DWORD)idx;
	__dpmi_int(0x1a, &r);
	if( r.h.ah != 0 ){
		logerror_("pci write config byte failed\n");
		result = FALSE;
	}
	return result;
}

static BOOL pci_write_config_word(PCI_DEV *pci, int idx, WORD data)
{
	__dpmi_regs r;
	BOOL result = TRUE;

	r.d.eax = 0x0000B10C;						/* write config word */
	r.d.ebx = (DWORD)pci->device_bus_number;
	r.d.ecx = (DWORD)data;
	r.d.edi = (DWORD)idx;
	__dpmi_int(0x1a, &r);
	if( r.h.ah != 0 ){
		logerror_("pci write config word failed\n");
		result = FALSE;
	}
	return result;
}

static BOOL pci_write_config_dword(PCI_DEV *pci, int idx, DWORD data)
{
	__dpmi_regs r;
	BOOL result = TRUE;

	r.d.eax = 0x0000B10D;						/* write config dword */
	r.d.ebx = (DWORD)pci->device_bus_number;
	r.d.ecx = (DWORD)data;
	r.d.edi = (DWORD)idx;
	__dpmi_int(0x1a, &r);
	if( r.h.ah != 0 ){
		logerror_("pci write config dword failed\n");
		result = FALSE;
	}
	return result;
}

static BOOL set_pci_hardware_interrupt(PCI_DEV *pci, BYTE int_pin, BYTE irq)
{
	__dpmi_regs r;
	BOOL result = TRUE;

	r.d.eax = 0x0000B10F;						/* set pci hardware interrupt */
	r.d.ebx = (DWORD)pci->device_bus_number;
	r.h.cl	= int_pin;
	r.h.ch	= irq;
	r.x.ds	= 0xF000;
	__dpmi_int(0x1a, &r);
	if( r.h.ah != 0 ){
		logerror_("set pci hardware interrupt failed.\n");
		result = FALSE;
	}
	pci_write_config_byte(pci, 0x3C, irq);
	return result;
}


static BOOL check_pci_bios(void)
{
	__dpmi_regs r;
	BOOL result = TRUE;

	r.d.eax = 0x0000B101;					// PCI BIOS - INSTALLATION CHECK
	r.d.edi = 0x00000000;
	__dpmi_int(0x1a, &r);
	if( r.d.edx != 0x20494350 ){			// ' ICP'
		result = FALSE;
	}
	return result;
}


static BOOL find_pci_device(PCI_DEV *pci)
{
	__dpmi_regs r;
	WORD wdata;

	r.d.eax = 0x0000B102;					// PCI BIOS - FIND PCI DEVICE
	r.d.ecx = pci->device_id;				// device ID
	r.d.edx = pci->vender_id;				// vendor ID
	r.d.esi = 0x00000000;					// device index
	__dpmi_int(0x1a, &r);
	if( r.h.ah != 0 ){
		return FALSE;						// no specified device found
	}
	pci->device_bus_number = r.x.bx;		// save the device & bus number
	if(pci->sub_vender_id != PCI_ANY_ID){
		/* get subsystem vender id */
		if(pci_read_config_word(pci, 0x2C, &wdata) == FALSE) return FALSE;
		if(wdata != pci->sub_vender_id) return FALSE;
	}
	if(pci->sub_device_id != PCI_ANY_ID){
		/* get subsystem device id */
		if(pci_read_config_word(pci, 0x2E, &wdata) == FALSE) return FALSE;
		if(wdata != pci->sub_device_id) return FALSE;
	}
	/* device found */
	return TRUE;
}


static void enable_pci_io_access(PCI_DEV *pci)
{
	WORD wdata;

	pci_read_config_word(pci, 0x04, &wdata);
	/*	set I/O access enable bit  */
	wdata |= 0x0001;
	pci_write_config_word(pci, 0x04, wdata);
}


static void enable_pci_memory_access(PCI_DEV *pci)
{
	WORD wdata;

	pci_read_config_word(pci, 0x04, &wdata);
	/*	set memory access enable bit  */
	wdata |= 0x0002;
	pci_write_config_word(pci, 0x04, wdata);
}

static void enable_pci_bus_master(PCI_DEV *pci)
{
	WORD wdata;

	pci_read_config_word(pci, 0x04, &wdata);
	/*	set bus master enable bit  */
	wdata |= 0x0004;
	pci_write_config_word(pci, 0x04, wdata);
}


/********************************************************************
 *	Windows detection
 ********************************************************************/

static BOOL detect_windows(void)
{
	// return  TRUE:	windows detected
	//		  FALSE: no windows detected

	__dpmi_regs r;

	r.x.ax = 0x1600;
	__dpmi_int(0x2F, &r);
//	  printf("ax = %04x\n", r.x.ax);
	if( (r.h.al & 0x7F) != 0 ){
		return TRUE;
	}
	return FALSE;
}


/********************************************************************
	Interrupt and others helper functions
 ********************************************************************/

static DWORD tasmania = 0;

void w_enter_critical(void)
{
	__dpmi_regs r;

	tasmania += 1;
	if(tasmania == 1){
		r.x.ax = 0x1681;
		__dpmi_int(0x2F, &r);
		asm volatile("cli");
	}
}

void w_exit_critical(void)
{
	__dpmi_regs r;

	tasmania -= 1;
	if(tasmania == 0){
		r.x.ax = 0x1682;
		__dpmi_int(0x2F, &r);
		asm volatile("sti");
	}
}

static void eoi(int irq)
{
	outportb(0x20, 0x20);
	if (irq > 7) outportb(0xA0, 0x20);
}

static void unmask_irq(int irq)
{
	BYTE d0;

	d0 = inportb(0x21);

	if (irq > 7) {
		outportb(0x21, d0 & 0xFB);
		d0 = inportb(0xA1);
		outportb(0xA1, d0 & ~(1<<(irq-8)) );
   }else{
	  outportb(0x21, d0 & (~(1<<irq)) );
   }
}

static void mask_irq(int irq)
{
	BYTE d0;

	if (irq > 7) {
		d0 = inportb(0xA1);
		outportb(0xA1, d0 | (1<<(irq-8)));
	}else{
		d0 = inportb(0x21);
		outportb(0x21, d0 | (1<<irq));
	}
}


static int install_irq_handler(int irq, void *my_handler)
{
	int bl = TRUE;
	int irq_vector = 0;

	if( !(0 <= irq && irq <= 15) ) return FALSE;

	if(irq < 8){
		irq_vector = irq + 0x08;
	}else{
		irq_vector = irq - 8 + 0x70;
	}

	if(old_handler_irq_vector == -1){
		old_handler_irq_vector = irq_vector;
		_go32_dpmi_get_protected_mode_interrupt_vector(irq_vector, &old_handler);
		new_handler.pm_offset = (long)my_handler;
		_go32_dpmi_allocate_iret_wrapper(&new_handler);
		_go32_dpmi_set_protected_mode_interrupt_vector(irq_vector, &new_handler);
	}else{
		bl = FALSE;
	}

	return bl;
}

static void restore_irq_handler(void)
{
	if(old_handler_irq_vector != -1){
		_go32_dpmi_set_protected_mode_interrupt_vector(old_handler_irq_vector, &old_handler);
		_go32_dpmi_free_iret_wrapper(&new_handler);
		old_handler_irq_vector = -1;
	}
}


static void udelay(int usec)
{
	cycles_t  prev;
	double t;
	cycles_t  cps = osd_cycles_per_second();

	t = ((double)cps / 1000000.0) * usec;
	prev = osd_cycles();
	while(1){
		if( (osd_cycles() - prev) >= (cycles_t)t ) break;
	}
}



/********************************************************************
	DOS memory allocation helper funtions
 ********************************************************************/

static void copy_to_dos_memory(DWORD dos_address, BYTE *src, DWORD length)
{
	int d0;

	_farsetsel(_dos_ds);
	d0 = 0;
	while(d0 < length){
		_farnspokeb(dos_address + d0, *(src + d0));
		d0 += 1;
	}
}

static void copy_from_dos_memory(DWORD dos_address, BYTE *dest, DWORD length)
{
	int d0;

	_farsetsel(_dos_ds);
	d0 = 0;
	while(d0 < length){
		*(dest + d0) = _farnspeekb(dos_address + d0);
		d0 += 1;
	}
}


static BOOL allocate_dosmem4k(void)
{
	if(g_dosmem4k_sel  != NULL) return FALSE;
	if(g_dosmem4k_addr != NULL) return FALSE;
	if(_dma_allocate_mem4k(&g_dosmem4k_sel, &g_dosmem4k_addr) == FALSE){
		return FALSE;
	}
//	  info4k.size	 = 4096;
//	  info4k.address = g_dosmem4k_addr;
//	  __dpmi_lock_linear_region(&info4k);

	return TRUE;
}

static void free_dosmem4k(void)
{
	if(g_dosmem4k_sel != NULL && g_dosmem4k_addr != NULL) {
	//	  if(info4k.address != NULL){
	//		  __dpmi_unlock_linear_region(&info4k);
	//	  }
		__dpmi_free_dos_memory(g_dosmem4k_sel);
	}

	g_dosmem4k_sel = NULL;
	g_dosmem4k_addr = NULL;
}

static DWORD get_address_dosmem4k(void)
{
	return g_dosmem4k_addr;
}


static BOOL allocate_dosmem64k_for_dma(int format)
{
	BYTE *a0;
	int d0;
	BOOL lockflag = FALSE;

	g_dma_buff_size = SAMPLECNT;
	switch(format){
		case _8BITMONO:
			g_dma_buff_size *= 1;
			g_dmacnt_shift_count = 0;
			break;
		case _8BITSTEREO:
			g_dma_buff_size *= 2;
			g_dmacnt_shift_count = 1;
			break;
		case _16BITSTEREO:
			g_dma_buff_size *= 4;
			g_dmacnt_shift_count = 2;
			break;
		default:
			g_dma_buff_size = 0;
	}
	if(g_dma_buff_size == 0) return FALSE;
	a0 = malloc(g_dma_buff_size);
	if(a0 == NULL) return FALSE;
	if (_dma_allocate_mem(&g_wss_dma_sel, &g_wss_dma_addr) == FALSE)
		return FALSE;
	d0 = 0;
	while(d0 < g_dma_buff_size){
		if(format == _8BITSTEREO || format == _8BITMONO)
			a0[d0] = 0x80;
		else
			a0[d0] = 0x00;
		d0 += 1;
	}
	copy_to_dos_memory(g_wss_dma_addr, a0, g_dma_buff_size);
	free(a0);

	if(lockflag == TRUE){
		info64k.size	= 65536;
		info64k.address = g_wss_dma_addr;
		__dpmi_lock_linear_region(&info64k);
	}else{
		info64k.address = NULL;
	}

	return TRUE;
}

static void free_dosmem64k_for_dma(void)
{
	if(g_wss_dma_sel  == NULL) return;
	if(g_wss_dma_addr == NULL) return;
	if(info64k.address != NULL){
		__dpmi_unlock_linear_region(&info64k);
	}
	__dpmi_free_dos_memory(g_wss_dma_sel);
}

static DWORD get_address_dosmem64k_for_dma(void)
{
	return g_wss_dma_addr;
}





/********************************************************************
	VDS helper functions
 ********************************************************************/

typedef struct {
	DWORD Region_Size		__attribute__ ((packed));
	DWORD Offset			__attribute__ ((packed));
	WORD  Seg_or_Select 	__attribute__ ((packed));
	WORD  Buffer_ID 		__attribute__ ((packed));
	DWORD Physical_Address	__attribute__ ((packed));
} DDS;


static BOOL is_vds_available(void)
{
	__dpmi_regs r;
	int available;
	BOOL flag = FALSE;

	available = (_farpeekb (_dos_ds, 0x0047b) & 0x20) != 0;

	if(available != 0){
		// get vds verson nunber
		r.x.ax = 0x8102;
		r.x.dx = 0x0000;
		__dpmi_int(0x4B, &r);
		if((r.x.flags & 0x01) == 0){		// carry is clear if successful
			if(r.x.ax != 0x8102){
				flag = TRUE;
			}
		}
	}

	return flag;
}


static BOOL vds_helper_lock(DWORD buffer_addr, DWORD *physical_addr_table, int pages)
{
	__dpmi_regs r;
	DWORD temp;
	DDS dds;
	int d0;
	int sel;
	unsigned long dos_addr;
	BOOL vds_available;

	// determine Virtual DMA Services is available or not
	// if VDS is not available, assume running in Pure DOS (no EMM386 present)
	vds_available = is_vds_available();

	if(_dma_allocate_mem4k(&sel, &dos_addr) == FALSE){
		set_error_message("vds_helper_lock: _dma_allocate_mem4k() error.\n");
		return FALSE;
	}
	memset(&dds, 0, sizeof(dds));

	d0 = 0;
	while(d0 < pages){
		// page block size is 4096 bytes
		temp = buffer_addr + (4096 * d0);
		if(vds_available == TRUE){
			// VDS is available
			dds.Region_Size   = 4096;
			dds.Offset		  = temp & 0x0F;
			dds.Seg_or_Select = temp >> 4;
//			printf("%08x : %08x : %08x\n", temp, dds.Offset, dds.Seg_or_Select);

			// copy DDS struct to dos memory to call VDS service
			copy_to_dos_memory(dos_addr, (BYTE*)&dds, sizeof(dds));
			r.x.ax = 0x8103;			// LOCK DMA BUFFER REGION
			r.x.dx = 0x0000;
			r.x.di = dos_addr & 0xF;
			r.x.es = dos_addr >> 4;

			// call Virtual DMA Services 0x8103 (LOCK DMA BUFFER REGION)
			__dpmi_int(0x4B, &r);
			if((r.x.flags & 0x01) != 0){
				set_error_message("vds_helper_lock: LOCK DMA BUFFER REGION error\n");
				__dpmi_free_dos_memory(sel);
				return FALSE;
			}

			// copy the result of calling VDS service from dos memory
			copy_from_dos_memory(dos_addr, (BYTE*)&dds, sizeof(dds));
			if(dds.Buffer_ID != 0){
				// a buffer in dos memory was allocated, instead of the buffer was prepared
				// but only need the physical address of the buffer was prepared
				set_error_message("vds_helper_lock: Buffer_ID allocated. error\n");
				__dpmi_free_dos_memory(sel);
				return FALSE;
			}
			physical_addr_table[d0] = dds.Physical_Address;
		}else{
			// VDS is not available
			// assume running in Pure DOS (no EMM386 present), no need to get the physical address
			physical_addr_table[d0] = temp;
		}
		// next page block
		d0 += 1;
	}
	__dpmi_free_dos_memory(sel);

	return TRUE;
}

static BOOL vds_helper_unlock(DWORD buffer_addr, DWORD *physical_addr_table, int pages)
{
	__dpmi_regs r;
	DWORD temp;
	DDS dds;
	int d0;
	int sel;
	unsigned long dos_addr;
	BOOL vds_available;

	vds_available = is_vds_available();

	if(_dma_allocate_mem4k(&sel, &dos_addr) == FALSE){
		set_error_message("vds_helper_unlock: _dma_allocate_mem4k() error.\n");
		return FALSE;
	}
	memset(&dds, 0, sizeof(dds));

	d0 = 0;
	while(d0 < pages){
		if(vds_available == TRUE){
			/* vds available */
			temp = buffer_addr + (4096 * d0);
			dds.Region_Size   = 4096;
			dds.Offset		  = temp & 0x0F;
			dds.Seg_or_Select = temp >> 4;
			dds.Buffer_ID	  = 0;
			dds.Physical_Address = physical_addr_table[d0];
//			printf("%08x : %08x : %08x\n", temp, dds.Offset, dds.Seg_or_Select);
			copy_to_dos_memory(dos_addr, (BYTE*)&dds, sizeof(dds));
			r.x.ax = 0x8104;			// UNLOCK DMA BUFFER REGION
			r.x.dx = 0x0000;
			r.x.di = dos_addr & 0xF;
			r.x.es = dos_addr >> 4;
			__dpmi_int(0x4B, &r);
			if((r.x.flags & 0x01) != 0){
				set_error_message("vds_helper_unlock: UNLOCK DMA BUFFER REGION error\n");
				__dpmi_free_dos_memory(sel);
				return FALSE;
			}
		}
		physical_addr_table[d0] = NULL;
		d0 += 1;
	}
	__dpmi_free_dos_memory(sel);

	return TRUE;
}


/********************************************************************
	DMA helper functions
	This is based on Allegro liblary.
 ********************************************************************/

// 8bit DMAC
#define DMA1_STAT		0x08
#define DMA1_WCMD		0x08
#define DMA1_WREQ		0x09
#define DMA1_SNGL		0x0A
#define DMA1_MODE		0x0B
#define DMA1_CLRFF		0x0C
#define DMA1_MCLR		0x0D
#define DMA1_CLRM		0x0E
#define DMA1_WRTALL 	0x0F

// 16bit DMAC
#define DMA2_STAT		0xD0
#define DMA2_WCMD		0xD0
#define DMA2_WREQ		0xD2
#define DMA2_SNGL		0xD4
#define DMA2_MODE		0xD6
#define DMA2_CLRFF		0xD8
#define DMA2_MCLR		0xDA
#define DMA2_CLRM		0xDC
#define DMA2_WRTALL 	0xDE


#define DMA0_ADDR		0x00
#define DMA0_CNT		0x01
#define DMA1_ADDR		0x02
#define DMA1_CNT		0x03
#define DMA2_ADDR		0x04
#define DMA2_CNT		0x05
#define DMA3_ADDR		0x06
#define DMA3_CNT		0x07
#define DMA4_ADDR		0xC0
#define DMA4_CNT		0xC2
#define DMA5_ADDR		0xC4
#define DMA5_CNT		0xC6
#define DMA6_ADDR		0xC8
#define DMA6_CNT		0xCA
#define DMA7_ADDR		0xCC
#define DMA7_CNT		0xCE

#define DMA0_PAGE		0x87
#define DMA1_PAGE		0x83
#define DMA2_PAGE		0x81
#define DMA3_PAGE		0x82
#define DMA4_PAGE		0x8F
#define DMA5_PAGE		0x8B
#define DMA6_PAGE		0x89
#define DMA7_PAGE		0x8A


typedef struct {
   BYTE dma_disable;
   BYTE dma_enable;
   WORD page;
   WORD addr;
   WORD count;
   WORD single;
   WORD mode;
   WORD clear_ff;
   BYTE write;
   BYTE read;
} DMA_S;



static DMA_S mydma[] =
{
   /* channel 0 */
   { 0x04, 0x00, DMA0_PAGE, DMA0_ADDR, DMA0_CNT,
	 DMA1_SNGL, DMA1_MODE, DMA1_CLRFF, 0x48, 0x44 },

   /* channel 1 */
   { 0x05, 0x01, DMA1_PAGE, DMA1_ADDR, DMA1_CNT,
	 DMA1_SNGL, DMA1_MODE, DMA1_CLRFF, 0x49, 0x45 },

   /* channel 2 */
   { 0x06, 0x02, DMA2_PAGE, DMA2_ADDR, DMA2_CNT,
	 DMA1_SNGL, DMA1_MODE, DMA1_CLRFF, 0x4A, 0x46 },

   /* channel 3 */
   { 0x07, 0x03, DMA3_PAGE, DMA3_ADDR, DMA3_CNT,
	 DMA1_SNGL, DMA1_MODE, DMA1_CLRFF, 0x4B, 0x47 },

   /* channel 4 */
   { 0x04, 0x00, DMA4_PAGE, DMA4_ADDR, DMA4_CNT,
	 DMA2_SNGL, DMA2_MODE, DMA2_CLRFF, 0x48, 0x44 },

   /* channel 5 */
   { 0x05, 0x01, DMA5_PAGE, DMA5_ADDR, DMA5_CNT,
	 DMA2_SNGL, DMA2_MODE, DMA2_CLRFF, 0x49, 0x45 },

   /* channel 6 */
   { 0x06, 0x02, DMA6_PAGE, DMA6_ADDR, DMA6_CNT,
	 DMA2_SNGL, DMA2_MODE, DMA2_CLRFF, 0x4A, 0x46 },

   /* channel 7 */
   { 0x07, 0x03, DMA7_PAGE, DMA7_ADDR, DMA7_CNT,
	 DMA2_SNGL, DMA2_MODE, DMA2_CLRFF, 0x4B, 0x47 }
};



inline BYTE get_8bit_pcm_value(long value)
{
	BYTE d0;

	if(value >	32767 - 128) value =  32767 - 128;
	if(value < -32768)		 value = -32768;
	d0 = (value + 128 - ((value >> 15) & 0x0001)) >> 8;

	return (d0 + 0x80);
}

#if 0

static int _dma_allocate_mem(int bytes, int *sel, unsigned long *phys)
{
   int seg;

   /* allocate twice as much memory as we really need */
   seg = __dpmi_allocate_dos_memory((bytes*2 + 15) >> 4, sel);

   if (seg < 0) {
	  *sel = 0;
	  *phys = 0;
	  return -1;
   }

   *phys = seg << 4;

   /* if it crosses a page boundary, use the second half of the block */
   if ((*phys>>16) != ((*phys+bytes)>>16))
	  *phys += bytes;

   return 0;
}

#endif

static int _dma_allocate_mem(int *sel, unsigned long *phys)
{
	int seg;

	/* allocate twice as much memory as we really need */
	seg = __dpmi_allocate_dos_memory(131072 >> 4, sel); 	// 128K

	if (seg < 0) {
	   *sel = 0;
	   *phys = 0;
	   return FALSE;
	}

   *phys = seg << 4;

   *phys = (*phys + 65535) & ~0xFFFF ;						// 64K boundary

   return TRUE;
}

static BOOL _dma_allocate_mem4k(int *sel, unsigned long *phys)
{
	int seg;

	/* allocate twice as much memory as we really need */
	seg = __dpmi_allocate_dos_memory(8192 >> 4, sel);	  // 8K

	if (seg < 0) {
	   *sel = 0;
	   *phys = 0;
	   return FALSE;
	}

   *phys = seg << 4;

   *phys = (*phys + 4095) & ~0x0FFF ;					   // 4K boundary

   return TRUE;
}



static void _dma_start(int channel, unsigned long addr, int size, int auto_init, int input)
{
   DMA_S *tdma;
   unsigned long page, offset;
   int mode;

   tdma = &mydma[channel];
   page = addr >> 16;

   if (channel >= 4) {
	  addr >>= 1;
	  size >>= 1;
   }

   offset = addr & 0xFFFF;
   size--;

   _dma_count = size;

   mode = (input) ? tdma->read : tdma->write;
   if (auto_init)
	  mode |= 0x10;

   outportb(tdma->single, tdma->dma_disable);
   outportb(tdma->mode, mode);
   outportb(tdma->clear_ff, 0);
   outportb(tdma->addr, offset & 0xFF);
   outportb(tdma->addr, offset >> 8);
   outportb(tdma->page, page);
   outportb(tdma->clear_ff, 0);
   outportb(tdma->count, size & 0xFF);
   outportb(tdma->count, size >> 8);
   outportb(tdma->single, tdma->dma_enable);
}


static void _dma_stop(int channel)
{
   DMA_S *tdma = &mydma[channel];
   outportb(tdma->single, tdma->dma_disable);
}







static DWORD _dma_todo(int channel)
{
	int val1, val2 = 0;
	DMA_S *tdma = &mydma[channel];
	int _dma_todo_limit = 0;

	outportb(tdma->clear_ff, 0xff);
	while(_dma_todo_limit < 1024){
		val1  = inportb(tdma->count);
		val1 |= inportb(tdma->count) << 8;
		val2  = inportb(tdma->count);
		val2 |= inportb(tdma->count) << 8;
		val1 -= val2;
		if(0 <= val1 && val1 < 8) break;
		_dma_todo_limit += 1;
	}
	if(val2 > _dma_count) val2 = _dma_count;
	if(channel > 3) val2 <<= 1;
	return val2;
}



/********************************************************************
	AC'97 drivers
	This is based on ALSA drivers
 ********************************************************************/

#define AC97_RESET				0x00	/* Reset */
#define AC97_MASTER 			0x02	/* Master Volume */
#define AC97_HEADPHONE			0x04	/* Headphone Volume (optional) */
#define AC97_PCM				0x18	/* PCM Volume */
#define AC97_EXTENDED_ID		0x28	/* Extended Audio ID */
#define AC97_EXTENDED_STATUS	0x2a	/* Extended Audio Status */
#define AC97_PCM_FRONT_DAC_RATE 0x2c	/* PCM Front DAC Rate */

#define AC97_EA_VRA 	0x0001	/* Variable bit rate enable bit */


typedef struct {
	WORD vender_id;
	WORD device_id;
	WORD sub_vender_id;
	WORD sub_device_id;
	int  type;
	char *string;
} AC97_DEVICE_LIST;


/********************************************************************
	VIA 686/8233 driver

 note:
	This driver works with 8233 perhaps.
 ********************************************************************/


#define VIA686_AC97ControllerCommand   0x80

#define VIA686_AC97ControllerBusy	   (1 << 24)
#define VIA686_PrimaryCodecDataValid   (1 << 25)

#define VIA686_CodecRead   (1 << 23)
#define VIA686_CodecWrite  0
#define VIA686_INDEX_MASK  0x7F

#define VIA686_PrimaryCodec    0x00000000

#define VIA_REG_OFFSET_STATUS			0x00	/* byte - channel status */
#define VIA_REG_OFFSET_TYPE 			0x02	/* byte - channel type */
#define VIA_REG_OFFSET_CURR_PTR 		0x04	/* dword - channel current pointer */
#define VIA_REG_PLAYBACK_CURR_COUNT 	0x0C	/* dword - channel current count */

#define VIA_REG_OFFSET_CONTROL			0x01	/* byte - channel control */
#define   VIA_REG_CTRL_START			0x80	/* WO */
#define   VIA_REG_CTRL_TERMINATE		0x40	/* WO */
#define   VIA_REG_CTRL_PAUSE			0x08	/* RW */
#define   VIA_REG_CTRL_RESET			0x01	/* RW - probably reset? undocumented */
// VT8233
#define   VIA_REG_CTRL_AUTOSTART		0x20

// VT8233
/* multi-channel playback */
#define VIA_REG_MULTPLAY_STATUS 		0x40	/* byte - channel status */
#define VIA_REG_MULTPLAY_CONTROL		0x41	/* byte - channel control */
#define VIA_REG_MULTPLAY_FORMAT 		0x42	/* byte - format and channels */
#define VIA_REG_MULTPLAY_FMT_8BIT		0x00
#define VIA_REG_MULTPLAY_FMT_16BIT		0x80
#define VIA_REG_MULTPLAY_FMT_CH_MASK	0x70	/* # channels << 4 (valid = 1,2,4,6) */
#define VIA_REG_MULTPLAY_SCRATCH		0x43	/* byte - nop */
#define VIA_REG_MULTPLAY_TABLE_PTR		0x44	/* dword - channel table pointer */
#define VIA_REG_MULTPLAY_CURR_PTR		0x44	/* dword - channel current pointer */
#define VIA_REG_MULTPLAY_STOP_IDX		0x48	/* dword - stop index, slots */
#define VIA_REG_MULTPLAY_CURR_COUNT 	0x4c	/* dword - channel current count (24 bit) */
#define VIA_REG_MULTIPLAY_CURR_INDEX	0x4f	/* byte - channel current index */


#define DEVICE_VT82C686    0
#define DEVICE_VT8233	   1

static AC97_DEVICE_LIST via686_dev_list[] = {
	{ 0x1106, 0x3058, PCI_ANY_ID, PCI_ANY_ID, DEVICE_VT82C686, "VIA VT82C686 integrated AC97 audio" },
	{ 0x1106, 0x3059, PCI_ANY_ID, PCI_ANY_ID, DEVICE_VT8233,   "VIA VT8233 integrated AC97 audio" },

	{ 0x0000, 0x0000, PCI_ANY_ID, PCI_ANY_ID, 0, "" }
};

static int via686_device_type = DEVICE_VT82C686;


static DWORD via686_ReadAC97Codec_sub(void)
{
	DWORD d0;
	int limit = 1024;

	/* wait for ready */
	do{
		d0 = inportl(g_pci.base0 + VIA686_AC97ControllerCommand);
//		  printf("read debug %x\n", d0);
		/* always valid, maybe */
		if( (d0 & VIA686_PrimaryCodecDataValid) != 0 ) break;
		udelay(100);
	}while(limit-- > 0);

	d0 = inportl(g_pci.base0 + VIA686_AC97ControllerCommand);
	return d0;
}


static void via686_WriteAC97Codec_sub(DWORD value_to_write)
{
	DWORD d0;
	int limit = 1024;

	/* wait for ready (writing) */
	do{
		d0 = inportl(g_pci.base0 + VIA686_AC97ControllerCommand);
//		  printf("%x\n", d0);
		if( (d0 & VIA686_AC97ControllerBusy) == 0 ) break;
		udelay(100);
	}while(limit-- > 0);

	outportl(g_pci.base0 + VIA686_AC97ControllerCommand, value_to_write);

	/* wait for ready (reading) */
	limit = 1024;
	do{
		d0 = inportl(g_pci.base0 + VIA686_AC97ControllerCommand);
//		  printf("%x\n", d0);
		if( (d0 & VIA686_AC97ControllerBusy) == 0 ) break;
		udelay(100);
	}while(limit-- > 0);
}

static WORD via686_ReadAC97Codec(BYTE idx)
{
	DWORD d0;

	idx = idx & VIA686_INDEX_MASK;
	d0 = VIA686_PrimaryCodec | VIA686_CodecRead | ((DWORD)idx << 16);
	via686_WriteAC97Codec_sub(d0);
	via686_WriteAC97Codec_sub(d0);

	return via686_ReadAC97Codec_sub();
}

static void via686_WriteAC97Codec(BYTE idx, WORD data)
{
	DWORD d0;

	idx = idx & VIA686_INDEX_MASK;
	d0 = VIA686_PrimaryCodec | VIA686_CodecWrite | ((DWORD)idx << 16) | data;
	via686_WriteAC97Codec_sub(d0);
	via686_WriteAC97Codec_sub(d0);
}

static void via686_channel_reset(void)
{
	if(via686_device_type == DEVICE_VT82C686){
		outportb(g_pci.base0 + VIA_REG_OFFSET_CONTROL,
					VIA_REG_CTRL_PAUSE | VIA_REG_CTRL_TERMINATE | VIA_REG_CTRL_RESET);
		udelay(50);
		outportb(g_pci.base0 + VIA_REG_OFFSET_CONTROL, 0x00);
		outportb(g_pci.base0 + VIA_REG_OFFSET_STATUS, 0xFF);
		outportb(g_pci.base0 + VIA_REG_OFFSET_TYPE, 0x00);
		outportl(g_pci.base0 + VIA_REG_OFFSET_CURR_PTR, 0);
	}else{
		outportb(g_pci.base0 + VIA_REG_MULTPLAY_CONTROL, VIA_REG_CTRL_TERMINATE);
		udelay(50);
		/* disable interrupts */
		outportb(g_pci.base0 + VIA_REG_MULTPLAY_CONTROL, 0x00);
		/* clear interrupts */
		outportb(g_pci.base0 + VIA_REG_MULTPLAY_STATUS, 0x03);
	}
}

static DWORD via686_current_pos(void)
{
	DWORD d0 = 0;
	DWORD d1 = 0;
	DWORD d2;
	int limit = 1024;

	w_enter_critical();
	while(limit > 0){
	   if(via686_device_type == DEVICE_VT82C686){
			d0 = inportl(g_pci.base0 + VIA_REG_OFFSET_CURR_PTR);
			d1 = inportl(g_pci.base0 + VIA_REG_PLAYBACK_CURR_COUNT);
			d2 = inportl(g_pci.base0 + VIA_REG_OFFSET_CURR_PTR);
		}else{
			d0 = inportl(g_pci.base0 + VIA_REG_MULTPLAY_CURR_PTR);
			d1 = inportl(g_pci.base0 + VIA_REG_MULTPLAY_CURR_COUNT);
			d2 = inportl(g_pci.base0 + VIA_REG_MULTPLAY_CURR_PTR);
		}
		if(d0 == d2) break;

		// the pointer of lookup table has been advanced while polling
		// try again
		limit -= 1;
	}
	w_exit_critical();


	// g_dosmem4k_phys_table[0] is top address of lookup table of physical addresses for 4096 bytes blocks.
	//
	// *(g_dosmem4k_phys_table[0] +  0) physical address of 1st 4096 bytes block
	// *(g_dosmem4k_phys_table[0] +  4) 0x00001000 (size of 1st block, always 4096)
	// *(g_dosmem4k_phys_table[0] +  8) physical address of 2nd 4096 bytes block
	// *(g_dosmem4k_phys_table[0] + 12) 0x00001000 (size of 2nd block, always 4096)
	//									.
	//									.
	//									.

	// compute playback cursor position
	d0 = d0 - g_dosmem4k_phys_table[0]; 	// d0 is address of lookup table of playing cursor
	d0 = d0 >> 3;							// current block number of playing cursor
	d0 = d0 - 1;
	d0 = (15 - d0) & 0x0F;					// the number of buffers is 16, for the cyclic buffer
	d0 = (d0 * 4096) + (d1 & 0x00FFFFFF);	// d1 is byte offset of 4096 bytes block.
	d0 = d0 & 0x0000FFFF;

	return d0;
}

static void via686_exit(void)
{
	via686_channel_reset();
	if(vds_helper_unlock(get_address_dosmem64k_for_dma(), &g_dosmem64k_phys_table[0], 16) == FALSE){
		set_error_message("vds_helper_unlock error\n");
	}
	if(vds_helper_unlock(get_address_dosmem4k(), &g_dosmem4k_phys_table[0], 1) == FALSE){
		set_error_message("vds_helper_unlock error\n");
	}
	free_dosmem64k_for_dma();
	free_dosmem4k();
	wavedevice_struct_init();
}

static void via686_set_volume(void)
{
	BYTE vol;

	if(sound_device_master_volume == -1){
		vol = 0x08;    // default, -12dB
	}else{
		vol = sound_device_master_volume;
	}
	/* reset codec */
	via686_WriteAC97Codec(AC97_RESET, 0x00);
	udelay(100);

	via686_WriteAC97Codec(AC97_MASTER,	  vol * 0x0101);		/* Master Volume */
//	via686_WriteAC97Codec(AC97_HEADPHONE, vol * 0x0101);		/* Headphone Volume (optional) */
	via686_WriteAC97Codec(AC97_PCM, 0x0808);					/* PCM OUT	0dB */
}

static void via686_chip_init(BOOL initialize)
{
	BYTE data;

	if(initialize == TRUE){
		if(via686_device_type == DEVICE_VT82C686){
			/* deassert ACLink reset, force SYNC */
			pci_write_config_byte(&g_pci, 0x41, 0xE0);
			udelay(100);
			pci_write_config_byte(&g_pci, 0x41, 0x00);
			udelay(100);
			/* ACLink on, deassert ACLink reset, VSR, SGD data out */
			/* note - FM data out has trouble with non VRA codecs !! */
			pci_write_config_byte(&g_pci, 0x41, 0xCD);
			udelay(100);
		}else{
			/* deassert ACLink reset */
			pci_write_config_byte(&g_pci, 0x41, 0xE0);
			udelay(100);
			/* deassert ACLink reset, force SYNC (warm AC'97 reset) */
			pci_write_config_byte(&g_pci, 0x41, 0x60);
			udelay(2);
			/* ACLink on, deassert ACLink reset, VSR, SGD data out */
			pci_write_config_byte(&g_pci, 0x41, 0xcc);
		}
		via686_set_volume();
	}else{
		if(0){
			if(via686_device_type == DEVICE_VT82C686){
				pci_read_config_byte(&g_pci, 0x41, &data);
				/* enable read channel pcm data output */
				data |= 0x04;
				pci_write_config_byte(&g_pci, 0x41, 0xCD);
			}
		}
	}
}


static BOOL via686_start(int rate, BOOL initialize)
{
	static char *device_name_via686 = NULL;
	DWORD data;
	int d0;
	DWORD a0;

	/* now via686 driver may work in dos-box under windows9x */
	if(0){
		if(detect_windows() == TRUE){
			set_error_message("VIA686: Windows detected. VIA686 driver only works in Pure DOS.\n");
			return FALSE;
		}
	}

	if(check_pci_bios() == FALSE){
		set_error_message("VIA686: No PCI BIOS found.\n");
		return FALSE;
	}

	/* detection */
	d0 = 0;
	while(via686_dev_list[d0].vender_id != 0x0000){
		g_pci.vender_id 	= via686_dev_list[d0].vender_id;
		g_pci.device_id 	= via686_dev_list[d0].device_id;
		g_pci.sub_vender_id = via686_dev_list[d0].sub_vender_id;
		g_pci.sub_device_id = via686_dev_list[d0].sub_device_id;
		if(find_pci_device(&g_pci) == TRUE){
			logerror_("VIA686: %s found.\n", via686_dev_list[d0].string);
			break;
		}
		d0 += 1;
	}
	if(via686_dev_list[d0].vender_id == 0x0000){
		set_error_message("VIA686: no compatble device found.\n");
		return FALSE;
	}
	via686_device_type = via686_dev_list[d0].type;
	device_name_via686 = via686_dev_list[d0].string;

	/* get io base address */
	pci_read_config_dword(&g_pci, 0x10, &data);
	data &= 0xFF00;
	if(data == 0){
		set_error_message("VIA686: device found, but disabled.\n");
		return FALSE;
	}
	g_pci.base0 = data;
	logerror_("VIA686: pci base0 at I/O %04X\n", g_pci.base0);

	/* initialize */
	via686_chip_init(initialize);

	/* playing buffer in dos memory */
	if(allocate_dosmem64k_for_dma(_16BITSTEREO) == FALSE){
		set_error_message("VIA686: allocate_dosmem64k_for_dma() error.\n");
//		printf("error\n");
		return FALSE;
	}
	/* get physical address every 4KB */
	if(vds_helper_lock(get_address_dosmem64k_for_dma(), &g_dosmem64k_phys_table[0], 16) == FALSE){
		set_error_message("VIA686: vds_helper_lock() error.\n");
		return FALSE;
	}

	/* scatter/gather table  buffer in dos memory */
	if(allocate_dosmem4k() == FALSE){
		set_error_message("VIA686: allocate_dosmem4k() error.\n");
		return FALSE;
	}
	/* get physical address */
	if(vds_helper_lock(get_address_dosmem4k(), &g_dosmem4k_phys_table[0], 1) == FALSE){
		set_error_message("VIA686: vds_helper_lock() error.\n");
		return FALSE;
	}

	/* stop and reset if playing */
	via686_channel_reset();

	/* setup scatter/gather table for playback */
	_farsetsel(_dos_ds);
	a0 = get_address_dosmem4k();
	d0 = 0;
	while(d0 < 16){
		_farnspokel(a0	  , g_dosmem64k_phys_table[d0]);
		_farnspokel(a0 + 4, 0x00001000);		// 4096 bytes = 1 buffer
		if(d0 == 15){
			_farnspokel(a0 + 4, 0x80001000);	// set end point
		}
		a0 += 8;
		d0 += 1;
	}

	/* force playback rate 48000Hz */
	data = via686_ReadAC97Codec(AC97_EXTENDED_ID);
	if( (data & AC97_EA_VRA) != 0 ){
		/* if VBR supported, disable VBR, force 48000Hz */
		via686_WriteAC97Codec(AC97_EXTENDED_STATUS, 0x0000);
	}
//	  via686_WriteAC97Codec(AC97_PCM_FRONT_DAC_RATE, 0x80BB);
	udelay(100);

	if(via686_device_type == DEVICE_VT82C686){
		/* VT82C686 */

		/* set scatter/gather table pointer */
		outportl(g_pci.base0 + VIA_REG_OFFSET_CURR_PTR, g_dosmem4k_phys_table[0]);

		/* auto-start, 16bitpcm, stereo */
		outportb(g_pci.base0 + VIA_REG_OFFSET_TYPE, 0xB0);

		/* start playing */
		outportb(g_pci.base0 + VIA_REG_OFFSET_CONTROL, VIA_REG_CTRL_START);

	}else{
		/* VT8233 */

		/* set scatter/gather table pointer */
		outportl(g_pci.base0 + VIA_REG_MULTPLAY_TABLE_PTR, g_dosmem4k_phys_table[0]);

		// stereo (2ch), 16bitpcm
		data  = 2 << 4;
		data |= VIA_REG_MULTPLAY_FMT_16BIT;
		outportb(g_pci.base0 + VIA_REG_MULTPLAY_FORMAT, (BYTE)data);

		// slot 2ch
		data = (1<<0) | (2<<4);
		/* STOP index is never reached */
		outportl(g_pci.base0 + VIA_REG_MULTPLAY_STOP_IDX, 0xFF000000 | data);

		/* start playing */
		outportb(g_pci.base0 + VIA_REG_MULTPLAY_CONTROL, VIA_REG_CTRL_START | VIA_REG_CTRL_AUTOSTART);
	}

	wd.device_name	   = device_name_via686;
	wd.playback_rate   = 48000;
	wd.pcm_format	   = _16BITSTEREO;
	wd.device_exit	   = via686_exit;
	wd.pcm_upload	   = common_pcm_upload_func;
	wd.get_current_pos = via686_current_pos;
	wd.initialized = TRUE;

	return TRUE;
}

static BOOL via686_start_chip_init(int rate)
{
	return via686_start(rate, TRUE);
}

static BOOL via686_start_no_chip_init(int rate)
{
	return via686_start(rate, FALSE);
}




/********************************************************************
	Intel810 and derivations driver

 note:
	This driver was tested with i810(ICH), SiS7012, VIA686.
 ********************************************************************/


/* capture block */
#define ICH_REG_PI_BDBAR		0x00	/* dword - buffer descriptor list base address */
#define ICH_REG_PI_CIV			0x04	/* byte - current index value */
#define ICH_REG_PI_LVI			0x05	/* byte - last valid index */
#define   ICH_REG_LVI_MASK		0x1f
#define ICH_REG_PI_SR			0x06	/* byte - status register */
#define   ICH_FIFOE 		0x10	/* FIFO error */
#define   ICH_BCIS			0x08	/* buffer completion interrupt status */
#define   ICH_LVBCI 		0x04	/* last valid buffer completion interrupt */
#define   ICH_CELV			0x02	/* current equals last valid */
#define   ICH_DCH			0x01	/* DMA controller halted */
#define ICH_REG_PI_PICB 		0x08	/* word - position in current buffer */
#define ICH_REG_PI_PIV			0x0a	/* byte - prefetched index value */
#define   ICH_REG_PIV_MASK		0x1f	/* mask */
#define ICH_REG_PI_CR			0x0b	/* byte - control register */
#define   ICH_IOCE			0x10	/* interrupt on completion enable */
#define   ICH_FEIE			0x08	/* fifo error interrupt enable */
#define   ICH_LVBIE 		0x04	/* last valid buffer interrupt enable */
#define   ICH_RESETREGS 		0x02	/* reset busmaster registers */
#define   ICH_STARTBM			0x01	/* start busmaster operation */
/* playback block */
#define ICH_REG_PO_BDBAR		0x10	/* dword - buffer descriptor list base address */
#define ICH_REG_PO_CIV			0x14	/* byte - current index value */
#define ICH_REG_PO_LVI			0x15	/* byte - last valid command */
#define ICH_REG_PO_SR			0x16	/* byte - status register */
#define ICH_REG_PO_PICB 		0x18	/* word - position in current buffer */
#define ICH_REG_PO_PIV			0x1a	/* byte - prefetched index value */
#define ICH_REG_PO_CR			0x1b	/* byte - control register */
/* mic capture block */
#define ICH_REG_MC_BDBAR		0x20	/* dword - buffer descriptor list base address */
#define ICH_REG_MC_CIV			0x24	/* byte - current index value */
#define ICH_REG_MC_LVI			0x25	/* byte - last valid command */
#define ICH_REG_MC_SR			0x26	/* byte - status register */
#define ICH_REG_MC_PICB 		0x28	/* word - position in current buffer */
#define ICH_REG_MC_PIV			0x2a	/* byte - prefetched index value */
#define ICH_REG_MC_CR			0x2b	/* byte - control register */
/* global block */
#define ICH_REG_GLOB_CNT		0x2c	/* dword - global control */
#define   ICH_PCM_246_MASK	0x00300000	/* 6 channels (not all chips) */
#define   ICH_PCM_6 	0x00200000	/* 6 channels (not all chips) */
#define   ICH_PCM_4 	0x00100000	/* 4 channels (not all chips) */
#define   ICH_PCM_2 	0x00000000	/* 2 channels (stereo) */
#define   ICH_SRIE		0x00000020	/* secondary resume interrupt enable */
#define   ICH_PRIE		0x00000010	/* primary resume interrupt enable */
#define   ICH_ACLINK		0x00000008	/* AClink shut off */
#define   ICH_AC97WARM		0x00000004	/* AC'97 warm reset */
#define   ICH_AC97COLD		0x00000002	/* AC'97 cold reset */
#define   ICH_GIE		0x00000001	/* GPI interrupt enable */
#define ICH_REG_GLOB_STA		0x30	/* dword - global status */
#define   ICH_MD3		0x00020000	/* modem power down semaphore */
#define   ICH_AD3		0x00010000	/* audio power down semaphore */
#define   ICH_RCS		0x00008000	/* read completion status */
#define   ICH_BIT3		0x00004000	/* bit 3 slot 12 */
#define   ICH_BIT2		0x00002000	/* bit 2 slot 12 */
#define   ICH_BIT1		0x00001000	/* bit 1 slot 12 */
#define   ICH_SRI		0x00000800	/* secondary resume interrupt */
#define   ICH_PRI		0x00000400	/* primary resume interrupt */
#define   ICH_SCR		0x00000200	/* secondary codec ready */
#define   ICH_PCR		0x00000100	/* primary codec ready */
#define   ICH_MCINT 	0x00000080	/* MIC capture interrupt */
#define   ICH_POINT 	0x00000040	/* playback interrupt */
#define   ICH_PIINT 	0x00000020	/* capture interrupt */
#define   ICH_MOINT 	0x00000004	/* modem playback interrupt */
#define   ICH_MIINT 	0x00000002	/* modem capture interrupt */
#define   ICH_GSCI		0x00000001	/* GPI status change interrupt */
#define ICH_REG_ACC_SEMA		0x34	/* byte - codec write semaphore */
#define   ICH_CAS			0x01	/* codec access semaphore */

#define ICH_MAX_FRAGS		32		/* max hw frags */


#define DEVICE_INTEL	0
#define DEVICE_SIS		1

static AC97_DEVICE_LIST intel_ich_dev_list[] = {
	{ 0x8086, 0x2415, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL, "Intel 82801AA integrated AC97 audio" },
	{ 0x8086, 0x2425, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL, "Intel 82901AB integrated AC97 audio" },
	{ 0x8086, 0x2445, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL, "Intel 82801BA integrated AC97 audio" },
	{ 0x8086, 0x2485, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL, "Intel ICH3 integrated AC97 audio"	  },
	{ 0x8086, 0x24c5, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL, "Intel ICH4 integrated AC97 audio"	  },
	{ 0x8086, 0x7195, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL, "Intel 440MX integrated AC97 audio"   },
	{ 0x1039, 0x7012, PCI_ANY_ID, PCI_ANY_ID, DEVICE_SIS,	"SiS SI7012 AC97 audio" 			  },
	{ 0x10de, 0x01b1, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL, "NVIDIA NFORCE integrated AC97 audio" },
	{ 0x1022, 0x764d, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL, "AMD AMD8111 integrated AC97 audio"   },
	{ 0x1022, 0x7445, PCI_ANY_ID, PCI_ANY_ID, DEVICE_INTEL, "AMD AMD768 integrated AC97 audio"	  },

	{ 0x0000, 0x0000, PCI_ANY_ID, PCI_ANY_ID, 0, "" }
};

static int intel_ich_device_type = DEVICE_INTEL;


static BOOL intel_ich_codec_semaphore(void)
{
	DWORD d0;
	int limit = 1024;

	/* codec ready ? */
	d0 = inportl(g_pci.base1 + ICH_REG_GLOB_STA);
	if((d0 & ICH_PCR) == 0){
		return TRUE;
	}

	do{
		d0 = inportb(g_pci.base1 + ICH_REG_ACC_SEMA);
		if((d0 & ICH_CAS) == 0){
			return TRUE;
		}
		udelay(10);
	}while(limit-- > 0);

	return FALSE;
}

static WORD intel_ich_ReadAC97Codec(BYTE idx)
{
	intel_ich_codec_semaphore();
	return inportw(g_pci.base0 + idx);
}

static void intel_ich_WriteAC97Codec(BYTE idx, WORD data)
{
	intel_ich_codec_semaphore();
	outportw(g_pci.base0 + idx, data);
}

static void intel_ich_stop(void)
{
	outportb(g_pci.base1 + ICH_REG_PO_CR, 0x00);
	udelay(100);
	outportb(g_pci.base1 + ICH_REG_PO_CR, 0x02);
	udelay(100);
}

static void intel_ich_set_volume(void)
{
	BYTE vol;

	if(sound_device_master_volume == -1){
		vol = 0;	// default, 0dB
	}else{
		vol = sound_device_master_volume;
	}
	intel_ich_WriteAC97Codec(AC97_RESET, 0x00);

	intel_ich_WriteAC97Codec(AC97_MASTER,	 vol * 0x0101); 	/* Master Volume */
//	intel_ich_WriteAC97Codec(AC97_HEADPHONE, vol * 0x0101); 	/* Headphone Volume (optional) */
	intel_ich_WriteAC97Codec(AC97_PCM, 0x0808); 				/* PCM OUT	0dB */
}




static void intel_ich_init(void)
{
	DWORD cnt;
	int limit;

	/* put logic to right state */
	/* first clear status bits */
	cnt = inportl(g_pci.base1 + ICH_REG_GLOB_STA);
	outportl(g_pci.base1 + ICH_REG_GLOB_STA,
			cnt & (ICH_RCS | ICH_MCINT | ICH_POINT | ICH_PIINT));

	/* ACLink on, 2 channels */
	cnt = inportl(g_pci.base1 + ICH_REG_GLOB_CNT);
	cnt &= ~(ICH_ACLINK | ICH_PCM_246_MASK);
	/* finish cold or do warm reset */
	cnt |= (cnt & ICH_AC97COLD) == 0 ? ICH_AC97COLD : ICH_AC97WARM;
	outportl(g_pci.base1 + ICH_REG_GLOB_CNT, cnt);
	limit = 1024;
	do {
		if((inportl(g_pci.base1 + ICH_REG_GLOB_CNT) & ICH_AC97WARM) == 0) break;
		udelay(1000);
	} while (limit-- > 0);

	/* wait for primary codec ready status.
	 * Once it becomes ready it should remain ready
	 * as long as we do not disable the ac97 link.
	 */
	limit = 1024;
	do {
		if (inportl(g_pci.base1 + ICH_REG_GLOB_STA) & ICH_PCR) break;
		udelay(1000);
	} while (limit-- > 0);

	inportw(g_pci.base0);	 /* clear semaphore flag */

	/* disable interrupts */
	outportb(g_pci.base1 + ICH_REG_PI_CR, 0x00);
	outportb(g_pci.base1 + ICH_REG_PO_CR, 0x00);
	outportb(g_pci.base1 + ICH_REG_MC_CR, 0x00);
	/* reset channels */
	outportb(g_pci.base1 + ICH_REG_PI_CR, ICH_RESETREGS);
	outportb(g_pci.base1 + ICH_REG_PO_CR, ICH_RESETREGS);
	outportb(g_pci.base1 + ICH_REG_MC_CR, ICH_RESETREGS);

	intel_ich_set_volume();
}



static void intel_ich_exit(void)
{
	intel_ich_stop();
	if(vds_helper_unlock(get_address_dosmem64k_for_dma(), &g_dosmem64k_phys_table[0], 16) == FALSE){
		set_error_message("vds_helper_unlock error\n");
	}
	if(vds_helper_unlock(get_address_dosmem4k(), &g_dosmem4k_phys_table[0], 1) == FALSE){
		set_error_message("vds_helper_unlock error\n");
	}
	free_dosmem64k_for_dma();
	free_dosmem4k();
	wavedevice_struct_init();
}

static DWORD intel_ich_current_pos(void)
{
	BYTE d0 = 0;
	WORD d1 = 0;
	BYTE d2;
	DWORD d3;
	int limit = 1024;

	w_enter_critical();
	while(limit > 0){
		if(intel_ich_device_type == DEVICE_SIS){
			d0 = inportb(g_pci.base1 + ICH_REG_PO_CIV);
			d1 = inportw(g_pci.base1 + ICH_REG_PO_SR);
			d2 = inportb(g_pci.base1 + ICH_REG_PO_CIV);
		}else{
			d0 = inportb(g_pci.base1 + ICH_REG_PO_CIV);
			d1 = inportw(g_pci.base1 + ICH_REG_PO_PICB);
			d2 = inportb(g_pci.base1 + ICH_REG_PO_CIV);
		}
		if(d0 == d2) break;
		limit -= 1;
	}
	outportb(g_pci.base1 + ICH_REG_PO_LVI, (d0 + 31) & 0x1F);
	w_exit_critical();

	d3 = (15 - d0) & 0x0F;
	d3 = (d3 * 4096);
	if(intel_ich_device_type == DEVICE_SIS){
		d3 = d3 + d1;
	}else{
		d3 = d3 + (d1 << 1);
	}
	d3 = d3 & 0x0000FFFF;

//	  for debug
//	  d3 = (d0 << 16) | d1;

	return d3;
}


static BOOL intel_ich_start(int rate, BOOL initialize)
{
	static char *device_name_intel_ich = NULL;
	DWORD data;
	int d0;
	DWORD a0;

	/* now intel_ich driver may work in dos box under windows9x */
	if(0){
		if(detect_windows() == TRUE){
			set_error_message("Intel_ICH: Windows detected. Intel_ICH driver only works in Pure DOS.\n");
			return FALSE;
		}
	}

	if(check_pci_bios() == FALSE){
		set_error_message("Intel_ICH: No PCI BIOS found.\n");
		return FALSE;
	}

	/* detection */
	d0 = 0;
	while(intel_ich_dev_list[d0].vender_id != 0x0000){
		g_pci.vender_id 	= intel_ich_dev_list[d0].vender_id;
		g_pci.device_id 	= intel_ich_dev_list[d0].device_id;
		g_pci.sub_vender_id = intel_ich_dev_list[d0].sub_vender_id;
		g_pci.sub_device_id = intel_ich_dev_list[d0].sub_device_id;
		if(find_pci_device(&g_pci) == TRUE){
			logerror_("Intel_ICH: %s found.\n", intel_ich_dev_list[d0].string);
			break;
		}
		d0 += 1;
	}
	if(intel_ich_dev_list[d0].vender_id == 0x0000){
		set_error_message("Intel_ICH: no compatble device found.\n");
		return FALSE;
	}
	intel_ich_device_type = intel_ich_dev_list[d0].type;
	device_name_intel_ich = intel_ich_dev_list[d0].string;

	/* get io base address */
	pci_read_config_dword(&g_pci, 0x10, &data);
	data &= 0xFF00;
	if(data == 0){
		set_error_message("Intel_ICH: device found, but disabled.\n");
		return FALSE;
	}
	g_pci.base0 = data;
	logerror_("Intel_ICH: PCI BASE0 at I/O %04X\n", g_pci.base0);

	pci_read_config_dword(&g_pci, 0x14, &data);
	data &= 0xFFC0;
	if(data == 0){
		set_error_message("Intel_ICH: device found, but disabled.\n");
		return FALSE;
	}
	g_pci.base1 = data;
	logerror_("Intel_ICH: PCI BASE1 at I/O %04X\n", g_pci.base1);

	/* stop and reset if playing */
	intel_ich_stop();

	if(initialize == TRUE){
		/* initialize */
		intel_ich_init();
	}

	/* playing buffer in dos memory */
	if(allocate_dosmem64k_for_dma(_16BITSTEREO) == FALSE){
		set_error_message("Intel_ICH: allocate_dosmem64k_for_dma() error.\n");
//		printf("error\n");
		return FALSE;
	}
	/* get physical address every 4KB */
	if(vds_helper_lock(get_address_dosmem64k_for_dma(), &g_dosmem64k_phys_table[0], 16) == FALSE){
		set_error_message("Intel_ICH: vds_helper_lock() error.\n");
		return FALSE;
	}

	/* scatter/gather table buffer in dos memory */
	if(allocate_dosmem4k() == FALSE){
		set_error_message("Intel_ICH: allocate_dosmem4k() error.\n");
		return FALSE;
	}
	/* get physical address */
	if(vds_helper_lock(get_address_dosmem4k(), &g_dosmem4k_phys_table[0], 1) == FALSE){
		set_error_message("Intel_ICH: vds_helper_lock() error.\n");
		return FALSE;
	}

	/* setup scatter/gather table for playback */
	_farsetsel(_dos_ds);
	a0 = get_address_dosmem4k();
	d0 = 0;
	while(d0 < 32){
		_farnspokel(a0	  , g_dosmem64k_phys_table[d0 & 0x0F]);
		if(intel_ich_device_type == DEVICE_SIS){
			_farnspokel(a0 + 4, 0x00001000);		// in bytes   (4096 bytes = 1 buffer)
		}else{
			_farnspokel(a0 + 4, 0x00000800);		// in samples (16bit pcm)
		}
		a0 += 8;
		d0 += 1;
	}

	/* set scatter/gather table pointer */
	outportl(g_pci.base1 + ICH_REG_PO_BDBAR, g_dosmem4k_phys_table[0]);
	outportb(g_pci.base1 + ICH_REG_PO_LVI, 0x1F);

	/* 16bitpcm, stereo */
	data = inportl(g_pci.base1 + ICH_REG_GLOB_CNT);
	data = data & ~ICH_PCM_246_MASK;
	data = data & ~ICH_GIE;
	outportl(g_pci.base1 + ICH_REG_GLOB_CNT, data);

	/* force playback rate 48000Hz */
	data = intel_ich_ReadAC97Codec(AC97_EXTENDED_ID);
	if( (data & AC97_EA_VRA) != 0 ){
		/* if VBR supported, disable VBR, force 48000Hz */
		intel_ich_WriteAC97Codec(AC97_EXTENDED_STATUS, 0x0000);
	}
//	  intel_ich_WriteAC97Codec(AC97_PCM_FRONT_DAC_RATE, 0x80BB);
	udelay(100);

	/* start playing */
	outportb(g_pci.base1 + ICH_REG_PO_CR, ICH_STARTBM);

	wd.device_name	   = device_name_intel_ich;
	wd.playback_rate   = 48000;
	wd.pcm_format	   = _16BITSTEREO;
	wd.device_exit	   = intel_ich_exit;
	wd.pcm_upload	   = common_pcm_upload_func;
	wd.get_current_pos = intel_ich_current_pos;
	wd.initialized = TRUE;

	return TRUE;
}


static BOOL intel_ich_start_chip_init(int rate)
{
	return intel_ich_start(rate, TRUE);
}

static BOOL intel_ich_start_no_chip_init(int rate)
{
	return intel_ich_start(rate, FALSE);
}


/********************************************************************
	AC97 auto-detection
 ********************************************************************/

static BOOL ac97_auto_detect_start(int rate, BOOL initialize)
{
	clear_error_message();
	if(via686_start(rate, initialize) == TRUE) return TRUE;

//	  clear_error_message();
	if(intel_ich_start(rate, initialize) == TRUE) return TRUE;

//	  clear_error_message();
	set_error_message("AC97_AUTODETECT: no supported device found.\n");
	return FALSE;
}



/********************************************************************
	SoundBlaster and derivations drivers
	This driver is based on Allegro liblary.
 ********************************************************************/

static int sb_16bit    = FALSE; 		   /* in 16 bit mode? */
static int sb_stereo   = FALSE; 		   /* in stereo mode? */
static int sb_type	   = 0;

static volatile DWORD sb_interrupt_driven_dma_position = 0;
static DWORD sb_cursor_offset = 0;
static int sb_samples_per_interrupt = 64;
static int sb_current_address = 0;


#define SBTYPE_SB100   0x100
#define SBTYPE_SB201   0x201
#define SBTYPE_SBPRO   0x300
#define SBTYPE_SB16    0x400

static void _sbpro_master_volume(int d0);
static void _sb_voice(int state);
static int get_blaster(void);
static int sb_read_dsp_version(void);
static int sb_write_dsp(unsigned char byte);
static void sb_stereo_mode(int enable);
static void sb_set_sample_rate(unsigned int rate);
static int sb_interrupt(void);
static void sb_play_buffer(int size);
static void sb_stop(void);
static int _sb_reset_dsp(int);
static int sb_read_dsp(void);
static void _sb_dac_level(BYTE);
static void _sb16_dac_level(BYTE);
static void sb16_get_dma_and_irq(void);
static void wdm_sbpro_interrupt(void);


static int wdm_sbpro_start(int rate_no);
static int sb201_start(int rate_no);
static int sbpro_start(int rate_no);
static int sbpro_ex_start(int rate_no);
static int sb16_start(int rate_no);
static int sb16_ex_start(int rate_no);
static int sb_auto_detect_start(int rate_no);
static int sb201_interrupt_driven_start(int rate_no);
static int sb100_interrupt_driven_start(int rate_no);



/********************************************************************
	SoundBlaster helper funtions
 ********************************************************************/


/*	from Allegro sb.c  */
static int sb_read_dsp_version()
{
	int sb_hw_dsp_ver = -1;
	int x, y;

	if (wd.isa_port > 0){
		if (_sb_reset_dsp(1) != 0) {
			sb_hw_dsp_ver = -1;
		}
		else {
			sb_write_dsp(0xE1);
			x = sb_read_dsp();
			y = sb_read_dsp();
			sb_hw_dsp_ver = ((x << 8) | y);
		}
	}
	return sb_hw_dsp_ver;
}

static void _sbpro_master_volume(int d0)
{
/* d0 can be 0 to 7 */

	d0 = d0 << 1;
	d0 = d0 | (d0 << 4) | 0x11;
	outportb(wd.isa_port + 4, 0x22);
	outportb(wd.isa_port + 5, (BYTE)d0);
}


static void _sb_dac_level(BYTE d0)
{
	outportb(wd.isa_port + 4, 4);
	outportb(wd.isa_port + 5, d0);
}

static void _sb16_dac_level(BYTE d0)
{
	outportb(wd.isa_port + 4, 0x32);		 // DAC level left
	outportb(wd.isa_port + 5, d0 & 0xFF);
	outportb(wd.isa_port + 4, 0x33);		 // DAC level right
	outportb(wd.isa_port + 5, d0 & 0xFF);
}



static void _sb_voice(int state)
{
	if (state == TRUE) {
		sb_write_dsp(0xD1);
	}else{
		sb_write_dsp(0xD3);
	}
}





static int get_blaster(void)
{
	char *wsscfg = getenv("BLASTER");
	if (wsscfg != NULL) {
		while (*wsscfg) {
			while ((*wsscfg == ' ') || (*wsscfg == '\t')){
				wsscfg++;
			}
			if (*wsscfg) {
				switch (*wsscfg) {
					case 'a': case 'A':
						wd.isa_port = strtol(wsscfg+1, NULL, 16);
						break;

					case 'i': case 'I':
						wd.irq = strtol(wsscfg+1, NULL, 10);
						if(wd.irq == 2) wd.irq = 9;
						break;

					case 'd': case 'D':
						wd.isa_dma = strtol(wsscfg+1, NULL, 10);
						break;

					case 'h': case 'H':
						wd.isa_hdma = strtol(wsscfg+1, NULL, 10);
						break;
				}
				while ((*wsscfg) && (*wsscfg != ' ') && (*wsscfg != '\t'))
					wsscfg++;
			}
		}
	}


	if ((wd.isa_port < 0) || (wd.irq < 0) || (wd.isa_dma < 0))
		return FALSE;
	if(wd.isa_hdma < 0)
		wd.isa_hdma = wd.isa_dma;

	return TRUE;
}

static int sb_write_dsp(unsigned char byte)
{
   int x;

   for (x=0; x<0xFFFF; x++) {
	  if (!(inportb(0x0C + wd.isa_port) & 0x80)) {
	 outportb(0x0C + wd.isa_port, byte);
	 return 0;
	  }
   }
   return -1;
}


static void sb_play_buffer(int size)
{
	if (sb_type == SBTYPE_SB201 || sb_type == SBTYPE_SBPRO){	/* 8 bit high-speed auto-initialised */
		sb_write_dsp(0x48);
		sb_write_dsp((size-1) & 0xFF);
		sb_write_dsp((size-1) >> 8);
		sb_write_dsp(0x90);
	}else if(sb_type == SBTYPE_SB100){
		sb_write_dsp(0x14);
		sb_write_dsp((size-1) & 0xFF);
		sb_write_dsp((size-1) >> 8);
	}else if(sb_type == SBTYPE_SB16){						  /* 16 bit */
		size /= 2;
		sb_write_dsp(0xB6);
		sb_write_dsp(0x30);
		sb_write_dsp((size-1) & 0xFF);
		sb_write_dsp((size-1) >> 8);
	}
}

static void sb_stereo_mode(int enable)
{
	BYTE d0;

	outportb(wd.isa_port+0x04, 0x0E);
	d0 = (enable == TRUE) ? 2 : 0;	   // stereo or mono
	d0 = d0 | 0x20; 					// sbpro output filter off
	outportb(wd.isa_port+0x05, d0);
}

static void sb_set_sample_rate(unsigned int rate)
{
	if (sb_type == SBTYPE_SB16){
		sb_write_dsp(0x41);
		sb_write_dsp(rate >> 8);
		sb_write_dsp(rate & 0xff);
	}else{
		if(sb_stereo == TRUE)
			rate *= 2;
		sb_write_dsp(0x40);
		sb_write_dsp((unsigned char)(256 - (1000000 / rate)));
	}
}


static int sb_interrupt(void)
{
   if (sb_16bit == TRUE)							 /* acknowledge SB */
	  inportb(wd.isa_port+0x0F);
   else
	  inportb(wd.isa_port+0x0E);

   eoi(wd.irq); 						/* acknowledge interrupt */
   return 0;
}

static void sb_exit(void)
{
	sb_stop();
	_dma_stop(wd.isa_dma);
	mask_irq(wd.irq);
	restore_irq_handler();

	free_dosmem64k_for_dma();
	wavedevice_struct_init();
}


static void sb_stop(void)
{
   /* halt sound output */
   _sb_voice(FALSE);

	if(sb_type == SBTYPE_SB16){
		sb_write_dsp(0xD5);
		sb_write_dsp(0xD9);
		sb_write_dsp(0xD5);
	}

   _sb_reset_dsp(1);
}


static int _sb_reset_dsp(int d0)
{
	int val;
	int cwait;
	int result;
	int d7;

	result = -1;
	cwait = 8;
	while(cwait <= 1024){
		w_enter_critical();
		outportb(0x06 + wd.isa_port, d0);
		d7 = 0;
		while(d7 < cwait){
			inportb(0x06 + wd.isa_port);
			d7 += 1;
		}
		outportb(0x06 + wd.isa_port, 0);
		w_exit_critical();
		for(d7 = 0 ; d7 < 4 ; d7 += 1){
			val = sb_read_dsp();
			if(val == -1) continue;
			if(val == 0xAA){
				result = 0;
				break;
			}
			break;
		}
		if(result == 0) break;
		cwait = cwait * 2;
	}
	return result;
}

static int sb_read_dsp(void)
{
   int x;

   for (x=0; x<0xFFFF; x++)
	  if (inportb(0x0E + wd.isa_port) & 0x80)
	 return inportb(0x0A + wd.isa_port);

   return -1;
}


static void sb16_get_dma_and_irq(void)
{
	int d0;

	outportb(wd.isa_port + 4, 0x81);
	d0 = inportb(wd.isa_port + 5);
	if(d0 & 0x01) wd.isa_dma = 0;
	if(d0 & 0x02) wd.isa_dma = 1;
	if(d0 & 0x04) wd.isa_dma = 2;
	if(d0 & 0x08) wd.isa_dma = 3;
	if(d0 & 0x10) wd.isa_hdma = 4;
	if(d0 & 0x20) wd.isa_hdma = 5;
	if(d0 & 0x40) wd.isa_hdma = 6;
	if(d0 & 0x80) wd.isa_hdma = 7;

	outportb(wd.isa_port + 4, 0x80);
	d0 = inportb(wd.isa_port + 5);
	if(d0 & 0x01) wd.irq = 9;
	if(d0 & 0x02) wd.irq = 5;
	if(d0 & 0x04) wd.irq = 7;
	if(d0 & 0x08) wd.irq = 10;
}





/********************************************************************
	SB1.0 interrupt driven driver
 ********************************************************************/

static unsigned long sb100_get_next_addr(void)
{
	unsigned long d0;

	d0 = g_wss_dma_addr + sb_current_address;
	sb_current_address += sb_samples_per_interrupt;
	sb_current_address &= SAMPLECNTMASK;

	return d0;
}

static int sb100_interrupt_driven_interrupt(void)
{
	inportb(wd.isa_port+0x0E);
	eoi(wd.irq);						 /* acknowledge interrupt */

	_dma_start(wd.isa_dma, sb100_get_next_addr(), sb_samples_per_interrupt, FALSE, FALSE);
	sb_interrupt_driven_dma_position  = SAMPLECNT - sb_current_address - 1;
	sb_interrupt_driven_dma_position -= sb_cursor_offset;
	sb_interrupt_driven_dma_position &= SAMPLECNTMASK;

	sb_play_buffer(sb_samples_per_interrupt);


	return 0;
}

static DWORD sb100irq_current_pos(void)
{
	static DWORD dmac_sbirq_prev = 0;
	static int ticker_flag = 0;
	static cycles_t prev = 0;
	DWORD d0;

	d0 = sb_interrupt_driven_dma_position;
	if(ticker_flag == 0){
		if(d0 == dmac_sbirq_prev){
			ticker_flag = 1;
			prev = osd_cycles();
		}else{
			dmac_sbirq_prev = d0;
		}
	}else{
		if(d0 == dmac_sbirq_prev){
			if( (osd_cycles() - prev) > (osd_cycles_per_second() / 120) ){
				ticker_flag = 0;
				w_enter_critical();
				sb100_interrupt_driven_interrupt();
				w_exit_critical();
			}
		}else{
			dmac_sbirq_prev = d0;
			ticker_flag = 0;
		}
	}
	return d0;
}


static int sb100_interrupt_driven_start(int rate)
{
	static char device_name_sb100_irq[] = "Sound Blaster 1.0 (interrupt driven)";
	int ver = -1;

	if(rate <= 11111){
		rate = 11111;
		sb_samples_per_interrupt = 16;
	}else{
		rate = 22222;
		sb_samples_per_interrupt = 32;
	}

	if( get_blaster() == FALSE ){
		set_error_message("BLASTER not found.\n");
		return FALSE;
	}

	sb_16bit  = FALSE;
	sb_stereo = FALSE;
	sb_type   = SBTYPE_SB100;

	if(_sb_reset_dsp(1) != 0){
		set_error_message("sb_reset_dsp failed.\n");
		return FALSE;
	}

	ver = sb_read_dsp_version();
	if(ver < 0){
		set_error_message("sb_read_dsp_ver failed.\n");
		return FALSE;
	}

	if(allocate_dosmem64k_for_dma(_8BITMONO) == FALSE){
		set_error_message("allocate_dosmem64k_for_dma() error.\n");
		return FALSE;
	}
	unmask_irq(wd.irq);
	install_irq_handler(wd.irq, (void*)sb100_interrupt_driven_interrupt);
	_sb_voice(TRUE);
//	  sb_stereo_mode(sb_stereo);
	sb_set_sample_rate(rate);


	sb_current_address = 0;
	_dma_start(wd.isa_dma, sb100_get_next_addr(), sb_samples_per_interrupt, FALSE, FALSE);
	sb_interrupt_driven_dma_position = SAMPLECNT - sb_current_address - 1;
	sb_interrupt_driven_dma_position -= sb_cursor_offset;
	sb_interrupt_driven_dma_position &= SAMPLECNTMASK;
	sb_play_buffer(sb_samples_per_interrupt);

	if(ver >= 0x300){
		_sb_dac_level(0xFF);
	}

	wd.device_name	   = device_name_sb100_irq;
	wd.playback_rate   = rate;
	wd.pcm_format	   = _8BITMONO;
	wd.device_exit	   = sb_exit;
	wd.pcm_upload	   = common_pcm_upload_func;
	wd.get_current_pos = sb100irq_current_pos;
	wd.initialized = TRUE;

	return TRUE;
}


/********************************************************************
	SB2.01 interrupt driven driver
 ********************************************************************/

static DWORD sb201irq_current_pos(void)
{
	return sb_interrupt_driven_dma_position;
}

static int sb201_interrupt_driven_interrupt(void)
{
	sb_interrupt_driven_dma_position -= sb_samples_per_interrupt;
	sb_interrupt_driven_dma_position &= (SAMPLECNT - 1);

	inportb(wd.isa_port+0x0E);
	eoi(wd.irq);						 /* acknowledge interrupt */
	return 0;
}

static int sb201_interrupt_driven_start(int rate)
{
	static char device_name_sb201_irq[] = "Sound Blaster 2.01 (interrupt driven)";
	int ver  = -1;

	if(rate <= 11111){
		rate = 11111;
		sb_samples_per_interrupt = 32;
	}else if(rate <= 21739){
		rate = 21739;
		sb_samples_per_interrupt = 64;
	}else if(rate <= 22727){
		rate = 22727;
		sb_samples_per_interrupt = 64;
	}else{
		rate = 45454;
		sb_samples_per_interrupt = 128;
	}

	if( get_blaster() == FALSE ){
		set_error_message("BLASTER not found.\n");
		return FALSE;
	}

	sb_16bit  = FALSE;
	sb_stereo = FALSE;
	sb_type   = SBTYPE_SB201;

	if(_sb_reset_dsp(1) != 0){
		set_error_message("sb_reset_dsp failed.\n");
		return FALSE;
	}

	ver = sb_read_dsp_version();
	if(ver < 0){
		set_error_message("sb_read_dsp_ver failed.\n");
		return FALSE;
	}

	if(allocate_dosmem64k_for_dma(_8BITMONO) == FALSE){
		set_error_message("allocate_dosmem64k_for_dma() error.\n");
		return FALSE;
	}
	unmask_irq(wd.irq);
	install_irq_handler(wd.irq, (void*)sb201_interrupt_driven_interrupt);
	_sb_voice(TRUE);
//	  sb_stereo_mode(sb_stereo);
	sb_set_sample_rate(rate);

	_dma_start(wd.isa_dma, g_wss_dma_addr, g_dma_buff_size, TRUE, FALSE);

	sb_interrupt_driven_dma_position  = SAMPLECNT - 1;
	sb_interrupt_driven_dma_position -= sb_cursor_offset;
	sb_interrupt_driven_dma_position -= sb_samples_per_interrupt;
	sb_interrupt_driven_dma_position &= (SAMPLECNT - 1);

	sb_play_buffer(sb_samples_per_interrupt);

	if(ver >= 0x300){
		_sb_dac_level(0xFF);
	}

	wd.device_name	   = device_name_sb201_irq;
	wd.playback_rate   = rate;
	wd.pcm_format	   = _8BITMONO;
	wd.device_exit	   = sb_exit;
	wd.pcm_upload	   = common_pcm_upload_func;
	wd.get_current_pos = sb201irq_current_pos;
	wd.initialized = TRUE;

	return TRUE;
}



/********************************************************************
	SBPro interrupt driven driver
 ********************************************************************/

static int sbpro_interrupt_driven_interrupt(void)
{
	sb_interrupt_driven_dma_position -= (sb_samples_per_interrupt * 2);
	sb_interrupt_driven_dma_position &= ((SAMPLECNT * 2) - 1);

	inportb(wd.isa_port+0x0E);
	eoi(wd.irq);						 /* acknowledge interrupt */
	return 0;
}

static int sbpro_interrupt_driven_start(int rate)
{
	static char device_name_sbpro_irq[] = "Sound Blaster Pro (interrupt driven)";
	int ver  = -1;

	if(rate <= 11111){
		rate = 11111;
		sb_samples_per_interrupt = 32;
	}else if(rate <= 21739){
		rate = 21739;
		sb_samples_per_interrupt = 64;
	}else{
		rate = 22727;
		sb_samples_per_interrupt = 64;
	}

	if( get_blaster() == FALSE ){
		set_error_message("BLASTER not found.\n");
		return FALSE;
	}

	sb_16bit  = FALSE;
	sb_stereo = TRUE;
	sb_type   = SBTYPE_SBPRO;

	if(_sb_reset_dsp(1) != 0){
		set_error_message("sb_reset_dsp failed.\n");
		return FALSE;
	}

	ver = sb_read_dsp_version();
	if(ver < 0){
		set_error_message("sb_read_dsp_ver failed.\n");
		return FALSE;
	}

	if(allocate_dosmem64k_for_dma(_8BITSTEREO) == FALSE){
		set_error_message("allocate_dosmem64k_for_dma() error.\n");
		return FALSE;
	}
	unmask_irq(wd.irq);
	install_irq_handler(wd.irq, (void*)sbpro_interrupt_driven_interrupt);
	_sb_voice(TRUE);
	sb_stereo_mode(sb_stereo);
	sb_set_sample_rate(rate);

	_dma_start(wd.isa_dma, g_wss_dma_addr, g_dma_buff_size, TRUE, FALSE);

	sb_interrupt_driven_dma_position  = (SAMPLECNT * 2) - 1;
	sb_interrupt_driven_dma_position -= (sb_cursor_offset * 2);
	sb_interrupt_driven_dma_position -= (sb_samples_per_interrupt * 2);
	sb_interrupt_driven_dma_position &= ((SAMPLECNT * 2) - 1);

	sb_play_buffer(sb_samples_per_interrupt * 2);

	if(ver >= 0x300){
		_sb_dac_level(0xFF);
	}

	wd.device_name	   = device_name_sbpro_irq;
	wd.playback_rate   = rate;
	wd.pcm_format	   = _8BITSTEREO;
	wd.device_exit	   = sb_exit;
	wd.pcm_upload	   = common_pcm_upload_func;
	wd.get_current_pos = sb201irq_current_pos;
	wd.initialized = TRUE;

	return TRUE;
}


/********************************************************************
	SBPro interrupt driven driver for WDM SBPro emulation device
 ********************************************************************/

static volatile int wdm_sbpro_init_flag;
static volatile int wdm_sbpro_ticks_index;
static volatile cycles_t wdm_sbpro_average_tick_per_interrupt;
static volatile cycles_t wdm_sbpro_current_time;
static volatile cycles_t wdm_sbpro_ticks[16];
static volatile cycles_t wdm_sbpro_prev_tick;
static volatile cycles_t wdm_sbpro_virtual_tick;
static volatile cycles_t wdm_sbpro_tick_correction;

static volatile DWORD wdm_sbpro_virtual_dma_position = 0;
static volatile int   wdm_sbpro_delta = 0;

#define WDM_SBPRO_SAMPLES_PER_INTERRUPT 	1024

static DWORD sbprowdm_current_pos(void)
{
	cycles_t ticktemp;
	DWORD d0;

	w_enter_critical();
	ticktemp = osd_cycles() - wdm_sbpro_virtual_tick + wdm_sbpro_tick_correction;
	d0	= (ticktemp * (sb_samples_per_interrupt*2 - wdm_sbpro_delta/64))
							/ wdm_sbpro_average_tick_per_interrupt;
	d0	= wdm_sbpro_virtual_dma_position - d0;
	d0 &= ((SAMPLECNT * 2) - 1);
	w_exit_critical();

	return d0;
}


static void wdm_sbpro_interrupt(void)
{
	cycles_t ticktemp;
	int d0;

	wdm_sbpro_current_time = osd_cycles();
	sb_interrupt_driven_dma_position -= (sb_samples_per_interrupt * 2);
	sb_interrupt_driven_dma_position &= ((SAMPLECNT * 2) - 1);

	if(wdm_sbpro_init_flag != 0){
		if(wdm_sbpro_init_flag == 1){
			wdm_sbpro_prev_tick = wdm_sbpro_current_time;
			wdm_sbpro_ticks_index = 0;
			wdm_sbpro_init_flag += 1;
		}else{
			wdm_sbpro_ticks[wdm_sbpro_ticks_index]
								= wdm_sbpro_current_time - wdm_sbpro_prev_tick;
			wdm_sbpro_ticks_index = (wdm_sbpro_ticks_index + 1) & 0x0F;
			wdm_sbpro_prev_tick = wdm_sbpro_current_time;
			wdm_sbpro_virtual_tick=wdm_sbpro_prev_tick;

			if(wdm_sbpro_init_flag == (16 + 1)){
				wdm_sbpro_average_tick_per_interrupt = 0;
				d0 = 0;
				while(d0 < 16){
					wdm_sbpro_average_tick_per_interrupt += wdm_sbpro_ticks[d0];
					d0 += 1;
				}
				wdm_sbpro_average_tick_per_interrupt /= 16;
				if(wdm_sbpro_average_tick_per_interrupt == 0)
					wdm_sbpro_average_tick_per_interrupt = 1;
				wdm_sbpro_virtual_dma_position = sb_interrupt_driven_dma_position;
				wdm_sbpro_delta = 0;
				wdm_sbpro_init_flag = 0;		 // finished
			}else{
				wdm_sbpro_init_flag += 1;
			}
		}
	}else{
		ticktemp = wdm_sbpro_current_time - wdm_sbpro_virtual_tick + wdm_sbpro_tick_correction;
		d0	= (ticktemp * (sb_samples_per_interrupt*2 - wdm_sbpro_delta/64))
								/ wdm_sbpro_average_tick_per_interrupt;
		wdm_sbpro_virtual_dma_position -= d0;
		wdm_sbpro_virtual_dma_position &= ((SAMPLECNT * 2) - 1);
		wdm_sbpro_delta += d0 - (sb_samples_per_interrupt * 2);

		wdm_sbpro_ticks[wdm_sbpro_ticks_index]
							= wdm_sbpro_current_time - wdm_sbpro_prev_tick;
		wdm_sbpro_ticks_index = (wdm_sbpro_ticks_index + 1) & 0x0F;
		wdm_sbpro_average_tick_per_interrupt = 0;
		d0 = 0;
		while(d0 < 16){
			wdm_sbpro_average_tick_per_interrupt += wdm_sbpro_ticks[d0];
			d0 += 1;
		}
		wdm_sbpro_average_tick_per_interrupt /= 16;
		if(wdm_sbpro_average_tick_per_interrupt == 0)
			wdm_sbpro_average_tick_per_interrupt = 1;
		wdm_sbpro_virtual_tick += wdm_sbpro_average_tick_per_interrupt;
		wdm_sbpro_prev_tick = wdm_sbpro_current_time;
		wdm_sbpro_virtual_tick += (wdm_sbpro_prev_tick - wdm_sbpro_virtual_tick) / 64;
		wdm_sbpro_tick_correction = wdm_sbpro_virtual_tick - wdm_sbpro_prev_tick;
	}

	inportb(wd.isa_port+0x0E);
	eoi(wd.irq);						 /* acknowledge interrupt */
	return;
}


static int wdm_sbpro_start(int rate)
{
	static char device_name_sbpro_wdm[] = "Sound Blaster Pro for WDM SBPro emulation device";

	int ver  = -1;

	if(rate <= 21739){
		rate = 21739;
	}else{
		rate = 22050;
	}

	sb_samples_per_interrupt = WDM_SBPRO_SAMPLES_PER_INTERRUPT;

	wd.playback_rate = rate;
	wd.pcm_format = _8BITSTEREO;

	if( get_blaster() == FALSE ){
		set_error_message("BLASTER not found.\n");
		return FALSE;
	}

	sb_16bit  = FALSE;
	sb_stereo = TRUE;
	sb_type   = SBTYPE_SBPRO;

	if(_sb_reset_dsp(1) != 0){
		set_error_message("sb_reset_dsp failed.\n");
		return FALSE;
	}

	ver = sb_read_dsp_version();
	if(ver < 0){
		set_error_message("sb_read_dsp_ver failed.\n");
		return FALSE;
	}

	if(allocate_dosmem64k_for_dma(_8BITSTEREO) == FALSE){
		set_error_message("allocate_dosmem64k_for_dma() error.\n");
		return FALSE;
	}
	unmask_irq(wd.irq);
	install_irq_handler(wd.irq, (void*)wdm_sbpro_interrupt);
	_sb_voice(TRUE);
	sb_stereo_mode(sb_stereo);
	sb_set_sample_rate(rate);

	_dma_start(wd.isa_dma, g_wss_dma_addr, g_dma_buff_size, TRUE, FALSE);

	sb_interrupt_driven_dma_position  = (SAMPLECNT * 2) - 1;
	sb_interrupt_driven_dma_position -= (sb_cursor_offset * 2);
	sb_interrupt_driven_dma_position += 512;
//	  sb_interrupt_driven_dma_position -= (sb_samples_per_interrupt * 2);
	sb_interrupt_driven_dma_position &= ((SAMPLECNT * 2) - 1);

	if(ver >= 0x300){
		_sb_dac_level(0xFF);
	}

	wdm_sbpro_init_flag = 1;
	sb_play_buffer(sb_samples_per_interrupt * 2);
	while(1){
		if(wdm_sbpro_init_flag == 0) break;
	}

	wd.device_name	   = device_name_sbpro_wdm;
	wd.playback_rate   = rate;
	wd.pcm_format	   = _8BITSTEREO;
	wd.device_exit	   = sb_exit;
	wd.pcm_upload	   = common_pcm_upload_func;
	wd.get_current_pos = sbprowdm_current_pos;
	wd.initialized = TRUE;


	return TRUE;
}


/********************************************************************
	SB2.01 driver
 ********************************************************************/

static int sb201_start(int rate)
{
	static char device_name_sb201[] = "Sound Blaster 2.01";
	int ver  = -1;

	if(rate <= 11111){
		rate = 11111;
	}else if(rate <= 21739){
		rate = 21739;
	}else if(rate <= 22727){
		rate = 22727;
	}else if(rate <= 32258){
		rate = 32258;
	}else{
		rate = 45454;
	}

	if( get_blaster() == FALSE ){
		set_error_message("BLASTER not found.\n");
		return FALSE;
	}

	sb_16bit  = FALSE;
	sb_stereo = FALSE;
	sb_type   = SBTYPE_SB201;

	if(_sb_reset_dsp(1) != 0){
		set_error_message("sb_reset_dsp failed.\n");
		return FALSE;
	}

	ver = sb_read_dsp_version();
	if(ver < 0){
		set_error_message("sb_read_dsp_ver failed.\n");
		return FALSE;
	}

	if(allocate_dosmem64k_for_dma(_8BITMONO) == FALSE){
		set_error_message("allocate_dosmem64k_for_dma() error.\n");
		return FALSE;
	}
	unmask_irq(wd.irq);
	install_irq_handler(wd.irq, (void*)sb_interrupt);
	_sb_voice(TRUE);
//	  sb_stereo_mode(sb_stereo);
	sb_set_sample_rate(rate);

	_dma_start(wd.isa_dma, g_wss_dma_addr, g_dma_buff_size, TRUE, FALSE);

//	  sb_play_buffer(g_dma_buff_size);
	sb_play_buffer(2048);

	if(ver >= 0x300){
		_sb_dac_level(0xFF);
	}

	wd.device_name	   = device_name_sb201;
	wd.playback_rate   = rate;
	wd.pcm_format	   = _8BITMONO;
	wd.device_exit	   = sb_exit;
	wd.pcm_upload	   = common_pcm_upload_func;
	wd.get_current_pos = common_dma_current_pos;
	wd.initialized = TRUE;

	return TRUE;
}



/********************************************************************
	SBPro driver
 ********************************************************************/

static int sbpro_start(int rate)
{
	static char device_name_sbpro[] = "Sound Blaster Pro";

	if(rate < 0x10000){
		if(rate <= 11111){
			rate = 11111;
		}else if(rate <= 21739){
			rate = 21739;
		}else{
			rate = 22727;
		}
	}else{
		rate = rate - 0x10000;
	}

	if( get_blaster() == FALSE ){
		set_error_message("BLASTER not found.\n");
		return FALSE;
	}

	sb_16bit  = FALSE;
	sb_stereo = TRUE;
	sb_type   = SBTYPE_SBPRO;

	if(_sb_reset_dsp(1) != 0){
		set_error_message("sb_reset_dsp failed.\n");
		return FALSE;
	}
	if(allocate_dosmem64k_for_dma(_8BITSTEREO) == FALSE){
		set_error_message("allocate_dosmem64k_for_dma() error.\n");
		return FALSE;
	}
	unmask_irq(wd.irq);
	install_irq_handler(wd.irq, (void*)sb_interrupt);
	_sb_voice(TRUE);
	sb_stereo_mode(sb_stereo);
	sb_set_sample_rate(rate);

	_dma_start(wd.isa_dma, g_wss_dma_addr, g_dma_buff_size, TRUE, FALSE);

//	  sb_play_buffer(g_dma_buff_size);
	sb_play_buffer(2048);
	_sb_dac_level(0xFF);
	_sbpro_master_volume(7);

	wd.device_name	   = device_name_sbpro;
	wd.playback_rate   = rate;
	wd.pcm_format	   = _8BITSTEREO;
	wd.device_exit	   = sb_exit;
	wd.pcm_upload	   = common_pcm_upload_func;
	wd.get_current_pos = common_dma_current_pos;
	wd.initialized = TRUE;

	return TRUE;
}

static int sbpro_ex_start(int rate)
{
	if(rate <= 11111){
		rate = 11111;
	}else if(rate <= 31250){
		rate = 31250;
	}else if(rate <= 32000){
		rate = 32000;
	}else if(rate <= 45454){
		rate = 45454;
	}else{
		rate = 47619;
	}

	return sbpro_start(rate | 0x10000);
}


/********************************************************************
	SB16 driver
 ********************************************************************/

static int sb16_start(int rate)
{
	static char device_name_sb16[]		= "Sound Blaster 16";

	if(rate < 0x10000){
		if(rate <= 11025){
			rate = 11025;
		}else if(rate <= 22050){
			rate = 22050;
		}else if(rate <= 33075){
			rate = 33075;
		}else if(rate <= 100000){
			rate = 44100;
		}
	}else{
		rate = rate - 0x10000;
	}

	if( get_blaster() == FALSE ){
		set_error_message("BLASTER not found.\n");
		return FALSE;
	}

	wd.isa_dma = wd.isa_hdma;

	sb_16bit  = TRUE;		  // cmd Bxh
	sb_stereo = TRUE;
	sb_type   = SBTYPE_SB16;

	if(_sb_reset_dsp(1) != 0){
		set_error_message("sb_reset_dsp failed.\n");
		return FALSE;
	}
	if(allocate_dosmem64k_for_dma(_16BITSTEREO) == FALSE){
		set_error_message("allocate_dosmem64k_for_dma() error.\n");
		return FALSE;
	}
	unmask_irq(wd.irq);
	install_irq_handler(wd.irq, (void*)sb_interrupt);
	_sb_voice(TRUE);
	sb_set_sample_rate(rate);

	_dma_start(wd.isa_dma, g_wss_dma_addr, g_dma_buff_size, TRUE, FALSE);

//	  sb_play_buffer(g_dma_buff_size);
	sb_play_buffer(2048);
	_sb_dac_level(0xFF);
	_sb16_dac_level(0xFF);

	wd.device_name	   = device_name_sb16;
	wd.playback_rate   = rate;
	wd.pcm_format	   = _16BITSTEREO;
	wd.device_exit	   = sb_exit;
	wd.pcm_upload	   = common_pcm_upload_func;
	wd.get_current_pos = common_dma_current_pos;
	wd.initialized = TRUE;

	return TRUE;
}

static int sb16_ex_start(int rate)
{
	return sb16_start(rate | 0x10000);
}


/********************************************************************
	SoundBlaster auto-detection
 ********************************************************************/

static int sb_auto_detect_start(int rate_no)
{
	int result;
	int ver;

	get_blaster();
	if(wd.isa_port < 0){
		set_error_message("BLASTER not found.\n");
		return FALSE;
	}

	ver = sb_read_dsp_version();
	if(ver < 0){
		set_error_message("sb_read_dsp_ver failed.\n");
		return FALSE;
	}

	if(ver < 0x201){
		result = sb100_interrupt_driven_start(rate_no);
	}else if(0x201 <= ver && ver < 0x300){
		result = sb201_start(rate_no);
	}else if(0x300 <= ver && ver < 0x400){
		result = sbpro_start(rate_no);
	}else{
		sb16_get_dma_and_irq();
		result = sb16_start(rate_no);
	}
	return result;
}


#if 0

/*

	For CMI8x38 users, Use SB interrupt-driven driver instead.

*/

/********************************************************************
	CMI8x38 (SB16 or SBPro) driver
 ********************************************************************/

static int detectCMI8x38(void);

static DWORD cmi8x38_iobase = -1;

static int detectCMI8x38(void)
{
	__dpmi_regs 	r;

	r.d.eax = 0x0000b101;						// PCI BIOS - INSTALLATION CHECK
	r.d.edi = 0x00000000;
	__dpmi_int(0x1a, &r);
	if( r.d.edx != 0x20494350 ){				// ' ICP'
		return FALSE;
	}

	while(1){
		r.d.eax = 0x0000b102;						// PCI BIOS - FIND PCI DEVICE
		r.d.ecx = 0x00000100;						// device ID
		r.d.edx = 0x000013F6;						// vendor ID
		r.d.esi = 0x00000000;						// device index
		__dpmi_int(0x1a, &r);
		if( r.h.ah == 0 ) break;

		r.d.eax = 0x0000b102;						// PCI BIOS - FIND PCI DEVICE
		r.d.ecx = 0x00000101;						// device ID
		r.d.edx = 0x000013F6;						// vendor ID
		r.d.esi = 0x00000000;						// device index
		__dpmi_int(0x1a, &r);
		if( r.h.ah == 0 ) break;

		r.d.eax = 0x0000b102;						// PCI BIOS - FIND PCI DEVICE
		r.d.ecx = 0x00000111;						// device ID
		r.d.edx = 0x000013F6;						// vendor ID
		r.d.esi = 0x00000000;						// device index
		__dpmi_int(0x1a, &r);
		if( r.h.ah == 0 ) break;

		r.d.eax = 0x0000b102;						// PCI BIOS - FIND PCI DEVICE
		r.d.ecx = 0x00000101;						// device ID
		r.d.edx = 0x000010B9;						// vendor ID
		r.d.esi = 0x00000000;						// device index
		__dpmi_int(0x1a, &r);

		if( r.h.ah == 0 ) break;
		r.d.eax = 0x0000b102;						// PCI BIOS - FIND PCI DEVICE
		r.d.ecx = 0x00000111;						// device ID
		r.d.edx = 0x000010B9;						// vendor ID
		r.d.esi = 0x00000000;						// device index
		__dpmi_int(0x1a, &r);
		if( r.h.ah == 0 ) break;

		return FALSE;
	}

	r.d.eax = 0x0000b10a;						// READ CONFIGURATION DWORD
												// BH = bus number
												// BL = device/function number
	r.d.edi = 0x00000010;						// register number
	__dpmi_int(0x1a, &r);
	if( r.h.ah != 0 ){
		return FALSE;
	}
	cmi8x38_iobase = r.d.ecx & ~0xFF;

	return TRUE;
}


static DWORD cmi8x38_current_pos(void)
{
	int limit;
	DWORD d0 = 0;
	DWORD d1;

	limit = 1024;
	w_enter_critical();
	while(limit > 0){
		d0 = inportw(cmi8x38_iobase + 0x1E);
		d1 = inportw(cmi8x38_iobase + 0x1E);
		if(d0 == d1) break;
		limit -= 1;
	}
	w_exit_critical();
	d0 = d0 * 2;

	return d0;
}

static int cmi8x38_as_sb16_start(int rate)
{
	static char device_name_cmi8x38_sb16[]	= "CMI8338/8738 as SB16";
	static char device_name_cmi8x38_sbpro[] = "CMI8338/8738 as SBPro";

	if( detectCMI8x38() == FALSE ){
		set_error_message("CMI8x38 not found.\n");
		return FALSE;
	}
	if( get_blaster() == FALSE ){
		set_error_message("BLASTER not found.\n");
		return FALSE;
	}

	if(sb_read_dsp_version() >= 0x400){
		/*	old version of CMI8x38 is compatible with SB16	*/
		sb16_get_dma_and_irq();
		sb16_ex_start(rate);
		wd.device_name	   = device_name_cmi8x38_sb16;
		wd.get_current_pos = cmi8x38_current_pos;
	}else{
		/* not tested */
		/*	new version of CMI8x38 is compatible with SBPro  */
		sbpro_start(rate);
		wd.device_name	   = device_name_cmi8x38_sbpro;
		wd.get_current_pos = cmi8x38_current_pos;
	}

	return TRUE;
}

#endif


/********************************************************************
	Trident 4DWave(SB16) driver (for safety DMA Counter reading)
	This Driver needs WAVEINIT.EXE for SB16 function.
 ********************************************************************/

static DWORD trid4dwave_iobase = -1;

static int GetSBDelta(int rate)
{
	double f0;

	f0 = (double)rate / 48000;

	return (int)(f0 * (1 << 12));
}

static int GetSampleRate(int sbdelta)
{
	double f0;

	f0 = sbdelta;

	return (int)((f0 * 48000) / (1 << 12));
}

static int SetSBDelta(DWORD iobase, int sbdelta)
{
	outportw(iobase + 0xAC, sbdelta & 0xFFFF);
	return TRUE;
}


static int detectTrid4DWave(void)
{
	__dpmi_regs 	r;

	r.d.eax = 0x0000b101;						// PCI BIOS - INSTALLATION CHECK
	r.d.edi = 0x00000000;
	__dpmi_int(0x1a, &r);
	if( r.d.edx != 0x20494350 ){				// ' ICP'
		return FALSE;
	}

	do{
		r.d.eax = 0x0000b102;						// PCI BIOS - FIND PCI DEVICE
		r.d.ecx = 0x00002000;						// device ID
		r.d.edx = 0x00001023;						// vendor ID
		r.d.esi = 0x00000000;						// device index
		__dpmi_int(0x1a, &r);
		if( r.h.ah == 0 ) break;					// DX found

		r.d.eax = 0x0000b102;						// PCI BIOS - FIND PCI DEVICE
		r.d.ecx = 0x00002001;						// device ID
		r.d.edx = 0x00001023;						// vendor ID
		r.d.esi = 0x00000000;						// device index
		__dpmi_int(0x1a, &r);
		if( r.h.ah == 0 ) break;					// NX found

		r.d.eax = 0x0000B102;						// PCI BIOS - FIND PCI DEVICE
		r.d.ecx = 0x00007018;						// device ID
		r.d.edx = 0x00001039;						// vendor ID
		r.d.esi = 0x00000000;						// device index
		__dpmi_int(0x1a, &r);
		if( r.h.ah == 0 ) break;					// SiS Audio 7018 found

		return FALSE;
	}while(0);

	r.d.eax = 0x0000b10a;						// READ CONFIGURATION DWORD
												// BH = bus number
												// BL = device/function number
	r.d.edi = 0x00000010;						// register number
	__dpmi_int(0x1a, &r);
	if( r.h.ah != 0 ){
		return FALSE;
	}

	trid4dwave_iobase = r.d.ecx & ~0xFF;
	return TRUE;
}



static DWORD trid4dwave_current_pos(void)
{
	int limit;
	DWORD d0;
	DWORD d1;

	limit = 1024;
	w_enter_critical();
	while(limit > 0){
		d0	= inportb(trid4dwave_iobase + 4);
		d0 |= inportb(trid4dwave_iobase + 5) << 8;
		d1	= inportb(trid4dwave_iobase + 4);
		d1 |= inportb(trid4dwave_iobase + 5) << 8;
		if(d0 == d1) break;
		limit -= 1;
	}
	w_exit_critical();

	return d0;
}

static int trid4dwave_as_sb16_start(int rate)
{
	static char device_name_trid4dwave[]  = "Trident 4DWave DX/NX as SB16";
	int rate1;
	int rate2;
	int sbdelta;

	if(rate <= 11025){
		rate = 11025;
	}
	if(rate >= 48000){
		rate = 48000;
	}

	if( detectTrid4DWave() == FALSE ){
		set_error_message("Trident 4DWave not found.\n");
		return FALSE;
	}

	sbdelta = GetSBDelta(rate);
	rate1 = GetSampleRate(sbdelta);
	rate2 = GetSampleRate(sbdelta + 1);
	if(rate > rate1)
		rate1 = rate - rate1;
	else
		rate1 = rate1 - rate;
	if(rate > rate2)
		rate2 = rate - rate2;
	else
		rate2 = rate2 - rate;
	if(rate1 > rate2)
		sbdelta += 1;
	SetSBDelta(trid4dwave_iobase, sbdelta);

	sb16_start(rate | 0x10000);

	wd.device_name	   = device_name_trid4dwave;
	wd.get_current_pos = trid4dwave_current_pos;

	return TRUE;
}








/********************************************************************
	Window Sound System driver
	This driver is based on Allegro liblary.
 ********************************************************************/

static char device_name_soundscape[] = "Ensoniq Soundscape";
static int soundscape_get_init_config(void);
static int soundscape_start(int rate_no);

static void wss_wait(void);
static int wss_start(int rate_no);
static void wss_exit(void);

struct codec_rate_struct
{
   int freq;
   int divider;
};

static struct codec_rate_struct codec_rates[] =
{
   { 5512,	0x01 }, 	//	0
   { 6620,	0x0F }, 	//	1
   { 8000,	0x00 }, 	//	2
   { 9600,	0x0E }, 	//	3
   { 11025, 0x03 }, 	//	4
   { 16000, 0x02 }, 	//	5
   { 18900, 0x05 }, 	//	6
   { 22050, 0x07 }, 	//	7
   { 27420, 0x04 }, 	//	8
   { 32000, 0x06 }, 	//	9
   { 33075, 0x0D }, 	//	10
   { 37800, 0x09 }, 	//	11
   { 44100, 0x0B }, 	//	12
   { 48000, 0x0C }, 	//	13
   { 54857, 0x08 }, 	//	14
   { 64000, 0x0A }		//	15
};


/* CODEC ports */
#define IADDR	(wd.isa_port+4)
#define IDATA	(wd.isa_port+5)
#define STATUS	(wd.isa_port+6)
#define PIO 	(wd.isa_port+7)

#define INIT	0x80
#define MCE 	0x40
#define TRD 	0x20

#define INT 	0x01
#define PRDY	0x02
#define PLR 	0x04
#define PUL 	0x08
#define SER 	0x10
#define CRDY	0x20
#define CLR 	0x40
#define CUL 	0x80

#define LADC	0x00
#define RADC	0x01
#define LAUX1	0x02
#define RAUX1	0x03
#define LAUX2	0x04
#define RAUX2	0x05
#define LDAC	0x06
#define RDAC	0x07
#define FS		0x08
#define INTCON	0x09
#define PINCON	0x0A
#define ERRSTAT 0x0B
#define MODE_ID 0x0C
#define LOOPBCK 0x0D
#define PB_UCNT 0x0E
#define PB_LCNT 0x0F


static int get_wsscfg(void)
{
	char *wsscfg = getenv("WSSCFG");
	if (wsscfg != NULL) {
		while (*wsscfg) {
			while ((*wsscfg == ' ') || (*wsscfg == '\t')){
				wsscfg++;
			}
			if (*wsscfg) {
				switch (*wsscfg) {
					case 'a': case 'A':
						wd.isa_port = strtol(wsscfg+1, NULL, 16);
						break;

					case 'i': case 'I':
						wd.irq = strtol(wsscfg+1, NULL, 10);
						break;

					case 'd': case 'D':
						wd.isa_dma = strtol(wsscfg+1, NULL, 10);
						break;
				}
				while ((*wsscfg) && (*wsscfg != ' ') && (*wsscfg != '\t'))
					wsscfg++;
			}
		}
	}
	if ((wd.isa_port < 0) || (wd.irq < 0) || (wd.isa_dma < 0)) return FALSE;
	return TRUE;
}

static void _wssout(BYTE d0, BYTE d1)
{
	outportb(IADDR, d0);
	outportb(IDATA, d1);
}

static void wss_wait(void)
{
   int i = 0xFFFF;

   while ((inportb(wd.isa_port + 4) & INIT) || (i-- > 0));
}

#if 0
static int wss_irq_handler(void)
{
	outportb(STATUS, 0);
	eoi(wd.irq);
	return 0;
}
#endif



static int wss_start(int rate)
{
	static char device_name_windows_sound_system[] = "Windows Sound System";
	int i;
	int wss_usedrate;
	int wss_stereo;
	int wss_16bits;
	int nsamples;

	if(rate <= 11025){
		rate = 11025;
		wss_usedrate = 4;
	}else if(rate <= 22050){
		rate = 22050;
		wss_usedrate = 7;
	}else if(rate <= 330750){
		rate = 33075;
		wss_usedrate = 10;
	}else if(rate <= 44100){
		rate = 44100;
		wss_usedrate = 12;
	}else if(rate <= 48000){
		rate = 48000;
		wss_usedrate = 13;
	}else{
		rate = 64000;
		wss_usedrate = 15;
	}
	wss_stereo = TRUE;
	wss_16bits = TRUE;
	if( get_wsscfg() == FALSE ){
		set_error_message("WSSCFG not found.\n");
		return FALSE;
	}
	if(allocate_dosmem64k_for_dma(_16BITSTEREO) == FALSE){
		set_error_message("allocate_dosmem64k_for_dma() error.\n");
		return FALSE;
	}

	nsamples = SAMPLECNT;

	_wssout(LADC, 0);
	_wssout(RADC, 0);
	_wssout(LAUX1, 0);
	_wssout(RAUX1, 0);
	_wssout(LAUX2, 0);
	_wssout(RAUX2, 0);
	_wssout(LDAC, 0x80);
	_wssout(RDAC, 0x80);
	_wssout(LOOPBCK, 0);
	_wssout(MODE_ID, 0x8A);

	/* Enable MCE */
	outportb(IADDR, MCE | INTCON);
	wss_wait();

	outportb(IDATA, 0x18);
	wss_wait();

	/* Disable MCE */
	outportb(IADDR, ERRSTAT);

	i = 0xFFFF;

	while ((inportb(IDATA) & 0x20) && (i-- > 0));

	if (i < 1)
	   return -1;

	/* Enter MCE */
	outportb(IADDR, MCE | FS);
	wss_wait();

	/* Set playback format */
	i = codec_rates[wss_usedrate].divider;
	if (wss_stereo)
	   i |= 0x10;
	if (wss_16bits)
	   i |= 0x40;
	outportb(IDATA, i);
	wss_wait();

	outportb(IADDR, 0);
	outportb(STATUS, 0);
	outportb(IADDR, PINCON);
	wss_wait();
//	  outportb(IDATA, 0x2);
	outportb(IDATA, 0x00);

//	  unmask_irq(wd.irq);
//	  install_irq_handler(wd.irq, (void*)wss_irq_handler);		// this driver does not need IRQ

	_dma_start(wd.isa_dma, g_wss_dma_addr, g_dma_buff_size, TRUE, FALSE);

	eoi(wd.irq);

	outportb(IADDR, PB_UCNT);
	wss_wait();
	outportb(IDATA, (nsamples - 1) >> 8);
	outportb(IADDR, PB_LCNT);
	wss_wait();
	outportb(IDATA, (nsamples - 1) & 0xFF);
	wss_wait();
	outportb(IADDR, INTCON | MCE);
	wss_wait();
	outportb(IDATA, 0x5);
	outportb(IADDR, 0);

	_wssout(LDAC, 0);
	_wssout(RDAC, 0);

	wd.device_name	   = device_name_windows_sound_system;
	wd.playback_rate   = rate;
	wd.pcm_format	   = _16BITSTEREO;
	wd.device_exit	   = wss_exit;
	wd.pcm_upload	   = common_pcm_upload_func;
	wd.get_current_pos = common_dma_current_pos;
	wd.initialized = TRUE;

	return TRUE;
}

static void wss_exit(void)
{
	/* Mute outputs */
	_wssout(LDAC, 0x80);
	_wssout(RDAC, 0x80);

	_dma_stop(wd.isa_dma);

	/* Stop playback */
	outportb(IADDR, MCE | INTCON);
	wss_wait();
	outportb(IDATA, 0);
	outportb(IADDR, 0);
	outportb(STATUS, 0);
//	  mask_irq(wd.irq);
//	  restore_irq_handler();

	free_dosmem64k_for_dma();
	wavedevice_struct_init();
}


/********************************************************************
	Ultrasound MAX (CS4231 codec) driver
 ********************************************************************/

static int get_ultrasnd(void);

static int get_ultra16(void)
{
	int max_port  = -1;
	int max_pdma  = -1;
	int max_irq   = -1;
	int max_type  = -1;
	int max_cdrom = -1;
	char *wsscfg = getenv("ULTRA16");

	if(get_ultrasnd() == FALSE) return FALSE;

	if (wsscfg != NULL) {
		sscanf(wsscfg, "%x, %d, %d, %d, %d",
			&max_port, &max_pdma, &max_irq, &max_type, &max_cdrom);
	}
	if( (max_port < 0) || (max_pdma < 0) || (max_type < 0) || (max_irq < 0) ) return FALSE;
	if(max_type == 0){
		wd.isa_port = max_port;
		wd.isa_dma	= max_pdma;
		wd.irq	= max_irq;
	}else{
		wd.isa_port = max_port - 4;
		wd.isa_dma	= wd.isa_hdma;
	}
	return TRUE;
}

static int ultramax_start(int rate_no)
{
	static char device_name_ultrasound_max[]  = "Ultrasound Max";

	if(get_ultra16() == FALSE){
		set_error_message("ULTRA16 not found.\n");
		return FALSE;
	}
	if(wss_start(rate_no) == FALSE) return FALSE;
	wd.device_name = device_name_ultrasound_max;
	return TRUE;
}


/********************************************************************
	Ensoniq Soundscape driver
	This driver is based on Allegro liblary.
 ********************************************************************/

static int get_ini_config_entry(char *entry, char *dest, FILE *fp)
{
	char str[83];
	char tokstr[33];
	char *p;

	/* make a local copy of the entry, upper-case it */
	strcpy(tokstr, entry);
	strupr(tokstr);

	/* rewind the file and try to find it... */
	rewind(fp);

	for (;;) {
		/* get the next string from the file */
		fgets(str, 83, fp);
		if (feof(fp)) {
			fclose(fp);
			return -1;
		}

		/* properly terminate the string */
		for (p=str; *p; p++) {
			if (isspace(*p)) {
				*p = 0;
				break;
			}
		}

		/* see if it's an 'equate' string; if so, zero the '=' */
		p = strchr(str, '=');
		if (!p)
			continue;

		*p = 0;

		/* upper-case the current string and test it */
		strupr(str);

		if (strcmp(str, tokstr))
			continue;

		/* it's our string - copy the right-hand value to buffer */
		strcpy(dest, str+strlen(str)+1);
		break;
	}

	return 0;
}

static int soundscape_get_init_config()
{
	FILE *fp = NULL;
	char str[78];
	char *ep;

	/* get the environment var and build the filename, then open it */
	if (!(ep = getenv("SNDSCAPE"))){
		set_error_message("SNDSCAPE not found.");
		return FALSE;
	}
	strcpy(str, ep);

	if (str[strlen(str)-1] == '\\')
		str[strlen(str)-1] = 0;

	strcat(str, "\\SNDSCAPE.INI");

	if (!(fp = fopen(str, "r"))){
		set_error_message("SNDSCAPE.INI not found.");
		return FALSE;
	}
   /* read all of the necessary config info ... */
   if (get_ini_config_entry("Product", str, fp)) {
	  fclose(fp);
	  set_error_message("error in SNDSCAPE.INI.");
	  return FALSE;
   }
#if 0
   /* if an old product name is read, set the IRQs accordingly */
   strupr(str);
   if (strstr(str, "SOUNDFX") || strstr(str, "MEDIA_FX"))
	  soundscape_irqset = rs_irqs;
   else
	  soundscape_irqset = ss_irqs;
   if (get_ini_config_entry("Port", str, fp)) {
	  fclose(fp);
	  return FALSE;
   }

   soundscape_baseport = strtol(str, NULL, 16);
#endif

   if (get_ini_config_entry("WavePort", str, fp)) {
	  fclose(fp);
	  set_error_message("error in SNDSCAPE.INI.");
	  return FALSE;
   }

   wd.isa_port = strtol(str, NULL, 16) - 4;
#if 0
   if (get_ini_config_entry("IRQ", str, fp)) {
	  fclose(fp);
	  return FALSE;
   }

   soundscape_midiirq = strtol(str, NULL, 10);

   if (soundscape_midiirq == 2)
	  soundscape_midiirq = 9;
#endif
   if (get_ini_config_entry("SBIRQ", str, fp)) {
	  fclose(fp);
	  set_error_message("error in SNDSCAPE.INI.");
	  return FALSE;
   }

   wd.irq = strtol(str, NULL, 10);

   if (wd.irq == 2)
	  wd.irq = 9;

   if (get_ini_config_entry("DMA", str, fp)) {
	  fclose(fp);
	  set_error_message("error in SNDSCAPE.INI.");
	  return FALSE;
   }

   wd.isa_dma = strtol(str, NULL, 10);

   fclose(fp);
   return TRUE;
}


static int soundscape_start(int rate_no)
{
	if(soundscape_get_init_config() == FALSE) return FALSE;
	if(wss_start(rate_no) == FALSE) return FALSE;
	wd.device_name = device_name_soundscape;
	return TRUE;
}


/********************************************************************
	Ultrasound (GF1) driver
 ********************************************************************/

static DWORD u_get_current_position(BYTE voice);
static void u_upload(DWORD addr, BYTE data);
static volatile int u_dma_upload(DWORD addr, WORD count, DWORD gusaddr,
					int flag, DWORD addr2, WORD count2, DWORD gusaddr2);
static void u_start_dma_upload(DWORD addr, WORD count, DWORD gusaddr);
static DWORD convert_to_16bit(DWORD address);
static void u_abort_dma_upload(void);


static int u_prim_voice = 0;
static int u_base = -1;
static int u_cmd;
static int u_datal;
static int u_datah;
static int u_page;
static int u_status;
static int u_dram;
static DWORD u_freq_value = 0;
static volatile int u_dma_busy_flag = FALSE;
static volatile int u_twoflag = FALSE;
static DWORD u_nextaddr;
static WORD  u_nextcount;
static DWORD u_nextgusaddr;


static void u_delay(void)
{
	int d0;

	d0 = 0;
	while(d0 < 7){
		inportb(u_dram);
		d0 += 1;
	}
}

static void u_voice_no(BYTE data)
{
	outportb(u_page, data);
}

static void u_cmdwb(BYTE _index, BYTE data)
{
	outportb(u_cmd,   _index);
	outportb(u_datah, data);
}

static BYTE u_cmdrb(BYTE _index)
{
	outportb(u_cmd, _index);
	return inportb(u_datah);
}

static void u_cmdww(BYTE _index, WORD data)
{
	outportb(u_cmd,   _index);
	outportw(u_datal, data);
}
static WORD u_cmdrw(BYTE _index)
{
	outportb(u_cmd,   _index);
	return inportw(u_datal);
}


static BYTE u_irq_status(void)
{
	return inportb(u_status);
}

// pan 0 to 7 to 15
//
static void u_set_pan(BYTE voice, BYTE pan)
{
	u_voice_no(voice);
	u_cmdwb(0x0C, pan);
}

static void u_set_freq(BYTE voice, DWORD freq)
{
	WORD fc;

	fc = (WORD)( ((freq << 9) + (u_freq_value >> 1)) / u_freq_value);
	fc = fc << 1;
	u_voice_no(voice);
	u_cmdww(0x01, fc);
}

static void u_set_freq_stereo(BYTE voice)
{
	WORD fc;

	fc = 2 << 10;
	u_voice_no(voice);
	u_cmdww(0x01, fc);
}

static void u_set_volume(BYTE voice, WORD volume)
{
	u_voice_no(voice);
	u_cmdww(0x09, volume);
	u_delay();
	u_cmdww(0x09, volume);
}

#if 0
static void u_voice_start(BYTE voice, DWORD current, DWORD start, DWORD end,
						  int bits, int loop, int pan, DWORD freq, WORD volume)
{
	BYTE d0;

	u_voice_no(voice);

	u_set_pan(voice, pan);
	u_set_freq(voice, freq);
	u_set_volume(voice, volume);

	current = current << 9;
	start	= start   << 9;
	end 	= end	  << 9;
	u_cmdww(0x0A, (current >> 16) & 0xFFFF);
	u_cmdww(0x0B, current & 0xFFFF);
	u_cmdww(0x02, (start >> 16) & 0xFFFF);
	u_cmdww(0x03, start & 0xFFFF);
	u_cmdww(0x04, (end >> 16) & 0xFFFF);
	u_cmdww(0x05, end & 0xFFFF);

	d0 = 0x00;
	if(bits == 16)	  d0 |= 0x04;
	if(loop == TRUE) d0 |= 0x08;

	u_cmdwb(0x00, d0);
}
#endif

static void u_16bit_stereo_voice_start(DWORD length, WORD volume)
{
	int d0;
	int voice = 0;
	DWORD current;
	DWORD start;
	DWORD end;

	u_prim_voice = voice;

	d0 = 0;
	while(d0 < 2){
		u_voice_no(voice + d0);
		current = d0;
		end 	= ((length + 31) * 2);
		start	= 31*2;
		current = current << 9;
		start	= start   << 9;
		end 	= end	  << 9;
		u_cmdww(0x0A, (current >> 16) & 0xFFFF);
		u_cmdww(0x0B, current & 0xFFFF);
		u_cmdww(0x02, (start >> 16) & 0xFFFF);
		u_cmdww(0x03, start & 0xFFFF);
		u_cmdww(0x04, (end >> 16) & 0xFFFF);
		u_cmdww(0x05, end & 0xFFFF);

		u_set_pan(voice + d0, (d0 == 0) ? 0 : 15);
		u_set_freq_stereo(voice + d0);
		u_set_volume(voice + d0, volume);

		d0 += 1;
	}

	w_enter_critical();
	u_voice_no(voice);
	u_cmdwb(0x00, 0x0C);
	u_voice_no(voice + 1);
	u_cmdwb(0x00, 0x0C);
	w_exit_critical();
}


static void u_voice_stop(BYTE voice)
{
	u_set_volume(voice, 0);
	u_voice_no(voice);
	u_cmdwb(0x00, 0x03);		// turn voice off
}


static DWORD u_get_current_position(BYTE voice)
{
	DWORD d0;
	DWORD d1;
	int d2;

	u_voice_no(voice);
	d2 = 0;
	while(d2 < 1024){
		d0	= (u_cmdrw(0x8A) & 0x1FFF) << 7;
		d0 |= u_cmdrw(0x8B) >> 9;
		d1	= (u_cmdrw(0x8A) & 0x1FFF) << 7;
		d1 |= u_cmdrw(0x8B) >> 9;
		if( (d1 - d0) == 0 || (d1 - d0) == 1 ) break;
		d2 += 1;
	}
	return d1;
}

static void u_upload(DWORD addr, BYTE data)
{
	static BYTE gusdramh = -1;

	if( ((addr >> 16) & 0x0F) != gusdramh ){
		gusdramh = (addr >> 16) & 0x0F;
		u_cmdwb(0x44, gusdramh);
	}
	u_cmdww(0x43, addr & 0xFFFF);
	outportb(u_base + 0x107, data);
}

static void u_set_dma_address(DWORD addr)
{
	if(wd.isa_dma > 3)
		addr = convert_to_16bit(addr);
	u_cmdww(0x42, (addr >> 4) & 0xFFFF);
}


/*	from GUSDK ROCK.C  */
static DWORD convert_to_16bit(DWORD address)
{
	DWORD hold_address;

	hold_address = address;

	/* Convert to 16 translated address. */
	address = address >> 1;

	/* Zero out bit 17. */
	address &= 0x0001ffffL;

	/* Reset bits 18 and 19. */
	address |= (hold_address & 0x000c0000L);

	return(address);
}


static volatile int u_dma_upload(DWORD addr, WORD count, DWORD gusaddr,
					int flag, DWORD addr2, WORD count2, DWORD gusaddr2)
{
	DWORD d0;

	d0 = 0;
	while(d0 < 65536*4){
		if(u_dma_busy_flag == FALSE) break;
		u_delay();
		d0 += 1;
	}
	w_enter_critical();
	if(u_dma_busy_flag == TRUE){
		u_abort_dma_upload();
		u_dma_busy_flag = FALSE;
	}
	w_exit_critical();

	u_twoflag = flag;
	if(u_twoflag == TRUE){
		d0 = gusaddr2 - (gusaddr2 & ~31);
		u_nextgusaddr = gusaddr2 & ~31;
		u_nextaddr	  = addr2  - d0;
		u_nextcount   = count2 + d0;
	}

	d0 = gusaddr - (gusaddr & ~31);
	gusaddr = gusaddr & ~31;
	w_enter_critical();
	u_dma_busy_flag = TRUE;
	u_start_dma_upload(addr - d0, count + d0, gusaddr);
	w_exit_critical();
	return TRUE;
}

static void u_start_dma_upload(DWORD addr, WORD count, DWORD gusaddr)
{
	BYTE d1;

	_dma_start(wd.isa_dma, addr, count, FALSE, FALSE);
	u_set_dma_address(gusaddr);
	d1 = 0x61;
	if(wd.isa_dma > 3) d1 |= 0x04;
	u_cmdwb(0x41, d1);
}

static void u_abort_dma_upload(void)
{
	BYTE d1;

	d1 = 0x60;
	if(wd.isa_dma > 3) d1 |= 0x04;
	u_cmdwb(0x41, d1);
	_dma_stop(wd.isa_dma);
	u_cmdrb(0x41);
}


static int u_gf1_irq_handler(void)
{
	BYTE d0;
	BYTE irq_status;

	tick += 1;

	while(1){
		irq_status = inportb(u_base + 0x006) & 0xE0;
		if(irq_status == 0) break;
		if( (irq_status & 0x80) != 0 ){
			d0 = u_cmdrb(0x41);
			if(u_twoflag == TRUE){
				u_twoflag = FALSE;
				u_start_dma_upload(u_nextaddr, u_nextcount, u_nextgusaddr);
			}else{
				u_dma_busy_flag = FALSE;
			}
		}
		if( (irq_status & 0x60) != 0 ){
			d0 = u_cmdrb(0x8F);
		}
	}
	eoi(wd.irq);
	return 0;
}




static void u_init(int base, DWORD freq)
{
	int voice_no;
	DWORD d0;
	BYTE d1;

	u_base	 = base;
	u_page	 = u_base + 0x102;
	u_cmd	 = u_base + 0x103;
	u_datal  = u_base + 0x104;
	u_datah  = u_base + 0x105;
	u_status = u_base + 0x006;
	u_dram	 = u_base + 0x107;

	u_cmdwb(0x4C, 0);				// reset
	u_delay();
	u_delay();
	u_cmdwb(0x4C, 1);				// enable
	u_delay();
	u_delay();

	u_cmdwb(0x41, 0);				// DMA off
	u_cmdwb(0x45, 0);				// disable TIMER irq
	u_cmdwb(0x49, 0);				// sampling control reg.

	if(freq == 44100){
		u_cmdwb(0x0E, (14-1) | 0xC0);	  // active voice 14 44100Hz
		u_freq_value = 44100;
	}else{
		u_cmdwb(0x0E, (28-1) | 0xC0);	  // active voice 28 22050Hz
		u_freq_value = 22050;
	}
	u_irq_status();

	u_cmdrb(0x41);
	u_cmdrb(0x49);
	u_cmdrb(0x8F);

	voice_no = 0;
	while(voice_no < 32){
		u_voice_stop(voice_no);
		u_voice_no(voice_no);
		u_cmdwb(0x0D, 0x03);		// turn ramp off
		voice_no += 1;
	}

	u_cmdrb(0x41);
	u_cmdrb(0x49);
	u_cmdrb(0x8F);
	u_cmdwb(0x4C, 0x07);
	u_delay();
	u_cmdwb(0x4C, 0x07);

	d1 = 0x00;
	d0 = 0;
	while(d0 < 65536*2){
		u_upload(d0, d1);
		d0 += 1;
	}

	outportb(u_base, 0x08);
}

static int get_ultrasnd(void)
{
	int midiirq;

	char *wsscfg = getenv("ULTRASND");
	if (wsscfg != NULL) {
		sscanf(wsscfg, "%x, %d, %d, %d, %d",
			&wd.isa_port, &wd.isa_dma, &wd.isa_hdma, &wd.irq, &midiirq);
	}
	if( (wd.isa_port < 0) || (wd.isa_dma < 0) || (wd.isa_hdma < 0) || (wd.irq < 0) ) return FALSE;

	return TRUE;
}


static DWORD ultrasound_current_pos(void)
{
	DWORD d0;

	w_enter_critical();
	d0 = (SAMPLECNT*2) - (u_get_current_position(u_prim_voice) - (31*2));
	w_exit_critical();

	return d0;
}


static void ultrasound_pcm_upload_func(void)
{
	int d0;
	long d1;
	WORD d2;
	DWORD d3;
	DWORD start;
	DWORD end;
	DWORD start2;
	DWORD end2;

	if(wd.initialized == FALSE) return;

	// dma upload
	start = -1;
	_farsetsel(_dos_ds);
	d0 = 0;
	while(d0 < g_current_req){
		g_write_cursor = g_write_cursor & SAMPLECNTMASK;
		if(wd.pcm_format == _16BITSTEREO){
			d1	= ((mixing_buff[(d0*2) + 0] * g_master_volume)/256);
			if(d1 >  32767) d1 =  32767;
			if(d1 < -32768) d1 = -32768;
			d3	= d1 & 0xFFFF;
			d1	= ((mixing_buff[(d0*2) + 1] * g_master_volume)/256);
			if(d1 >  32767) d1 =  32767;
			if(d1 < -32768) d1 = -32768;
			d3 |= d1 << 16;
			_farnspokel(g_wss_dma_addr + (g_write_cursor * 4), d3);
		}else if(wd.pcm_format == _8BITSTEREO){
			d1	= ((mixing_buff[(d0*2) + 0] * g_master_volume)/256);
			d2	= get_8bit_pcm_value(d1);
			d1	= ((mixing_buff[(d0*2) + 1] * g_master_volume)/256);
			d2 |= get_8bit_pcm_value(d1) << 8;
			_farnspokew(g_wss_dma_addr + (g_write_cursor * 2), d2);
		}else if(wd.pcm_format == _8BITMONO){
			d1	= ((mixing_buff[(d0*2) + 0] * g_master_volume)/256);
			d1 += ((mixing_buff[(d0*2) + 1] * g_master_volume)/256);
			d1	= d1 / 2;
			d2	= get_8bit_pcm_value(d1);
			_farnspokeb(g_wss_dma_addr + (g_write_cursor * 1), (BYTE)d2);
		}
		if(start == -1) start = g_write_cursor;
		g_write_cursor += 1;
		d0 += 1;
	}
	end = g_write_cursor;
	if(start <= end){
		start = start * 4;
		end   = end * 4;
		u_dma_upload(g_wss_dma_addr + start, end - start, start + (32*4),
				FALSE, 0, 0, 0);
	}else{
		d3 = _farnspeekl(g_wss_dma_addr + (SAMPLECNTMASK * 4) );
		u_upload( (31*4) + 0,  (d3 >>  0) & 0xFF);
		u_upload( (31*4) + 1,  (d3 >>  8) & 0xFF);
		u_upload( (31*4) + 2,  (d3 >> 16) & 0xFF);
		u_upload( (31*4) + 3,  (d3 >> 24) & 0xFF);

		start2 = 0;
		end2 = end * 4;
		start = start * 4;
		end  = SAMPLECNT * 4;
		u_dma_upload(g_wss_dma_addr + start, end - start, start + (32*4),
				TRUE, g_wss_dma_addr + start2, end2 - start2, start2 + (32*4));
	}
}

static void ultrasound_exit(void)
{
	int d0;

	d0 = 0;
	while(d0 < 2){
		u_voice_stop(d0);
		d0 += 1;
	}
	_dma_stop(wd.isa_dma);
	mask_irq(wd.irq);
	restore_irq_handler();
	u_init(wd.isa_port, 44100);

	free_dosmem64k_for_dma();
	wavedevice_struct_init();
}


static int ultrasound_start(int rate)
{
	static char device_name_ultrasound[]  = "Ultrasound";
	DWORD curr;

	if(rate <= 22050){
		rate = 22050;
	}else{
		rate = 44100;
	}

	if( get_ultrasnd() == FALSE ){
		set_error_message("ULTRASND not found.\n");
		return FALSE;
	}
	if(allocate_dosmem64k_for_dma(_16BITSTEREO) == FALSE){
		set_error_message("allocate_dosmem64k_for_dma() error.\n");
		return FALSE;
	}
	g_dmacnt_shift_count = 1;
	u_init(wd.isa_port, rate);
	unmask_irq(wd.irq);
	install_irq_handler(wd.irq, (void*)u_gf1_irq_handler);

	u_16bit_stereo_voice_start(SAMPLECNT, 0xFFFF);

	wd.device_name	   = device_name_ultrasound;
	wd.playback_rate   = rate;
	wd.pcm_format	   = _16BITSTEREO;
	wd.device_exit	   = ultrasound_exit;
	wd.pcm_upload	   = ultrasound_pcm_upload_func;
	wd.get_current_pos = ultrasound_current_pos;
	wd.initialized = TRUE;

	while(1){
		w_enter_critical();
		curr = u_get_current_position(u_prim_voice);
		w_exit_critical();
		if( curr > (32*4) ) break;
	}

	return TRUE;
}


/********************************************************************
	ESS AudioDrive driver
	This driver is based on Allegro liblary.
 ********************************************************************/

static volatile int ess_is_dsp_ready_for_read(void)
{
   return (inportb(0x0E + wd.isa_port) & 0x80);
}

static volatile int ess_read_dsp(void)
{
   int x;

   for (x=0; x<0xffff; x++)
	  if (inportb(0x0E + wd.isa_port) & 0x80)
	 return inportb(0x0A + wd.isa_port);

   return -1;
}

static volatile int ess_write_dsp(unsigned char byte)
{
   int x;

   for (x=0; x<0xffff; x++) {
	  if (!(inportb(0x0C + wd.isa_port) & 0x80)) {
	 outportb(0x0C + wd.isa_port, byte);
	 return 0;
	  }
   }
   return -1;
}

static void ess_set_sample_rate(unsigned int rate)
{
   int tc;
   int divider;

   if (rate > 22094)
	  tc = 256 - 795500/rate;
   else
	  tc = 128 - 397700/rate;

   rate = (rate*9)/20;
   divider = 256 - 7160000/(rate*82);

   ess_write_dsp(0xA1);
   ess_write_dsp(tc);

   ess_write_dsp(0xA2);
   ess_write_dsp(divider);
}


static int ess_read_dsp_version(void)
{
	int ess_hw_ver;

   if (_sb_reset_dsp(1) != 0) {
	  ess_hw_ver = -1;
   }else {
	  int major=0, minor=0;
	  int i;

	  ess_write_dsp(0xE7);

	  for (i=0; i<240; i++) {
	 if (ess_is_dsp_ready_for_read()) {
		if (!major)
		   major = ess_read_dsp();
		else
		   minor = ess_read_dsp();
	 }
	  }

	  if ((major==0x68) && ((minor&0xF0)==0x80)) {
	 if ((minor&0x0F) >= 0xB)
		ess_hw_ver = 0x1868;
	 else if ((minor&0x0F) >= 8)
		ess_hw_ver = 0x1688;
	 else
		ess_hw_ver = 0x0688;

	 return ess_hw_ver;
	  }
   }
   return -1;
}


static void ess_play_buffer(int size)
{
	int value;

	ess_write_dsp(0xA4);
	ess_write_dsp((-size)&0xFF);
	ess_write_dsp(0xA5);
	ess_write_dsp((-size)>>8);

	ess_write_dsp(0xC0);
	ess_write_dsp(0xB8);
	value = ess_read_dsp() | 0x05;
	ess_write_dsp(0xB8);
	ess_write_dsp(value);
}

static void ess_exit(void)
{
	/* halt sound output */
	_sb_voice(FALSE);
	/* stop dma transfer */
	_dma_stop(wd.isa_dma);
	_sb_reset_dsp(1);

	free_dosmem64k_for_dma();
	wavedevice_struct_init();
}


static int ess_start(int rate)
{
	static char device_name_ess_audiodrive[] = "ESS Audiodrive";
	int value;

	if(rate <= 11047){
		rate = 11047;
	}else if(rate <= 22097){
		rate = 22097;
	}else if(rate <= 33145){
		rate = 33145;
	}else if(rate <= 44194){
		rate = 44194;
	}else if(rate <= 49718){
		rate = 49718;
	}else{
		rate = 61192;
	}

	if( get_blaster() == FALSE ){
		set_error_message("BLASTER not found.\n");
		return FALSE;
	}
	if( ess_read_dsp_version() < 0 ){
		set_error_message("ESS Audiodrive not found.\n");
		return FALSE;
	}
	wd.playback_rate = rate;
	wd.pcm_format = _16BITSTEREO;

	if(allocate_dosmem64k_for_dma(_16BITSTEREO) == FALSE){
		set_error_message("allocate_dosmem64k_for_dma() error.\n");
		return FALSE;
	}
	/* switch to AudioDrive extended mode */
	_sb_reset_dsp(0x03);
	ess_write_dsp(0xC6);

	ess_set_sample_rate(rate);

	ess_write_dsp(0xB8);
	ess_write_dsp(0x04);

	ess_write_dsp(0xC0);
	ess_write_dsp(0xA8);
	value = ess_read_dsp() & ~3;

	/* 16 bit stereo */
	value |= 0x01;
	ess_write_dsp(0xA8);
	ess_write_dsp(value);
	ess_write_dsp(0xB6);
	ess_write_dsp(0x00);
	ess_write_dsp(0xB7);
	ess_write_dsp(0x71);
	ess_write_dsp(0xB7);
	ess_write_dsp(0xBC);

	/* demand mode (4 bytes/request) */
	ess_write_dsp(0xB9);
	ess_write_dsp(2);

	ess_write_dsp(0xC0);
	ess_write_dsp(0xB1);
//	  value = (ess_read_dsp() & 0x0F) | 0x50;
	value = (ess_read_dsp() & 0x0F) | 0x10;
	ess_write_dsp(0xB1);
	ess_write_dsp(value);

	ess_write_dsp(0xC0);
	ess_write_dsp(0xB2);
	value = (ess_read_dsp() & 0x0F) | 0x50;
	ess_write_dsp(0xB2);
	ess_write_dsp(value);

	_sb_voice(TRUE);
	_dma_start(wd.isa_dma, g_wss_dma_addr, g_dma_buff_size, TRUE, FALSE);
	ess_play_buffer(g_dma_buff_size);
	_sb_dac_level(0xFF);

	wd.device_name	   = device_name_ess_audiodrive;
	wd.playback_rate   = rate;
	wd.pcm_format	   = _16BITSTEREO;
	wd.device_exit	   = ess_exit;
	wd.pcm_upload	   = common_pcm_upload_func;
	wd.get_current_pos = common_dma_current_pos;
	wd.initialized = TRUE;

	return TRUE;
}


/********************************************************************
	ETERNAL SILENCE (timer device) driver
 ********************************************************************/

static cycles_t es_tick_per_hz = 0;
static cycles_t es_remain = 0;
static cycles_t es_prev = 0;
static DWORD  es_count;

static int eternal_silence_start(int rate_no);
static void eternal_silence_exit(void);
static DWORD eternal_silence_current_pos(void);
static char device_name_eternal_silence[] = "Eternal Silence";

static int eternal_silence_start(int rate)
{
	if(allocate_dosmem64k_for_dma(_16BITSTEREO) == FALSE){
		set_error_message("allocate_dosmem64k_for_dma() error.\n");
		return FALSE;
	}
	es_tick_per_hz = osd_cycles_per_second() / rate;
	es_remain = 0;
	es_prev = osd_cycles();
	es_count = 0;

	wd.device_name	   = device_name_eternal_silence;
	wd.playback_rate   = rate;
	wd.pcm_format	   = _16BITSTEREO;
	wd.device_exit	   = eternal_silence_exit;
	wd.pcm_upload	   = common_pcm_upload_func;
	wd.get_current_pos = eternal_silence_current_pos;
	wd.initialized = TRUE;

	return TRUE;
}

static void eternal_silence_exit(void)
{
	free_dosmem64k_for_dma();
	wavedevice_struct_init();
}


static DWORD eternal_silence_current_pos(void)
{
	cycles_t curr;

	curr = osd_cycles();
	es_remain += curr - es_prev;
	es_prev = curr;
	while(es_remain >= es_tick_per_hz){
		es_remain -= es_tick_per_hz;
		es_count -= 1;
	}
	es_count = es_count & SAMPLECNTMASK;
	return es_count << 2;						// assume _16BITSTEREO
}







/********************************************************************
	"w_" sound system funtions
 ********************************************************************/

void w_set_watermark(float latency, DWORD samples_per_frame)
{
	if(wd.initialized == FALSE) return;

	if(latency < 1.0) latency = 1.0;
	g_latencym = latency;
	g_latency = samples_per_frame * latency;
	g_dma_average_cnt = samples_per_frame << DMA_AVERAGE_SHIFT_COUNT;
	g_samples_per_frame = samples_per_frame;
	w_reset_write_cursor(0);

	w_clear_buffer();
}


void w_set_master_volume(int volume)
{
	g_master_volume = volume;
}




DWORD w_get_current_req(void)
{
	if(wd.initialized == FALSE) return 0;
	return g_current_req;
}

DWORD w_get_next_req(void)
{
	if(wd.initialized == FALSE) return 0;
	return g_next_req;
}


static void calc_next_req(void)
{
	if(wd.initialized == FALSE) return;

	g_dma_remainder += g_dma_average_cnt;
	g_dma_remainder += g_dma_dt;
	g_dma_dt = 0;
	g_current_req = g_next_req;
	g_next_req = g_dma_remainder >> DMA_AVERAGE_SHIFT_COUNT;
	g_dma_remainder = g_dma_remainder & DMA_AVERAGE_MASK;

	if(g_current_req == 0 || g_current_req > SAMPLECNT) g_current_req = 1;
	if(g_next_req	 == 0 ||	g_next_req > SAMPLECNT) g_next_req = 1;
}


void w_reverse_stereo(int flag)
{
	stereo_reverse_flag = (flag) ? TRUE : FALSE;
}


void w_lock_mixing_buffer(int currentsamplecount)
{
	DWORD d0;

	if(wd.initialized == FALSE) return;

	if(currentsamplecount != 0)
		g_current_req = currentsamplecount;

	if(g_current_req > SAMPLECNT)
		g_current_req = SAMPLECNT;
	if(g_current_req == 0)
		g_current_req = 1;

	d0 = 0;
	while(d0 < g_current_req){
		mixing_buff[(d0*2) + 0] = 0;
		mixing_buff[(d0*2) + 1] = 0;
		d0 += 1;
	}
}


void w_mixing_zero(void)
{
	int d1;
	int leftindex  = 0;
	int rightindex = 1;

	if(stereo_reverse_flag == TRUE){
		leftindex  = 1;
		rightindex = 0;
	}

	if(wd.initialized == FALSE) return;
	if(g_current_req > SAMPLECNT) return;

	d1 = 0;
	while(d1 < g_current_req){
		mixing_buff[(d1*2) + leftindex ] = 0;
		mixing_buff[(d1*2) + rightindex] = 0;
		d1 += 1;
	}
}


void w_mixing8(char data[], DWORD length, int leftvol, int rightvol)
{
	DWORD dt;
	DWORD d0;
	int d1;
	long d2;
	int leftindex  = 0;
	int rightindex = 1;

	if(stereo_reverse_flag == TRUE){
		leftindex  = 1;
		rightindex = 0;
	}

	if(wd.initialized == FALSE) return;
	if(g_current_req > SAMPLECNT) return;

	dt = (length << 16) / g_current_req;
	d0 = 0;
	d1 = 0;
	while(d1 < g_current_req){
		d2 = (long)data[d0 >> 16];
		d2 = d2 << 8;
		mixing_buff[(d1*2) + leftindex ] += (d2 * leftvol)	/ 256;
		mixing_buff[(d1*2) + rightindex] += (d2 * rightvol) / 256;
		d0 += dt;
		d1 += 1;
	}
}

void w_mixing(short data[], DWORD length, int leftvol, int rightvol)
{
	DWORD dt;
	DWORD d0;
	int d1;
	long d2;
	int leftindex  = 0;
	int rightindex = 1;

	if(stereo_reverse_flag == TRUE){
		leftindex  = 1;
		rightindex = 0;
	}

	if(wd.initialized == FALSE) return;
	if(g_current_req > SAMPLECNT) return;

	dt = (length << 16) / g_current_req;
	d0 = 0;
	d1 = 0;
	while(d1 < g_current_req){
		d2 = (long)data[d0 >> 16];
		mixing_buff[(d1*2) + leftindex ] += (d2 * leftvol)	/ 256;
		mixing_buff[(d1*2) + rightindex] += (d2 * rightvol) / 256;
		d0 += dt;
		d1 += 1;
	}
}

void w_mixing_stereo(short data[], DWORD length, int leftvol, int rightvol)
{
	DWORD dt;
	DWORD d0;
	int d1;
	long left;
	long right;
	int leftindex  = 0;
	int rightindex = 1;

	if(stereo_reverse_flag == TRUE){
		leftindex  = 1;
		rightindex = 0;
	}

	if(wd.initialized == FALSE) return;
	if(g_current_req > SAMPLECNT) return;

	dt = (length << 16) / g_current_req;
	d0 = 0;
	d1 = 0;
	while(d1 < g_current_req){
		left  = (long)data[((d0 >> 16) * 2)];
		right = (long)data[((d0 >> 16) * 2) + 1];
		mixing_buff[(d1*2) + leftindex ] += (left  * leftvol)  / 256;
		mixing_buff[(d1*2) + rightindex] += (right * rightvol) / 256;
		d0 += dt;
		d1 += 1;
	}
}


static void common_pcm_upload_func(void)
{
	int d0;
	long d1;
	WORD d2;
	DWORD d3;
	DWORD start;
	DWORD end;

	start = -1;
	_farsetsel(_dos_ds);
	d0 = 0;
	while(d0 < g_current_req){
		g_write_cursor = g_write_cursor & SAMPLECNTMASK;
		if(wd.pcm_format == _16BITSTEREO){
			d1	= ((mixing_buff[(d0*2) + 0] * g_master_volume)/256);
			if(d1 >  32767) d1 =  32767;
			if(d1 < -32768) d1 = -32768;
			d3	= d1 & 0xFFFF;
			d1	= ((mixing_buff[(d0*2) + 1] * g_master_volume)/256);
			if(d1 >  32767) d1 =  32767;
			if(d1 < -32768) d1 = -32768;
			d3 |= d1 << 16;
			_farnspokel(g_wss_dma_addr + (g_write_cursor * 4), d3);
		}else if(wd.pcm_format == _8BITSTEREO){
			d1	= ((mixing_buff[(d0*2) + 0] * g_master_volume)/256);
			d2	= get_8bit_pcm_value(d1);
			d1	= ((mixing_buff[(d0*2) + 1] * g_master_volume)/256);
			d2 |= get_8bit_pcm_value(d1) << 8;
			_farnspokew(g_wss_dma_addr + (g_write_cursor * 2), d2);
		}else if(wd.pcm_format == _8BITMONO){
			d1	= ((mixing_buff[(d0*2) + 0] * g_master_volume)/256);
			d1 += ((mixing_buff[(d0*2) + 1] * g_master_volume)/256);
			d1	= d1 / 2;
			d2	= get_8bit_pcm_value(d1);
			_farnspokeb(g_wss_dma_addr + (g_write_cursor * 1), (BYTE)d2);
		}
		if(start == -1) start = g_write_cursor;
		g_write_cursor += 1;
		d0 += 1;
	}
	end = g_write_cursor;
}



void w_unlock_mixing_buffer(void)
{
	if(wd.pcm_upload != NULL){
		(*wd.pcm_upload)();
	}
}


void w_set_sb_cursor_offset(int offset)
{
	sb_cursor_offset = offset & SAMPLECNTMASK;
}


static DWORD common_dma_current_pos(void)
{
	DWORD d0;
	w_enter_critical();
	d0 = _dma_todo(wd.isa_dma);
	w_exit_critical();

	return d0;
}




static DWORD _dma_counter(void)
{
	DWORD d0 = 0;

	if(wd.get_current_pos != NULL){
		d0 = (*wd.get_current_pos)();
	}
	return d0;
}



static DWORD get_played_sample_count(void)
{
	DWORD curr;
	DWORD d0;

	if(wd.initialized == FALSE) return 0;

	curr = w_get_play_cursor();
	d0 = w_calc_distance(curr, g_prev_play_cursor);
	g_prev_play_cursor = curr;

	return d0;
}





DWORD w_get_buffer_size(void)
{
	return SAMPLECNT;
}


DWORD w_get_play_cursor(void)
{
	return ((SAMPLECNT - 1) - (_dma_counter() >> g_dmacnt_shift_count)) & SAMPLECNTMASK;
}

DWORD w_get_write_cursor(void)
{
	return g_write_cursor;
}

void w_set_write_cursor(DWORD position)
{
	g_write_cursor = w_calc_position(position, 0);
}



void w_reset_watermark(void)
{
	w_reset_write_cursor(0);
}

void w_reset_watermark_ex(DWORD samplecount)
{
	w_reset_write_cursor(samplecount);
}

void w_reset_write_cursor(DWORD samplecount)
{
	DWORD current;

	if(wd.initialized == FALSE) return;

	current 		   = w_get_play_cursor();
	g_prev_play_cursor = current;

	if(samplecount <= g_latency){
		samplecount = g_latency - samplecount;
	}else{
		samplecount = 0;
	}
	w_set_write_cursor( w_calc_position(current, samplecount) );

	g_dma_remainder = 0;
	g_dma_dt = 0;
	calc_next_req();
	calc_next_req();
}


DWORD w_calc_distance(DWORD cursor1, DWORD cursor2)
{
	return (cursor1 - cursor2) & SAMPLECNTMASK;
}

DWORD w_calc_position(DWORD cursor1, DWORD cursor2)
{
	return (cursor1 + cursor2) & SAMPLECNTMASK;
}


DWORD w_get_requested_sample_count(void)
{
	DWORD d0;

	d0 = w_get_latency();
	if(d0 > w_get_buffer_size()/2){
		w_reset_watermark();
		return 0;
	}
	if(d0 >= g_latency) return 0;
	return g_latency - d0;
}


DWORD w_get_latency(void)
{
	DWORD d0;

	if(wd.initialized == FALSE){
		d0 = 0;
	}else{
		d0 = w_calc_distance(w_get_write_cursor(), w_get_play_cursor());
	}
	return d0;
}

int w_get_watermark_status(void)
{
	DWORD d0;
	int result;

	if(wd.initialized == FALSE) return 0;

	d0 = w_get_latency();

	if(d0 >= (DWORD)(g_latency*2.5)){
		result = W_WATERMARK_UNDERRUN;				// underrun or overrun
	}else if(d0 >= (DWORD)(g_latency*1.5)){
		result = W_WATERMARK_FULL;
	}else if(d0 >= (DWORD)g_latency){
		result = W_WATERMARK_GOOD;
	}else if(d0 >= g_samples_per_frame){
		result = W_WATERMARK_OK;
	}else if(d0 >= g_samples_per_frame/8){
		result = W_WATERMARK_ON_THE_EDGE;
	}else{
		result = W_WATERMARK_UNDERRUN;
	}

	return result;
}


static DWORD ofdt = 0;
static DWORD ufdt = 0;
static int dma_dt = 0;

DWORD w_adjust_latency_for_vsync(void)
{
	DWORD d0;

	if(wd.initialized == FALSE) return 0;

	d0 = w_calc_distance(w_get_write_cursor(), w_get_play_cursor());
	if( d0 >= (g_latency + 1) ){
		ofdt = d0 - g_latency;
		g_dma_overflow	+= 1;
		g_dma_underflow  = 0;
	}else if( d0 <= (g_latency - 1) ){
		ufdt = g_latency - d0;
		g_dma_overflow	 = 0;
		g_dma_underflow += 1;
	}else{
		g_dma_overflow	 = 0;
		g_dma_underflow  = 0;
	}
	if(g_dma_overflow > 16)
		g_dma_overflow = 16;
	if(g_dma_underflow > 16)
		g_dma_underflow = 16;
	ofdt = ofdt  * (g_dma_overflow	& 0xFE);
	ufdt = ufdt  * (g_dma_underflow & 0xFE);
	if(ofdt > (wd.playback_rate  >> 5))
		ofdt = (wd.playback_rate  >> 5);
	if(ufdt > (wd.playback_rate  >> 5))
		ufdt = (wd.playback_rate  >> 5);

	g_dma_dt = ufdt - ofdt;
	dma_dt = g_dma_dt;			// for debug

	calc_next_req();

	return d0;
}




float w_calc_samples_per_vsync(void (*__vsync)(void), int vsync_count)
{
	DWORD d0;
	DWORD result[2];
	int d1;
	int count;
	int limit;
	int d7;

	if(wd.initialized == FALSE) return 0;
	limit = 16;
	while(limit > 0){
		d7 = 0;
		while(d7 < 2){
			d1 = 0;
			while(d1 < 4){
				count = 0;
				while(count < vsync_count){
					(*__vsync)();
					count += 1;
				}
				get_played_sample_count();
				d1 += 1;
			}
			d0 = 0;
			d1 = 0;
			while(d1 < (256/8)){
				count = 0;
				while(count < vsync_count){
					(*__vsync)();
					count += 1;
				}
				d0 += get_played_sample_count();
				d1 += 1;
			}
			result[d7] = d0;
			d7 += 1;
		}
		if(result[0] > result[1]){
			d0 = result[0] - result[1];
		}else{
			d0 = result[1] - result[0];
		}
		if(d0 < 16) break;
		limit -= 1;
	}

	d0 = (result[0] + result[1]) * 4;
	g_latency = (d0 * g_latencym) / 256;
	g_dma_average_cnt = d0;
	calc_next_req();
	calc_next_req();

	return (float)d0/256;
}




void w_clear_buffer(void)
{
#if 0
	int d0;
	BYTE d1;

	if(wd.initialized == FALSE) return;
	d1 = 0x80;
	if(wd.pcm_format == _16BITSTEREO){
		d1 = 0x00;
	}
	_farsetsel(_dos_ds);
	d0 = 0;
	while(d0 < g_dma_buff_size){
		_farnspokeb(g_wss_dma_addr + d0, d1);
		d0 += 1;
	}

	if(DEVICE == DEVICE_GF1){
		u_set_volume(u_prim_voice + 0, 0);
		u_set_volume(u_prim_voice + 1, 0);

		d1 = 0x00;
		d0 = 0;
		while(d0 < 65536*2){
			u_upload(d0, d1);
			d0 += 1;
		}
		u_set_volume(u_prim_voice + 0, 0xFFFF);
		u_set_volume(u_prim_voice + 1, 0xFFFF);
	}
#else
	int d0;

	if(wd.initialized == FALSE) return;

	for(d0 = 0; d0 <= SAMPLECNT/1024; d0 += 1){
		w_lock_mixing_buffer(1024);
		w_mixing_zero();
		w_unlock_mixing_buffer();
	}
#endif
}





DWORD w_get_nominal_sample_rate(void)
{
	if(wd.initialized == FALSE) return 0;
	return wd.playback_rate;
}

char *w_get_device_name(void)
{
	if(wd.initialized == FALSE) return NULL;
	return wd.device_name;
}


void vga_vsync(void)
{
	w_enter_critical();
	while( (inportb(0x3da) & 0x08) != 0 );
	while( (inportb(0x3da) & 0x08) == 0 );
	w_exit_critical();
}


DWORD w_get_actual_sample_rate(void)
{
	DWORD d0;
	DWORD result[2];
	int d1;
	int d7;
	int limit;
	cycles_t prev;
	cycles_t curr;

	if(wd.initialized == FALSE) return 0;

	w_set_watermark(2.0, w_get_nominal_sample_rate()/60);

	prev = osd_cycles();
	while(1){
		if( (osd_cycles() - prev) >= osd_cycles_per_second()/2 ) break;
		get_played_sample_count();
	}

	limit = 16;
	while(limit > 0){
		d7 = 0;
		while(d7 < 2){
			d0 = 0;
			d1 = 65536;
			get_played_sample_count();
			while(d1 > 0){
				if(get_played_sample_count != 0) break;
				d1 -= 1;
			}
			prev = osd_cycles();
			while(1){
				d0 += get_played_sample_count();
				curr = osd_cycles();
				if( (curr - prev) >= osd_cycles_per_second()/2 ) break;
			}
			result[d7] = d0;
			d7 += 1;
		}
		if(result[0] > result[1]){
			d0 = result[0] - result[1];
		}else{
			d0 = result[1] - result[0];
		}
		if(d0 <= 8) break;
		limit -= 1;
	}
	actual_sample_rate = result[0] + result[1];
	return actual_sample_rate;
}


char *w_get_error_message(void)
{
	return get_error_message();
}

void w_set_device_master_volume(int volume)
{
	//		   max	  min
	// volume:	 0	-  31, or  -1(default)

	if( (0 <= volume) && (volume <= 31) ){
		sound_device_master_volume	= volume;
	}else{
		sound_device_master_volume	= -1;
	}
}


void w_sound_device_exit(void)
{
	w_clear_buffer();
	if(wd.device_exit != NULL){
		(*wd.device_exit)();
	}
	wd.initialized = FALSE;
}


int w_sound_device_init(int device_no, int rate)
{
	int result;

	if(wd.initialized == TRUE){
		set_error_message("The sound device was already initialized.\n");
		return FALSE;
	}

	if(rate <= 0)	 rate = 22050;
	if(rate > 64000) rate = 64000;

	switch(device_no){
		case 0:
			result = eternal_silence_start(rate);
			break;
		case 1:
			result = sb_auto_detect_start(rate);
			break;
		case 2:
			result = ac97_auto_detect_start(rate, FALSE);
			break;
		case 3:
			result = ac97_auto_detect_start(rate, TRUE);
			break;
		case 4:
			result = ultramax_start(rate);
			break;
		case 5:
			result = ultrasound_start(rate);
			break;
		case 6:
			result = wss_start(rate);
			break;
		case 7:
			result = soundscape_start(rate);
			break;
		case 8:
			result = ess_start(rate);
			break;
		case 9:
			result = wdm_sbpro_start(rate);
			break;
		case 10:
			result = sb201_interrupt_driven_start(rate);
			break;
		case 11:
			result = sb100_interrupt_driven_start(rate);
			break;
		case 12:
			result = sb201_start(rate);
			break;
		case 13:
			result = sbpro_start(rate);
			break;
		case 14:
			result = sb16_start(rate);
			break;
		case 15:
			result = sbpro_ex_start(rate);
			break;
		case 16:
			result = sb16_ex_start(rate);
			break;
		case 17:
/*
			  // Use SB interrupt-driven driver instead.
			result = cmi8x38_as_sb16_start(rate);
*/
			result = FALSE;
			set_error_message("invalid sound device number.\n");
			break;
		case 21:
			result = trid4dwave_as_sb16_start(rate);
			break;
		case 23:
			result = sbpro_interrupt_driven_start(rate);
			break;
		case 24:
			result = via686_start_chip_init(rate);
			break;
		case 25:
			result = via686_start_no_chip_init(rate);
			break;
		case 26:
			result = intel_ich_start_chip_init(rate);
			break;
		case 27:
			result = intel_ich_start_no_chip_init(rate);
			break;

		default:
			result = FALSE;
			set_error_message("invalid sound device number.\n");
	}

	w_set_watermark(2.2, 1200);

	return result;
}




//////////////////////////////////////////////////////////////////////
// main() for debug



#ifdef _WSS_DEBUG


static double sin1dt;
static double sin2ldt;
static double sin2rdt;
static double sin1t;
static double sin1t2;
static double sin2lt;
static double sin2rt;


static void sin_stream_init(int rate)
{
	sin1dt	= 2.0 * M_PI * 880.0 / rate;
	sin2ldt = 2.0 * M_PI * 441.5 / rate;
	sin2rdt = 2.0 * M_PI * 438.5 / rate;
	sin1t	= 0;
	sin1t2	= 0;
	sin2lt	= 0;
	sin2rt	= 0;
}

static short sin_stream1(void)
{
	short d0;

	sin1t2 += 0.00005;
	if(sin1t2 > 2*M_PI) sin1t2 -= 2*M_PI;
	d0 = (short)(  16384 * sin(sin1t += (sin1dt + (sin1dt/4)*sin(sin1t2)))	);
	return d0;
}

static short sin_stream2l(void)
{
	short d0;

	d0 = (short)(16384 * sin(sin2lt += sin2ldt));
	return d0;
}

static short sin_stream2r(void)
{
	short d0;

	d0 = (short)(16384 * sin(sin2rt += sin2rdt));
	return d0;
}


#define NORMAL							 1
#define VSYNC							 2

#define FRAMES_PER_SEC	60

int main()
{
	int d0;
	int actual_sample_rate;
	short a0[65536];
	short a1[65536*2];
	DWORD samples_this_frame = 0;
	DWORD samples_left_over;
	DWORD samples_per_frame;
	int soundcard;
	int i;
	int mode;

	mode = NORMAL;
//	  mode = VSYNC;

	printf("\nSelect the audio device:\n");
	printf("  0. Eternal Silence\n");
	printf("  1. Sound Blaster\n");
	printf("  2. Chipset integrated AC97 (no init, for Win9x DosBox. AT YOUR OWN RISK!)\n");
	printf("  3. Chipset integrated AC97 (for Pure DOS)\n");
	printf("  4. Ultrasound Max\n");
	printf("  5. Ultrasound\n");
	printf("  6. Windows Sound System\n");
	printf("  7. Ensoniq Soundscape\n");
	printf("  8. ESS Audiodrive\n");
	printf("  9. Sound Blaster Pro (for WDM SBPro device, -novsync & RDTSC required)\n");
	printf("\n");

	i = getch();
	soundcard = i - '0';
	if ( !((0 <= soundcard) && (soundcard <= 9)) )
	{
		printf("audio initialization failed\n");
		soundcard = -1;
		return -1;
	}

	if( w_sound_device_init(soundcard, 44100) == FALSE){
		printf("%s\n", w_get_error_message() );
		return -1;
	}

	printf("%s	", w_get_device_name() );
	printf("nominal rate : %d Hz\n", w_get_nominal_sample_rate());

	actual_sample_rate = w_get_actual_sample_rate();
	printf("  actual rate  : %d Hz\n", actual_sample_rate);

	sin_stream_init(actual_sample_rate);

	samples_this_frame = actual_sample_rate / FRAMES_PER_SEC;
	w_set_watermark(2.2, samples_this_frame);

	if(mode == VSYNC){
		samples_this_frame = (int)w_calc_samples_per_vsync(vga_vsync, 1);
		printf("samples per vsync : %d\n", samples_this_frame);
	}

	if(mode == NORMAL){
		samples_per_frame = (actual_sample_rate << 12) / FRAMES_PER_SEC;
		samples_left_over = 0;
	}

	/* main loop */
	while(1){
		if(mode == NORMAL){
			/* normal mode */
			samples_left_over += samples_per_frame;
			samples_this_frame = (DWORD)(samples_left_over >> 12);
			samples_left_over  = samples_left_over & 0xFFF;
		}else{
			/* vsync mode */
			samples_this_frame = w_get_next_req();
		}


		 /* do something here. start */

		 /* do something here. end */


		for(d0 = 0;d0 < samples_this_frame; d0 += 1){
			a0[d0] = sin_stream1();
		}
		for(d0 = 0;d0 < samples_this_frame; d0 += 1){
			a1[(d0 * 2) + 0] = sin_stream2l();
			a1[(d0 * 2) + 1] = sin_stream2r();
		}

		printf("samples : %d,  ", samples_this_frame);

		if(mode == NORMAL){
			/* normal mode */
			while(1){
				d0 = w_get_watermark_status();
				if(d0 == W_WATERMARK_GOOD || d0 == W_WATERMARK_OK || d0 == W_WATERMARK_ON_THE_EDGE) break;
				if(d0 == W_WATERMARK_UNDERRUN) {
					w_reset_watermark();
					printf("\n reset watermark \n\n");
					break;
				}
			}
			printf("latency:%5d, play:%5d, write:%5d \n",
						w_get_latency(),
						w_get_play_cursor(), w_get_write_cursor());
			w_lock_mixing_buffer(samples_this_frame);
			w_mixing(a0, samples_this_frame, 0, 256);
			w_mixing_stereo(a1, samples_this_frame, 256, 256);
			w_unlock_mixing_buffer();
		}else{
			/* vsync mode */
			w_lock_mixing_buffer(samples_this_frame);
			w_mixing(a0, samples_this_frame, 0, 256);
			w_mixing_stereo(a1, samples_this_frame, 256, 256);
			w_unlock_mixing_buffer();

			vga_vsync();

			d0 = w_get_watermark_status();
			if(d0 == W_WATERMARK_ON_THE_EDGE || d0 == W_WATERMARK_UNDERRUN){
				w_reset_watermark();
				printf("\n reset watermark \n\n");
			}
			w_adjust_latency_for_vsync();
			printf("latency:%5d, play:%5d, write:%5d \n",
						w_get_latency(),
						w_get_play_cursor(), w_get_write_cursor());
			printf("over %5d, under %5d, ofdt %5d, ufdt %5d, dt %5d\n",
				g_dma_overflow, g_dma_underflow, ofdt, ufdt, dma_dt);
		}

		if( kbhit() ) break;
	}

	w_sound_device_exit();

	return 0;
}


#endif

/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002 Andrea Mazzoleni
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
 * This video driver is STRONGLY derived from the video drivers of the
 * official MAME project
 */

#include <pc.h>
#include <conio.h>
#include <sys/farptr.h>
#include <go32.h>
#include <time.h>
#include <math.h>
#include <dpmi.h>

#include "board.h"

/* for Mach64 and later ATI cards with internal clocks */

/* Tested on: */
/* 3D Rage Pro II+, 3D Rage Charger and Xpert@Play */

/* CONFIG_CHIP_ID register constants */
#define CFG_CHIP_TYPE 0x0000FFFF
#define CFG_CHIP_CLASS 0x00FF0000
#define CFG_CHIP_REV 0xFF000000
#define CFG_CHIP_VERSION 0x07000000
#define CFG_CHIP_FOUNDRY 0x38000000
#define CFG_CHIP_REVISION 0xC0000000

/* Chip IDs read from CONFIG_CHIP_ID */
#define MACH64_GX_ID 0xD7
#define MACH64_CX_ID 0x57
#define MACH64_CT_ID 0x4354
#define MACH64_ET_ID 0x4554
#define MACH64_VT_ID 0x5654
#define MACH64_VU_ID 0x5655
#define MACH64_GT_ID 0x4754
#define MACH64_GU_ID 0x4755
#define MACH64_GP_ID 0x4750
#define MACH64_XP_ID 0x4742
#define MACH64_GM_ID 0x474d
#define MACH64_XP98_ID 0x4c42

/* Mach64 chip types */
#define MACH64_UNKNOWN 0
#define MACH64_GX 1
#define MACH64_CX 2
#define MACH64_CT 3
#define MACH64_ET 4
#define MACH64_VT 5
#define MACH64_GT 6

/* ATI Bios */
#define BIOS_DATA_BASE 0xC0000
#define BIOS_DATA_SIZE 0x8000

#define BUS_FIFO_ERR_ACK 0x00200000
#define BUS_HOST_ERR_ACK 0x00800000

/* Position of flags in the General Control register */
#define CRTC_Enable_Doubling 1
#define CRTC_Enable_Interlace (1 << 1)
#define CRTC_FLAG_lock_regs (1 << 22)
#define CRTC_FLAG_enable_crtc (1 << 25)
#define CRTC_FLAG_extended_display_mode (1 << 24)

/* Sync flags */
#define CRTC_H_SYNC_NEG 0x00200000
#define CRTC_V_SYNC_NEG 0x00200000

/* Flags for the General Test register */
#define GEN_OVR_OUTPUT_EN 0x20
#define HWCURSOR_ENABLE 0x80
#define GUI_ENGINE_ENABLE 0x100
#define BLOCK_WRITE_ENABLE 0x200

/* PLL registers */
#define PLL_WR_EN 0x02
#define PLL_MACRO_CNTL 0x01
#define PLL_REF_DIV 0x02
#define PLL_GEN_CNTL 0x03
#define MCLK_FB_DIV 0x04
#define PLL_VCLK_CNTL 0x05
#define VCLK_POST_DIV 0x06
#define VCLK0_FB_DIV 0x07
#define VCLK1_FB_DIV 0x08
#define VCLK2_FB_DIV 0x09
#define VCLK3_FB_DIV 0x0A
#define PLL_XCLK_CNTL 0x0B
#define PLL_TEST_CTRL 0x0E
#define PLL_TEST_COUNT 0x0F

/* MEM_CNTL register constants */
#define MEM_SIZE_ALIAS 0x00000007
#define MEM_SIZE_512K 0x00000000
#define MEM_SIZE_1M 0x00000001
#define MEM_SIZE_2M 0x00000002
#define MEM_SIZE_4M 0x00000003
#define MEM_SIZE_6M 0x00000004
#define MEM_SIZE_8M 0x00000005
#define MEM_SIZE_ALIAS_GTB 0x0000000F
#define MEM_SIZE_2M_GTB 0x00000003
#define MEM_SIZE_4M_GTB 0x00000007
#define MEM_SIZE_6M_GTB 0x00000009
#define MEM_SIZE_8M_GTB 0x0000000B
#define MEM_BNDRY 0x00030000
#define MEM_BNDRY_0K 0x00000000
#define MEM_BNDRY_256K 0x00010000
#define MEM_BNDRY_512K 0x00020000
#define MEM_BNDRY_1M 0x00030000
#define MEM_BNDRY_EN 0x00040000

/* DSP_CONFIG register constants */
#define DSP_XCLKS_PER_QW 0x00003fff
#define DSP_LOOP_LATENCY 0x000f0000
#define DSP_PRECISION 0x00700000

/* DSP_ON_OFF register constants */
#define DSP_OFF 0x000007ff
#define DSP_ON 0x07ff0000

/* CONFIG_STAT0 register constants (CT, ET, VT) */
#define CFG_MEM_TYPE_xT 0x00000007

/* Memory types for CT, ET, VT, GT */
#define DRAM 1
#define EDO_DRAM 2
#define PSEUDO_EDO 3
#define SDRAM 4
#define SGRAM 5

/* type of clock */
#define CLK_ATI18818_0 0
#define CLK_ATI18818_1 1
#define CLK_STG1703 2
#define CLK_CH8398 3
#define CLK_INTERNAL 4
#define CLK_ATT20C408 5
#define CLK_IBMRGB514 6

/* bit in clock register for strobing */
#define CLOCK_STROBE 0x40

/* Save values */
static int DSPSet;
static int SaveDSPOnOff;
static int SaveDSPConfig;

/* Currently -
 * only ATI cards based on the Mach64 chipset which have an internal clock/DAC
 * are supported (see note below)
 */

#define MACH64_CLOCK_TYPE_MAX 7

/* Type of clock */
const char *mach64ClockTypeTable[MACH64_CLOCK_TYPE_MAX] =
{
	"ATI18818-0",
	"ATI18818-1/ICS2595",
	"STG1703",
	"CH8398",
	"Internal",
	"AT&T20C408",
	"IBM-RGB514",
};

/* the clock info we need for integrated DACs */
static int RefFreq;     /* Reference frequency for clock */
static int RefDivider;  /* Reference divider for clock */
static int MinFreq;     /* Minimum frequency of clock */
static int MaxFreq;     /* Maximum frequency of clock */
static int VRAMMemClk;  /* Speed of video RAM clock */
static int CXClk;       /* ID of clock we're going to program */
static int MemSize;     /* Memory on the card */
static int MemType;     /* Type of memory on the card */
static int ChipID;      /* ID of chip on card */
static int ChipRev;     /* revision of chip on card */
static int ChipType;    /* what we decide in our wisdom the chip is */

/* the MACH64 registers we're interested in */
static int _mach64_clock_reg;
static int _mach64_gen_cntrl;
static int _mach64_off_pitch;
static int _mach64_dac_cntl;
static int _mach64_config_stat0;
static int _mach64_crtc_h_total;
static int _mach64_crtc_h_sync;
static int _mach64_crtc_v_total;
static int _mach64_crtc_v_sync;
static int _mach64_crtc_v_line;
static int _mach64_over_left_right;
static int _mach64_over_top_bott;
static int _mach64_mem_cntl;
static int _mach64_chip_id;
static int _mach64_dsp_config;
static int _mach64_dsp_on_off;

static int ati_clock_get(int nTargetMHz, int *N, int *P, int *externaldiv)
{
	int postDiv;
	int nActualMHz;
	float Q;

	/* assume the best */
	nActualMHz = nTargetMHz;

	card_log("ati: Clock range is %.2f - %.2f MHz\n", (double)MinFreq / 100, (double)MaxFreq / 100 );

	/* check clock is in range */
	if (nActualMHz < MinFreq)
		card_log("ati: Clock too low\n");
	if (nActualMHz > MaxFreq)
		return -1;

	/* formula for clock is as follows */
	/* Clock = ((2 * mach64RefFreq * N)/(mach64RefDivider * postDiv)) */
	Q = (nActualMHz * RefDivider)/(2.0 * RefFreq);
	card_log("ati: Q:%d\n", (int)Q);
	*externaldiv = 0;

	if (Q > 255) {
		card_log("ati: Q too big\n");
		Q = 255;
		*P = 0;
		postDiv = 1;
	} else if (Q > 127.5) {
		*P = 0;
		postDiv = 1;
	} else if (Q > 85) {
		*P = 1;
		postDiv = 2;
	} else if (Q > 63.75) {
		*P = 0;
		postDiv = 3;
		*externaldiv = 1;
	} else if (Q > 42.5) {
		*P = 2;
		postDiv = 4;
	} else if (Q > 31.875) {
		*P = 2;
		postDiv = 6;
		*externaldiv = 1;
	} else if (Q > 21.25) {
		*P = 3;
		postDiv = 8;
	} else if (Q >= 10.6666666667) {
		*P = 3;
		postDiv = 12;
		*externaldiv = 1;
	} else {
		*P = 3;
		postDiv = 12;
		*externaldiv = 1;
	}
	*N = (int)(Q * postDiv + 0.5);

	nActualMHz = ((2 * RefFreq * (*N)) / (RefDivider * postDiv));

	card_log("ati: MACH64 N val:%d\n", *N);
	card_log("ati: MACH64 Post Div val:%d\n", *P);
	card_log("ati: MACH64 external div:%d\n", *externaldiv);

	return nActualMHz;
}

static int ati_port_get(int io_type, int io_base, int io_sel, int mm_sel)
{
	if (io_type) {
		return (mm_sel << 2) + io_base;
	} else {
		if (!io_base)
			io_base = 0x2EC;
		return (io_sel << 10) + io_base;
	}
}

static int ati_dsp_set(int nAdd)
{
	unsigned short dsp_on, dsp_off, dsp_xclks_per_qw, dsp_prec, loop_latency;
	long portval;
	int offset = 0;

	offset += nAdd;
	/* okay, I've been unable to get any reliable information about the DSP */
	/* so these are just values that work for this :- */
	/* dot clock speed, colour depth and video clock speed */
	/* found after a bit of hacking around */
	/*- The memory stuff is probably real though */

	/* get the colour depth */
	portval = inportb (_mach64_gen_cntrl+1) & 7;

	switch (portval) {
		case  2: /* 8 bit colour */
			if (VRAMMemClk >= 10000)
				/* 100MHz - for newer/faster cards */
				dsp_xclks_per_qw = 2239+offset;
			else
				/* Standard video memory speed (usually 60MHz)*/
				dsp_xclks_per_qw = 2189+offset;
			card_log("ati: DSP value %d (8bit)\n", dsp_xclks_per_qw);
			break;
		case  3: /* 16 bit colour */
		case  4: /* either 555 or 565 */
			if (VRAMMemClk >= 10000)
				/* 100MHz - for newer/faster cards */
				dsp_xclks_per_qw = 3655+offset;
			else
				/* Standard video memory speed (usually 60MHz)*/
				dsp_xclks_per_qw = 3679+offset;
			card_log("ati: DSP value %d (16bit)\n", dsp_xclks_per_qw);
			break;
		default: /* any other colour depth */
			card_log("ati: Unsupported colour depth for ATI driver (%d)\n", (int)inportb (_mach64_gen_cntrl+1));
			return 0;
	}

	if (MemSize > MEM_SIZE_1M) {
		if (MemType >= SDRAM)
			loop_latency = 8;
		else
			loop_latency = 6;
	} else {
		if (MemType >= SDRAM)
			loop_latency = 9;
		else
			loop_latency = 8;
	}

	/* our DSP values */
	dsp_on = 106;
	dsp_off = 206;
	dsp_prec = 5;

	card_log("ati: DSP on:%d DSP off:%d DSP clks :%d  DSP prec:%d  latency :%d\n", (int)dsp_on, (int)dsp_off, (int)dsp_xclks_per_qw, (int)dsp_prec, (int)loop_latency);
	/* save whats there */
	SaveDSPOnOff = inportw (_mach64_dsp_on_off);
	SaveDSPConfig = inportw (_mach64_dsp_on_off);

	/* write everything out */
	outportw (_mach64_dsp_on_off, ((dsp_on << 16) & DSP_ON) | (dsp_off & DSP_OFF));
	outportw (_mach64_dsp_config, ((dsp_prec << 20) & DSP_PRECISION) | ((loop_latency << 16) & DSP_LOOP_LATENCY)
				| (dsp_xclks_per_qw & DSP_XCLKS_PER_QW));
	DSPSet = 1;

	return dsp_xclks_per_qw;
}

static void resetmach64DSP(void)
{
	if (DSPSet) {
		outportw (_mach64_dsp_on_off, SaveDSPOnOff);
		outportw (_mach64_dsp_config, SaveDSPConfig);
	}
}

/*see if we've really got an ATI card with a Mach64 chipset  */
int ati_detect(void)
{
	__dpmi_regs r;
	int scratch_reg;
	unsigned long old;
	char bios_data[BIOS_DATA_SIZE];
	unsigned short *sbios_data = (unsigned short *)bios_data;
	int ROM_Table_Offset;
	int Freq_Table_Ptr;
	int Clock_Type;

	/* query mach64 BIOS for the I/O base address */
	r.x.ax = 0xA012;
	r.x.cx = 0;
	__dpmi_int (0x10, &r);

	if (r.h.ah) {
		return 0;
	}

	/* test scratch register to confirm we have a mach64 */
	scratch_reg = ati_port_get(r.x.cx, r.x.dx, 0x11, 0x21);

	old = inportl(scratch_reg);

	outportl (scratch_reg, 0x55555555);
	if (inportl (scratch_reg) != 0x55555555)
	{
		outportl (scratch_reg, old);
		card_log("ati: Not Mach64 Chipset\n");
		return 0;
	}

	outportl (scratch_reg, 0xAAAAAAAA);
	if (inportl (scratch_reg) != 0xAAAAAAAA)
	{
		outportl (scratch_reg, old);
		card_log("ati: Not Mach64 Chipset\n");
		return 0;
	}

	outportl (scratch_reg, old);

	/* get info from the ATI BIOS */
	dosmemget (BIOS_DATA_BASE, BIOS_DATA_SIZE, bios_data);
	ROM_Table_Offset = sbios_data[0x48 >> 1];
	Freq_Table_Ptr = sbios_data[(ROM_Table_Offset >> 1) + 8];
	Clock_Type = bios_data[Freq_Table_Ptr];
	CXClk = bios_data[Freq_Table_Ptr + 6];
	RefFreq = sbios_data[(Freq_Table_Ptr >> 1) + 4];
	RefDivider = sbios_data[(Freq_Table_Ptr >> 1) + 5];
	MinFreq = sbios_data[(Freq_Table_Ptr >> 1) + 1];
	MaxFreq = sbios_data[(Freq_Table_Ptr >> 1) + 2];
	VRAMMemClk = sbios_data[(Freq_Table_Ptr >> 1) + 9];

	if (Clock_Type < MACH64_CLOCK_TYPE_MAX)
		card_log("ati: type of MACH64 clk: %s (%d)\n", mach64ClockTypeTable[Clock_Type], Clock_Type);
	else
		card_log("ati: type of MACH64 clk: Unknown (%d)\n", Clock_Type);

	card_log("ati: MACH64 ref Freq:%d\n", RefFreq);
	card_log("ati: MACH64 ref Div:%d\n", RefDivider);
	card_log("ati: MACH64 min Freq:%d\n", MinFreq);
	card_log("ati: MACH64 max Freq:%d\n", MaxFreq);
	card_log("ati: MACH64 Mem Clk %d\n", VRAMMemClk);

	/* Get some useful registers while we're here  */

/*
M+000h/02ECh D(R/W):  Crtc_H_Total_Disp
bit   0-7  Crtc_H_Total. Horizontal Total in character clocks (8 pixel units)
	16-23  Crtc_H_Disp. Horizontal Display End in character clocks.
*/

	_mach64_crtc_h_total = ati_port_get(r.x.cx, r.x.dx, 0x00, 0x00);
/*
M+004h/06ECh D(R/W):  Crtc_H_Sync_Strt_Wid
bit   0-7  Crtc_H_Sync_Strt. Horizontal Sync Start in character clocks (8
	pixel units)
	8-10  Crtc_H_Sync_Dly. Horizontal Sync Start delay in pixels
	16-20  Crtc_H_Sync_Wid. Horizontal Sync Width in character clocks
	21  Crtc_H_Sync_Pol. Horizontal Sync Polarity
*/
	_mach64_crtc_h_sync = ati_port_get(r.x.cx, r.x.dx, 0x01, 0x01);

/*
M+008h/0AECh D(R/W):  Crtc_V_Total_Disp
bit  0-10  Crtc_V_Total. Vertical Total
	16-26  Crtc_V_Disp. Vertical Displayed End
*/
	_mach64_crtc_v_total = ati_port_get(r.x.cx, r.x.dx, 0x02, 0x02);

/*
M+00Ch/0EECh D(R/W):  Crtc_V_Sync_Strt_Wid
bit  0-10  Crtc_V_Sync_Strt. Vertical Sync Start
	16-20  Crtc_V_Sync_Wid. Vertical Sync Width
	21  Crtc_V_Sync_Pol. Vertical Sync Polarity
*/
	_mach64_crtc_v_sync = ati_port_get(r.x.cx, r.x.dx, 0x03, 0x03);

/*
M+010h/12ECh D(R/W):  Crtc_Vline_Crnt_Vline
bit  0-10  The line at which Vertical Line interrupt is triggered
	16-26  (R) Crtc_Crnt_Vline. The line currently being displayed
*/
	_mach64_crtc_v_line = ati_port_get(r.x.cx, r.x.dx, 0x04, 0x04);

/*
M+014h/16ECh D(R/W):  Crtc_Off_Pitch
bit  0-19  Crtc_Offset. Display Start Address in units of 8 bytes.
	22-31  Crtc_Pitch. Display pitch in units of 8 pixels
*/
	_mach64_off_pitch = ati_port_get(r.x.cx, r.x.dx, 0x05, 0x05);

/*
M+01Ch/1EECh D(R/W):  Crtc_Gen_Cntl
bit     0  Crtc_Dbl_Scan_En. Enables double scan
	1  Crtc_Interlace_En. Enables interlace.
	2  Crtc_Hsync_Dis. Disables Horizontal Sync output
	3  Crtc_Vsync_Dis. Disables Vertical Sync output
	4  Crtc_Csync_En. Enable composite sync on Horizontal Sync output
	5  Crtc_Pic_By_2_En. CRTC advances 2 pixels per pixel clock
	8-10  Crtc_Pix_Width. Displayed bits/pixel: 1: 4bpp, 2: 8bpp, 3: 15bpp
		(5:5:5), 4: 16bpp (5:6:5), 5: 24bpp(undoc), 6: 32bpp
	11  Crtc_Byte_Pix_Order. Pixel order within each byte (4bpp).
		0: High nibble displayed first, 1: low nibble displayed first
	16-19  Crtc_Fifo_Lwm. Low Water Mark of the 16entry deep display FIFO.
	Only used in DRAM configurations. The minimum number of entries
	remaining in the FIFO before the CRTC starts refilling. Ideally
	should be set to the lowest number that gives a stable display.
	24  Crtc_Ext_Disp_En. 1:Extended display mode , 0:VGA display mode
	25  Crtc_En. Enables CRTC if set, resets if clear

*/
	_mach64_gen_cntrl = ati_port_get(r.x.cx, r.x.dx, 0x07, 0x07);

/* DSP registers  */
/* No reliable information available about these....  */
	_mach64_dsp_config = ati_port_get(r.x.cx, r.x.dx, 0x08, 0x08);
	_mach64_dsp_on_off = ati_port_get(r.x.cx, r.x.dx, 0x09, 0x09);

/*
M+090h/4AECh D(R/W):  Clock_Cntl
bit   0-3  Clock_Sel. Clock select bit 0-3. Output to the clock chip
	4-5  Clock_Div. Internal clock divider. 0: no div, 1: /2, 2: /4
	6  (W) Clock_Strobe. Connected to the strobe or clk input on
		programmable clock chips
	7  Clock_Serial_Data. Data I/O for programmable clock chips
*/
	_mach64_clock_reg = ati_port_get(r.x.cx, r.x.dx, 0x12, 0x24);

/*
M+0C4h/62ECh D(R/W):  Dac_Cntl
bit   0-1  Dac_Ext_Sel. Connected to the RS2 and RS3 inputs on the DAC.
	8  Dac_8bit_En. Enables 8bit DAC mode (256colors of 16M) if set
	9-10  Dac_Pix_Dly. Setup and hold time on pixel data. 0: None,
		1: 2ns - 4ns delay, 2: 4ns - 8ns delay
	11-12  Dac_Blank_Adj. Blank Delay in number of pixel clock periods.
		0: None, 1: 1 pixel clock, 2: 2 pixel clocks
	13  Dac_VGA_Adr_En. When bit 24 of Crtc_Gen_Cntl (M+01Ch/1EECh) is set,
	this bit enables access to the VGA DAC I/O addresses (3C6h-3C9h).
	16-18  Dac_Type. The DAC type - initialised from configuration straps on
	power-up. See Config_Stat0 (M+0E4h/72ECh) bits 9-11 for details
*/
	_mach64_dac_cntl = ati_port_get(r.x.cx, r.x.dx, 0x18, 0x31);

/*
M+0E4h/72ECh D(R):  Config_Stat0
bit   0-2  Cfg_Bus_Type. Host Bus type. 0: ISA, 1: EISA, 6: VLB, 7: PCI
	3-5  Cfg_Mem_Type. Memory Type. 0: DRAM (256Kx4), 1: VRAM (256Kx4, x8,
		x16), 2: VRAM (256Kx16 short shift reg), 3: DRAM (256Kx16),
		4: Graphics DRAM (256Kx16), 5: Enh VRAM (256Kx4, x8, x16), 6: Enh
		VRAM (256Kx16 short shift reg)
	6  Cfg_Dual_CAS_En. Dual CAS support enabled if set
	7-8  Cfg_Local_Bus_Option. Local Bus Option.
		1: Local option 1, 2: Local option 2, 3: Local option 3
	9-11  Cfg_Init_DAC_Type. DAC type. 2: ATI68875/TI 34075, 3: Bt476/Bt478,
		4: Bt481, 5: ATI68860/ATI68880, 6: STG1700, 7: SC15021
	12-14  Cfg_Init_Card_ID. Card ID. 0-6: Card ID 0-6, 7: Disable Card ID
	15  Cfg_Tri_Buf_Dis. Tri-stating of output buffers during reset
	disabled if set
	16-21  Cfg_Ext_ROM_Addr. Extended Mode ROM Base Address. Bits 12-17 of the
	ROM base address, 0: C0000h, 1: C1000h ... 3Fh: FE000h
	22  Cfg_ROM_Dis. Disables ROM if set
	23  Cfg_VGA_Enm. Enables VGA Controller
	24  Cfg_Local_Bus_Cfg. 0: Local Bus configuration 2, 1: configuration 1
	25  Cfg_Chip_En. Enables chip if set
	26  Cfg_Local_Read_Dly_Dis. If clear delays read cycle termination by 1
	bus clock, no delay if set
	27  Cfg_ROM_Option. ROM Address. 0: E0000h, 1: C0000h
	28  Cfg_Bus_option. EISA bus: Enables POS registers if set, disables
	POS registers and enables chip if clear.
	VESA Local Bus: Enables decode of I/O address 102h if clear,
	disables if set
	29  Cfg_Local_DAC_Wr_En. Enables local bus DAC writes if set
	30  Cfg_VLB_Rdy_Dis. Disables VESA local bus compliant RDY if set
	31  Cfg_Ap_4Gbyte_Dis. Disables 4GB Aperture Addressing if set
*/
	_mach64_config_stat0 = ati_port_get(r.x.cx, r.x.dx, 0x1c, 0x39);

/*
M+044h/26ECh D(R/W):  Ovr_Wid_Left_Right
bit   0-3  Ovr_Wid_Left. Left overscan width in character clocks
	16-19  Ovr_Wid_Right. Right overscan width in character clocks
*/
	_mach64_over_left_right = ati_port_get(r.x.cx, r.x.dx, 0x09, 0x11);

/*
M+048h/2AECh D(R/W):  Ovr_Wid_Top_Bottom
bit   0-7  Ovr_Wid_Top. Top overscan width in lines
	16-23  Ovr_Wid_Bottom. Bottom overscan width in lines
*/
	_mach64_over_top_bott = ati_port_get(r.x.cx, r.x.dx, 0x0a, 0x12);

/*
M+0B0h/52ECh D(R/W):  Mem_Cntl
bit   0-2  Mem_Size. Video Memory Size. 0: 512K, 1: 1MB, 2: 2MB, 3: 4MB,
		4: 6MB, 5: 8MB
	4  Mem_Rd_Latch_En. Enables latching on RAM port data
	5  Mem_Rd_Latch_Dly. Delays latching of RAM port data by 1/2 memory
	clock period
	6  Mem_Sd_Latch_En. Enables latching of data on serial port data
	7  Mem_Sd_Latch_Dly. Delays latching of serial port data by 1/2 memory
	clock period
	8  Mem_Fill_Pls. One memory clock period set for width of data latch
	pulse
	9-10  Mem_Cyc_Lnth. memory cycle length for non-paged access:
		0: 5 mem clock periods, 1: 6 mem clks, 2: 7 mem clks
	16-17  Mem_Bndry. VGA/Mach Memory boundary. If the memory boundary is
	enabled (bit 18 is set) defines the amount of memory reserved for
	the VGA.  0: 0K, 1: 256K, 2: 512K, 3: 1M
	18  Mem_Bndry_En. If set the video memory is divided between the VGA
	engine and the Mach engine, with the low part reserved for the VGA
	engine, if clear they share the video memory
*/
	_mach64_mem_cntl = ati_port_get(r.x.cx, r.x.dx, 0x14, 0x2c);

/*
M+0E0h/6EECh D(R):  Config_Chip_ID.
bit  0-15  Cfg_Chip_Type. Product Type Code. 0D7h for the 88800GX,
		57h for the 88800CX (guess)
	16-23  Cfg_Chip_Class. Class code
	24-31  Cfg_Chip_Rev. Revision code
*/
	_mach64_chip_id = ati_port_get(r.x.cx, r.x.dx, 0x1b, 0x38);

	/* get the chip ID  */
	old = inportl(_mach64_chip_id);
	ChipID = (int)(old & CFG_CHIP_TYPE);
	ChipRev = (int)((old & CFG_CHIP_REV) >> 24);

	card_log("ati: Chip ID :%d\n", ChipID);
	card_log("ati: Chip Rev :%d\n", ChipRev);

	switch (ChipID)
	{
		case MACH64_GX_ID:
			ChipType = MACH64_GX;
			break;
		case MACH64_CX_ID:
			ChipType = MACH64_CX;
			break;
		case MACH64_CT_ID:
			ChipType = MACH64_CT;
			break;
		case MACH64_ET_ID:
			ChipType = MACH64_ET;
			break;
		case MACH64_VT_ID:
		case MACH64_VU_ID:
			ChipType = MACH64_VT;
			break;
		case MACH64_GT_ID:
		case MACH64_GU_ID:
		case MACH64_GP_ID:
		case MACH64_XP_ID:
		case MACH64_XP98_ID:
		case MACH64_GM_ID:
			ChipType = MACH64_GT;
			break;
		default:
			ChipType=MACH64_UNKNOWN;
	}

	card_log("ati: Chip Type :%d\n", ChipType);

	/* and the memory on the card  */
	old = inportl (_mach64_mem_cntl);
	switch (old & MEM_SIZE_ALIAS_GTB)
	{
		case MEM_SIZE_512K:
		case MEM_SIZE_1M:
			MemSize = (int)old & MEM_SIZE_ALIAS_GTB;
			break;
		case MEM_SIZE_2M_GTB:
			MemSize= MEM_SIZE_2M;
			break;
		case MEM_SIZE_4M_GTB:
			MemSize= MEM_SIZE_4M;
			break;
		case MEM_SIZE_6M_GTB:
			MemSize= MEM_SIZE_6M;
			break;
		case MEM_SIZE_8M_GTB:
			MemSize= MEM_SIZE_8M;
			break;
		default:
			MemSize=MEM_SIZE_1M;
	}
	card_log("ati: Video Memory %d\n", MemSize);

	/* and the type of memory  */
	old = inportl(_mach64_config_stat0);
	MemType = old & CFG_MEM_TYPE_xT;

	card_log("ati: Video Memory  Type %d\n", MemType);

	/* only bail out here if the clock's wrong  */
	/* so we can collect as much info as possible about the card  */
	/* - just in case I ever feel like adding RAMDAC support  */
	if (Clock_Type != CLK_INTERNAL) {
		card_log("ati: Clock type not supported, only internal clocks implemented\n");
		return 0;
	}

	DSPSet = 0;

	card_log("ati: Found Mach64 based card with internal clock\n");
	return 1;
}

int ati_set(const card_crtc STACK_PTR* _cp, const card_mode STACK_PTR* cm, const card_mode STACK_PTR* co)
{
	card_crtc cp = *_cp;
	int n, extdiv, P;
	int nOffSet;
	long int temp;
	long int gen;
	int temp1, temp2, temp3;
	int nActualMHz;

	if (!card_compatible_mode(cm, co)) {
		CARD_LOG(("ati: incompatible mode\n"));
		return 0;
	}

	temp = inportl (_mach64_off_pitch);
	temp >>= 22;
	nOffSet = temp;

	if (cp.interlace) {
		CARD_LOG(("ati: halve the dotclock for interlace\n"));
		cp.dotclockHz /= 2;
	}

	card_log("ati: Clock requested:%.2f\n", (double)cp.dotclockHz / 1000000 );
	nActualMHz = ati_clock_get (cp.dotclockHz / 10000, &n, &P, &extdiv);
	if (nActualMHz < 0) {
		card_log("ati: Clock out of range\n");
		return 0;
	}
	card_log("ati: Clock selected:%.2f\n", (double)nActualMHz / 100);

	gen = inportl(_mach64_gen_cntrl);

	/* unlock the regs and disable the CRTC */
	outportw (_mach64_gen_cntrl, gen & ~(CRTC_FLAG_lock_regs|CRTC_FLAG_enable_crtc));

	/* we need to program the DSP - otherwise we'll get nasty artifacts all over the screen */
	/* when we change the clock speed */
	if (((ChipType == MACH64_VT || ChipType == MACH64_GT)&&(ChipRev & 0x07))) {
		card_log("ati: Programming the DSP\n");
		if (!ati_dsp_set(0)) {
			outportl (_mach64_gen_cntrl, gen);
			card_log("ati: Error programming the DSP\n");
			return 0;
		}
	} else {
		card_log("ati: Decided NOT to program the DSP\n");
	}

	/* now we can program the clock */
	outportb (_mach64_clock_reg + 1, PLL_VCLK_CNTL << 2);
	temp1 = inportb (_mach64_clock_reg + 2);
	outportb (_mach64_clock_reg + 1, (PLL_VCLK_CNTL  << 2) | PLL_WR_EN);
	outportb (_mach64_clock_reg + 2, temp1 | 4);

	outportb (_mach64_clock_reg + 1, (VCLK_POST_DIV << 2));
	temp2 = inportb (_mach64_clock_reg + 2);

	outportb (_mach64_clock_reg + 1, ((VCLK0_FB_DIV + CXClk) << 2) | PLL_WR_EN);
	outportb (_mach64_clock_reg +2, n);

	outportb (_mach64_clock_reg + 1, (VCLK_POST_DIV << 2) | PLL_WR_EN);
	outportb (_mach64_clock_reg + 2, (temp2 & ~(0x03 << (2 * CXClk))) | (P << (2 * CXClk)));

	outportb (_mach64_clock_reg + 1, PLL_XCLK_CNTL << 2);
	temp3 = inportb (_mach64_clock_reg + 2);
	outportb (_mach64_clock_reg + 1, (PLL_XCLK_CNTL << 2) | PLL_WR_EN);

	if (extdiv)
		outportb (_mach64_clock_reg + 2, temp3 | (1 << (CXClk + 4)));
	else
		outportb (_mach64_clock_reg + 2, temp3 & ~(1 << (CXClk + 4)));

	outportb (_mach64_clock_reg + 1, (PLL_VCLK_CNTL << 2) | PLL_WR_EN);
	outportb (_mach64_clock_reg + 2, temp1&~0x04);

	/* reset the DAC */
	inportb (_mach64_dac_cntl);

	card_log("ati: H total %d, H display %d, H sync offset %d\n", cp.HTotal / 8, cp.HDisp / 8, cp.HSStart / 8);
	card_log("ati: V total %d, V display %d, V sync offset %d\n", cp.VTotal, cp.VDisp, cp.VSStart);

	/* now setup the CRTC timings */
	outportb (_mach64_crtc_h_total, cp.HTotal / 8);  /* h total */ /* in clock unit */
	outportb (_mach64_crtc_h_total + 2, cp.HDisp / 8); /* h display width */ /* in clock unit */
	outportb (_mach64_crtc_h_sync, cp.HSStart / 8);   /* h sync start */ /* in clock unit */
	outportb (_mach64_crtc_h_sync + 1, 0);  /* h sync delay */
	outportb (_mach64_crtc_h_sync + 2, cp.HSEnd - cp.HSStart); /* h sync width */
	outportw (_mach64_crtc_v_total, cp.VTotal);   /* v total */
	outportw (_mach64_crtc_v_total + 2, cp.VDisp);  /* v display height */
	outportw (_mach64_crtc_v_sync, cp.VSStart);    /* v sync start */
	outportb (_mach64_crtc_v_sync + 2, cp.VSEnd - cp.VSStart);          /* v sync width */

	/* make sure sync is negative */
	temp = inportl(_mach64_crtc_h_sync);
	temp |= CRTC_H_SYNC_NEG;
	outportl (_mach64_crtc_h_sync, temp);

	temp = inportl(_mach64_crtc_v_sync);
	temp |= CRTC_V_SYNC_NEG;
	outportl (_mach64_crtc_v_sync, temp);

	/* clear any overscan */
	outportb (_mach64_over_left_right, 0);
	outportb (_mach64_over_left_right+2, 0);
	outportb (_mach64_over_top_bott, 0);
	outportb (_mach64_over_top_bott+2, 0);

	/* set memory for each line */
	temp = inportl(_mach64_off_pitch);
	temp &= 0xfffff;
	outportl (_mach64_off_pitch, temp|(nOffSet<<22));

	/* max out the FIFO */
	gen |= (15<<16);

	/* turn on/off interlacing */
	if (cp.interlace)
		gen |= CRTC_Enable_Interlace;
	else
		gen &=~ CRTC_Enable_Interlace;

	/* turn on/off doublescan */
	if (cp.doublescan)
		gen |= CRTC_Enable_Doubling;
	else
		gen &=~ CRTC_Enable_Doubling;

	/* set the display going again */
	outportl (_mach64_gen_cntrl, gen);

	/* finally select and strobe the clock */
	outportb (_mach64_clock_reg, CXClk | CLOCK_STROBE);

	return 1;
}

void ati_reset(void)
{
	/* reset the DSP */
	resetmach64DSP();
	/* it could be a bit risky resetting the clock + general registers if we're running on an arcade monitor */
	/* so, I'll leave 'em for the moment */
}



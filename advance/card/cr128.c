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
 * This driver requires a VBE2 4F04 call to save the state of the video
 * board BEFORE the r128_set call and the restore of the state BEFORE
 * setting another video mode using the BIOS (int 10h).
 * If this isn't done the clock is incorrectly setup.
 * I tried also implementing a "unset" call that restores the modified R128
 * registers, but without success...
 */

#include "board.h"

struct r128_id {
	unsigned value;
	const char* name;
	int supported;
};

/* Configuration */
#define R128_CONF_UNSUPPORTED 0
#define R128_CONF_GENERIC 1

static struct r128_id r128_id_list[] = {
/* From XFree 4.1.0 */
{ 0x4C45, "ATI Rage 128 Mobility LE PCI", R128_CONF_GENERIC },
{ 0x4C46, "ATI Rage 128 Mobility LF AGP", R128_CONF_GENERIC },
{ 0x4D46, "ATI Rage 128 Mobility MF AGP", R128_CONF_GENERIC },
{ 0x4D4C, "ATI Rage 128 Mobility ML AGP", R128_CONF_GENERIC },
{ 0x5046, "ATI Rage 128 Pro PF AGP", R128_CONF_GENERIC },
{ 0x5052, "ATI Rage 128 Pro PR PCI", R128_CONF_GENERIC },
{ 0x5245, "ATI Rage 128 RE PCI", R128_CONF_GENERIC },
{ 0x5246, "ATI Rage 128 RF AGP", R128_CONF_GENERIC }, /* TESTED */
{ 0x524B, "ATI Rage 128 RK PCI", R128_CONF_GENERIC },
{ 0x524C, "ATI Rage 128 RL AGP", R128_CONF_GENERIC },

/* From pcidevs.txt 151 */
{ 0x5041, "ATI Rage 128 Pro PCI", R128_CONF_GENERIC },
{ 0x5042, "ATI Rage 128 Pro AGP 2x", R128_CONF_GENERIC },
{ 0x5043, "ATI Rage 128 Pro AGP 4x", R128_CONF_GENERIC },
{ 0x5044, "ATI Rage 128 Pro PCI (TMDS)", R128_CONF_GENERIC },
{ 0x5045, "ATI Rage 128 Pro AGP 2x (TMDS)", R128_CONF_GENERIC },
{ 0x5047, "ATI Rage 128 Pro PCI", R128_CONF_GENERIC },
{ 0x5048, "ATI Rage 128 Pro AGP 2x", R128_CONF_GENERIC },
{ 0x5049, "ATI Rage 128 Pro AGP 4x", R128_CONF_GENERIC },
{ 0x504A, "ATI Rage 128 Pro PCI (TMDS)", R128_CONF_GENERIC },
{ 0x504B, "ATI Rage 128 Pro AGP 2x (TMDS)", R128_CONF_GENERIC },
{ 0x504C, "ATI Rage 128 Pro AGP 4x (TMDS)", R128_CONF_GENERIC },
{ 0x504D, "ATI Rage 128 Pro PCI", R128_CONF_GENERIC },
{ 0x504E, "ATI Rage 128 Pro AGP 2x", R128_CONF_GENERIC },
{ 0x504F, "ATI Rage 128 Pro AGP 4x", R128_CONF_GENERIC },
{ 0x5050, "ATI Rage 128 Pro PCI (TMDS)", R128_CONF_GENERIC },
{ 0x5051, "ATI Rage 128 Pro AGP 2x (TMDS)", R128_CONF_GENERIC },
{ 0x5052, "ATI Rage 128 Pro AGP 4x (TMDS)", R128_CONF_GENERIC },
{ 0x5053, "ATI Rage 128 Pro PCI", R128_CONF_GENERIC },
{ 0x5054, "ATI Rage 128 Pro AGP 2x", R128_CONF_GENERIC },
{ 0x5055, "ATI Rage 128 Pro AGP 4x", R128_CONF_GENERIC },
{ 0x5056, "ATI Rage 128 Pro PCI (TMDS)", R128_CONF_GENERIC },
{ 0x5057, "ATI Rage 128 Pro AGP 2x (TMDS)", R128_CONF_GENERIC },
{ 0x5058, "ATI Rage 128 Pro AGP 4x (TMDS)", R128_CONF_GENERIC },
{ 0x5345, "ATI Rage 128 4x PCI", R128_CONF_GENERIC },
{ 0x5346, "ATI Rage 128 4x AGP 2x", R128_CONF_GENERIC },
{ 0x5347, "ATI Rage 128 4x AGP 4x", R128_CONF_GENERIC },
{ 0x5348, "ATI Rage 128 4x", R128_CONF_GENERIC },
{ 0x534B, "ATI Rage 128 4x PCI", R128_CONF_GENERIC },
{ 0x534C, "ATI Rage 128 4x AGP 2x", R128_CONF_GENERIC },
{ 0x534D, "ATI Rage 128 4x AGP 4x", R128_CONF_GENERIC },
{ 0x534E, "ATI Rage 128 4x", R128_CONF_GENERIC },
{ 0, 0, R128_CONF_UNSUPPORTED }
};

static struct r128_id* r128_card; /* R128 detected */

static unsigned r128_bus_device_func;

/* Registers */
#define R128_CLOCK_CNTL_DATA 0x000c
#define R128_CLOCK_CNTL_INDEX 0x0008
#define R128_PLL_WR_EN (1 << 7)
#define R128_PLL_DIV_SEL (3 << 8)
#define R128_CRTC_CRNT_FRAME 0x0214
#define R128_CRTC_DEBUG 0x021c
#define R128_CRTC_EXT_CNTL 0x0054
#define R128_CRTC_VGA_XOVERSCAN (1 << 0)
#define R128_VGA_ATI_LINEAR (1 << 3)
#define R128_XCRT_CNT_EN (1 << 6)
#define R128_CRTC_HSYNC_DIS (1 << 8)
#define R128_CRTC_VSYNC_DIS (1 << 9)
#define R128_CRTC_DISPLAY_DIS (1 << 10)
#define R128_CRTC_EXT_CNTL_DPMS_BYTE 0x0055
#define R128_CRTC_HSYNC_DIS_BYTE (1 << 0)
#define R128_CRTC_VSYNC_DIS_BYTE (1 << 1)
#define R128_CRTC_DISPLAY_DIS_BYTE (1 << 2)
#define R128_CRTC_GEN_CNTL 0x0050
#define R128_CRTC_DBL_SCAN_EN (1 << 0)
#define R128_CRTC_INTERLACE_EN (1 << 1)
#define R128_CRTC_CUR_EN (1 << 16)
#define R128_CRTC_CUR_MODE_MASK (7 << 17)
#define R128_CRTC_EXT_DISP_EN (1 << 24)
#define R128_CRTC_EN (1 << 25)
#define R128_CRTC_GUI_TRIG_VLINE 0x0218
#define R128_CRTC_H_SYNC_STRT_WID 0x0204
#define R128_CRTC_H_SYNC_POL (1 << 23)
#define R128_CRTC_H_TOTAL_DISP 0x0200
#define R128_CRTC_OFFSET 0x0224
#define R128_CRTC_OFFSET_CNTL 0x0228
#define R128_CRTC_PITCH 0x022c
#define R128_CRTC_STATUS 0x005c
#define R128_CRTC_VBLANK_SAVE (1 << 1)
#define R128_CRTC_V_SYNC_STRT_WID 0x020c
#define R128_CRTC_V_SYNC_POL (1 << 23)
#define R128_CRTC_V_TOTAL_DISP 0x0208
#define R128_CRTC_VLINE_CRNT_VLINE 0x0210
#define R128_CRTC_CRNT_VLINE_MASK (0x7ff << 16)

#define R128_HTOTAL_CNTL 0x0009 /* PLL */

#define R128_PPLL_CNTL 0x0002 /* PLL */
#define R128_PPLL_RESET (1 << 0)
#define R128_PPLL_SLEEP (1 << 1)
#define R128_PPLL_ATOMIC_UPDATE_EN (1 << 16)
#define R128_PPLL_VGA_ATOMIC_UPDATE_EN (1 << 17)
#define R128_PPLL_DIV_0 0x0004 /* PLL */
#define R128_PPLL_DIV_1 0x0005 /* PLL */
#define R128_PPLL_DIV_2 0x0006 /* PLL */
#define R128_PPLL_DIV_3 0x0007 /* PLL */
#define R128_PPLL_FB3_DIV_MASK 0x07ff
#define R128_PPLL_POST3_DIV_MASK 0x00070000
#define R128_PPLL_REF_DIV 0x0003 /* PLL */
#define R128_PPLL_REF_DIV_MASK 0x03ff
#define R128_PPLL_ATOMIC_UPDATE_R (1 << 15) /* same as _W */
#define R128_PPLL_ATOMIC_UPDATE_W (1 << 15) /* same as _R */

/* Memory mapped register access macros */
#define R128INREG8(addr) _farpeekb(pci_MMIO_selector_get(), addr)
#define R128INREG16(addr) _farpeekw(pci_MMIO_selector_get(), addr)
#define R128INREG(addr) _farpeekl(pci_MMIO_selector_get(), addr)
#define R128OUTREG8(addr, val) _farpokeb(pci_MMIO_selector_get(), addr, val)
#define R128OUTREG16(addr, val) _farpokew(pci_MMIO_selector_get(), addr, val)
#define R128OUTREG(addr, val) _farpokel(pci_MMIO_selector_get(), addr, val)

#define R128OUTREGP(addr, val, mask) \
	do { \
	DWORD tmp = R128INREG(addr); \
	tmp &= (mask); \
	tmp |= (val); \
	R128OUTREG(addr, tmp); \
	} while (0)

#define R128OUTPLL(addr, val) \
	do { \
	R128OUTREG8(R128_CLOCK_CNTL_INDEX, ((addr) & 0x1f) | R128_PLL_WR_EN); \
	R128OUTREG(R128_CLOCK_CNTL_DATA, val); \
	} while (0)

#define R128OUTPLLP(addr, val, mask) \
	do { \
	DWORD tmp = r128_pll_get(addr); \
	tmp &= (mask); \
	tmp |= (val); \
	R128OUTPLL(addr, tmp); \
	} while (0)

static int r128_pll_get(int addr)
{ 
	R128OUTREG8(R128_CLOCK_CNTL_INDEX, addr & 0x1f);
	return R128INREG(R128_CLOCK_CNTL_DATA);
}

/* BIOS PLL values */
static struct r128_pll_t {
	WORD reference_freq; /* .01 Mhz */
	WORD reference_div;
	DWORD min_pll_freq; /* .01 Mhz */
	DWORD max_pll_freq; /* .01 Mhz */
	WORD xclk;
} r128_pll;

/* Video registers */
struct r128_reg_t {
	/* Crtc registers */
	DWORD crtc_gen_cntl;
	DWORD crtc_h_total_disp;
	DWORD crtc_h_sync_strt_wid;
	DWORD crtc_v_total_disp;
	DWORD crtc_v_sync_strt_wid;

	/* Computed values for PLL */
	unsigned dot_clock_freq;
	unsigned pll_output_freq;
	unsigned feedback_div;
	unsigned post_div;

	/* PLL registers */
	DWORD ppll_ref_div;
	DWORD ppll_div_3;
	DWORD htotal_cntl;
};

static void r128_BIOS_read(unsigned offset, void* buffer, unsigned length)
{
	pci_BIOS_read(buffer, offset, length);
}

int r128_detect(void)
{
	int i;
	WORD bios_header;
	WORD pll_info_block;

	if (pci_detect()!=0) {
		CARD_LOG(("r128: PCI BIOS not installed.\n"));
		return 0;
	}

	for(i=0;r128_id_list[i].name;++i) {
		if (pci_find_device(0x00001002, r128_id_list[i].value, 0, &r128_bus_device_func)==0)
			break;
	}

	if(!r128_id_list[i].name){
		return 0;
	}

	CARD_LOG(("r128: found %s, device id %04x\n", r128_id_list[i].name, r128_id_list[i].value));

	if (!r128_id_list[i].supported) {
		CARD_LOG(("r128: card not supported\n"));
		return 0;
	}

	r128_card = r128_id_list + i;

	if (pci_BIOS_address_map(r128_bus_device_func)!=0) {
		CARD_LOG(( "r128: pci_BIOS_address_map error\n"));
		return 0;
	}

	r128_BIOS_read(0x48, (BYTE*)&bios_header, sizeof(bios_header));
	CARD_LOG(("r128: header at 0x%04x\n", (unsigned)bios_header));

	r128_BIOS_read(bios_header + 0x30, (BYTE*)&pll_info_block, sizeof(pll_info_block));
	CARD_LOG(("r128: PLL information at 0x%04x\n", (unsigned)pll_info_block));

	r128_BIOS_read(pll_info_block + 0x0e, (BYTE*)&r128_pll.reference_freq, sizeof(r128_pll.reference_freq));
	r128_BIOS_read(pll_info_block + 0x10, (BYTE*)&r128_pll.reference_div, sizeof(r128_pll.reference_div));
	r128_BIOS_read(pll_info_block + 0x12, (BYTE*)&r128_pll.min_pll_freq, sizeof(r128_pll.min_pll_freq));
	r128_BIOS_read(pll_info_block + 0x16, (BYTE*)&r128_pll.max_pll_freq, sizeof(r128_pll.max_pll_freq));
	r128_BIOS_read(pll_info_block + 0x08, (BYTE*)&r128_pll.xclk, sizeof(r128_pll.xclk));

	CARD_LOG(("r128: PLL parameters: rf:%d, rd:%d, min:%d, max:%d, xclk:%d\n", (unsigned)r128_pll.reference_freq, (unsigned)r128_pll.reference_div, (unsigned)r128_pll.min_pll_freq, (unsigned)r128_pll.max_pll_freq, (unsigned)r128_pll.xclk));

	pci_BIOS_address_unmap();

	if (pci_MMIO_address_map(r128_bus_device_func, 0x18, 0xFFFFFF00)!=0) {
		CARD_LOG(( "r128: pci_MMIO_address_map error\n"));
		return 0;
	}

	return 1;
}

/* Compute n/d with rounding. */
static int r128_div(int n, int d)
{
	return (n + (d / 2)) / d;
}

/*
						FeedBack_Div
	DotClock = Reference_Freq * ----------------------------
					Reference_Div * Post_Div
*/

static long r128_pll_init(struct r128_reg_t* save, long dot_clock)
{
	unsigned freq = dot_clock / 10000;

	STACK_DECL struct {
		int divider;
		int bitvalue;
	} *post_div, post_divs[] = {
	/*
         * From RAGE 128 VR/RAGE 128 GL Register
	 * Reference Manual (Technical Reference
	 * Manual P/N RRG-G04100-C Rev. 0.04), page
	 * 3-17 (PLL_DIV_[3:0]).
         */
	{ 1, 0 }, /* VCLK_SRC */
	{ 2, 1 }, /* VCLK_SRC/2 */
	{ 4, 2 }, /* VCLK_SRC/4 */
	{ 8, 3 }, /* VCLK_SRC/8 */
	{ 3, 4 }, /* VCLK_SRC/3 */
	/* bitvalue = 5 is reserved */
	{ 6, 6 }, /* VCLK_SRC/6 */
	{ 12, 7 }, /* VCLK_SRC/12 */
	{ 0, 0 }
	};
	
	if (freq > r128_pll.max_pll_freq)
		return -1;

	for (post_div = &post_divs[0]; post_div->divider; ++post_div) {
		save->pll_output_freq = post_div->divider * freq;
		if (save->pll_output_freq >= r128_pll.min_pll_freq && save->pll_output_freq <= r128_pll.max_pll_freq)
			break;
	}

	if (!post_div->divider) {
		post_div = post_divs + 6;
		save->pll_output_freq = post_div->divider * freq;
	}

	save->dot_clock_freq = freq;
	save->feedback_div = r128_div(r128_pll.reference_div * save->pll_output_freq, r128_pll.reference_freq);
	save->post_div = post_div->divider;

	CARD_LOG(("r128: c:%d, of:%d, fd:%d, pd:%d\n", (unsigned)save->dot_clock_freq, (unsigned)save->pll_output_freq, (unsigned)save->feedback_div, (unsigned)save->post_div));

	save->ppll_ref_div = r128_pll.reference_div;
	save->ppll_div_3 = (save->feedback_div | (post_div->bitvalue << 16));
	save->htotal_cntl = 0;

	return (double)r128_pll.reference_freq * save->feedback_div * 10000 / r128_pll.reference_div / save->post_div;
}

static void r128_crtc_init(struct r128_reg_t* save, card_crtc* cp, const card_mode* cm)
{
	int format;
	int hsync_start;
	int hsync_wid;
	int hsync_fudge;
	int vsync_wid;

	switch (cm->bits_per_pixel) {
		case 4: format = 1; hsync_fudge = 0; break;
		case 8: format = 2; hsync_fudge = 18; break;
		case 15: format = 3; hsync_fudge = 9; break; /* 555 */
		case 16: format = 4; hsync_fudge = 9; break; /* 565 */
		case 24: format = 5; hsync_fudge = 6; break; /* RGB */
		case 32: format = 6; hsync_fudge = 5; break; /* xRGB */
		default:
			return;
	}

	CARD_LOG(("r128: format = %d\n", format));
	
	save->crtc_gen_cntl = (R128_CRTC_EXT_DISP_EN | R128_CRTC_EN | (format << 8) | ((cp->doublescan) ? R128_CRTC_DBL_SCAN_EN : 0) | ((cp->interlace) ? R128_CRTC_INTERLACE_EN : 0));

	save->crtc_h_total_disp = ((((cp->HTotal / 8) - 1) & 0xffff) | (((cp->HDisp / 8) - 1) << 16));
	
	hsync_wid = (cp->HSEnd - cp->HSStart) / 8;
	if (!hsync_wid) hsync_wid = 1;
	if (hsync_wid > 0x3f) hsync_wid = 0x3f;
	
	hsync_start = cp->HSStart - 8 + hsync_fudge;
	
	save->crtc_h_sync_strt_wid = ((hsync_start & 0xfff) | (hsync_wid << 16) | ((cp->hpolarity) ? R128_CRTC_H_SYNC_POL : 0));

	save->crtc_v_total_disp = (((cp->VTotal - 1) & 0xffff) | ((cp->VDisp - 1) << 16));

	vsync_wid = cp->VSEnd - cp->VSStart;
	if (!vsync_wid) vsync_wid = 1;
	if (vsync_wid > 0x1f) vsync_wid = 0x1f;

	save->crtc_v_sync_strt_wid = (((cp->VSStart - 1) & 0xfff) | (vsync_wid << 16) | ((cp->vpolarity) ? R128_CRTC_V_SYNC_POL : 0));
}

static void r128_crtc_all_set(struct r128_reg_t* restore)
{ 
	R128OUTREG(R128_CRTC_GEN_CNTL, restore->crtc_gen_cntl);

	R128OUTREG(R128_CRTC_H_TOTAL_DISP, restore->crtc_h_total_disp);
	R128OUTREG(R128_CRTC_H_SYNC_STRT_WID, restore->crtc_h_sync_strt_wid);
	R128OUTREG(R128_CRTC_V_TOTAL_DISP, restore->crtc_v_total_disp);
	R128OUTREG(R128_CRTC_V_SYNC_STRT_WID, restore->crtc_v_sync_strt_wid);

}

static void r128_pll_wait_for_read_update_complete(void)
{
	while (r128_pll_get(R128_PPLL_REF_DIV) & R128_PPLL_ATOMIC_UPDATE_R);
}

static void r128_pll_write_update(void)
{ 
	R128OUTPLLP(R128_PPLL_REF_DIV, R128_PPLL_ATOMIC_UPDATE_W, 0xffff);
}

static void r128_pll_all_set(struct r128_reg_t* restore)
{
	CARD_LOG(("r128: pll set (with some waits)\n"));

	R128OUTREGP(R128_CLOCK_CNTL_INDEX, R128_PLL_DIV_SEL, 0xffff);
	
	R128OUTPLLP(R128_PPLL_CNTL, R128_PPLL_RESET | R128_PPLL_ATOMIC_UPDATE_EN | R128_PPLL_VGA_ATOMIC_UPDATE_EN, 0xffff);

	r128_pll_wait_for_read_update_complete();
	R128OUTPLLP(R128_PPLL_REF_DIV, restore->ppll_ref_div, ~R128_PPLL_REF_DIV_MASK);
	r128_pll_write_update();
	
	r128_pll_wait_for_read_update_complete();
	R128OUTPLLP(R128_PPLL_DIV_3, restore->ppll_div_3, ~R128_PPLL_FB3_DIV_MASK);
	r128_pll_write_update();
	R128OUTPLLP(R128_PPLL_DIV_3, restore->ppll_div_3, ~R128_PPLL_POST3_DIV_MASK);
	r128_pll_write_update();
	
	r128_pll_wait_for_read_update_complete();
	R128OUTPLL(R128_HTOTAL_CNTL, restore->htotal_cntl);
	r128_pll_write_update();

	R128OUTPLLP(R128_PPLL_CNTL, 0, ~R128_PPLL_RESET);
	
	CARD_LOG(("r128: wrote: 0x%08x 0x%08x 0x%08x (0x%08x)\n", (unsigned)restore->ppll_ref_div, (unsigned)restore->ppll_div_3, (unsigned)restore->htotal_cntl, (unsigned)r128_pll_get(R128_PPLL_CNTL)));
	CARD_LOG(("r128: wrote: rd=%d, fd=%d, pd=%d\n", (unsigned)restore->ppll_ref_div & R128_PPLL_REF_DIV_MASK, (unsigned)restore->ppll_div_3 & R128_PPLL_FB3_DIV_MASK, (unsigned)(restore->ppll_div_3 & R128_PPLL_POST3_DIV_MASK) >> 16));
}

static void r128_signal_disable(void)
{
	R128OUTREGP(R128_CRTC_EXT_CNTL, R128_CRTC_DISPLAY_DIS, ~R128_CRTC_DISPLAY_DIS);
}

static void r128_signal_enable(void)
{
	R128OUTREGP(R128_CRTC_EXT_CNTL, 0, ~R128_CRTC_DISPLAY_DIS);
}

void r128_reset(void)
{
	pci_MMIO_address_unmap();
}

int r128_set(const card_crtc* _cp, const card_mode* cm, const card_mode* co)
{
	card_crtc cp = *_cp;
	struct r128_reg_t reg;

	if (!card_compatible_mode(cm, co)) {
		CARD_LOG(("r128: incompatible mode\n"));
		return 0;
	}

	if (cp.interlace) {
		CARD_LOG(("r128: halve the clock for interlaced mode\n"));
		cp.dotclockHz /= 2;
	}

	CARD_LOG(("r128: clock requested:%.2f MHz\n", (double)cp.dotclockHz / 1000000 ));
	cp.dotclockHz = r128_pll_init(&reg, cp.dotclockHz);
	if (cp.dotclockHz<0) {
		CARD_LOG(("r128: clock out of range\n"));
		return 0;
	}
	CARD_LOG(("r128: clock selected:%.2f MHz\n", (double)cp.dotclockHz / 1000000 ));

	r128_crtc_init(&reg, &cp, cm);

	r128_signal_disable();
	r128_crtc_all_set(&reg);
	r128_pll_all_set(&reg);
	r128_signal_enable();

	return 1;
}

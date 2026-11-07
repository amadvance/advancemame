/*
Rage 128 chipset driver
*/

#include <stdlib.h>
#include <stdio.h>		
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include "vga.h"
#include "libvga.h"
#include "svgadriv.h"
#include "timing.h"
#include "vgaregs.h"
#include "interfac.h"
#include "vgapci.h"
#include "endianes.h"
#include "r128_reg.h"

static enum { Rage128=0, Radeon } chiptype; /* r128io needs to know */
static int dac6bits;
#include "r128io.h"

#ifdef __PPC
#define NO_BIOS
#else
#undef NO_BIOS
#endif

typedef int Bool;

static int r128_ramtype;
static int BusCntl, CRTOnly, HasPanelRegs;

typedef struct {
    uint16_t        reference_freq;
    uint16_t        reference_div;
    uint32_t        min_pll_freq;
    uint32_t        max_pll_freq;
    uint16_t        xclk;
} R128PLLRec, *R128PLLPtr;

typedef struct {
				/* Common registers */
    uint32_t     ovr_clr;
    uint32_t     ovr_wid_left_right;
    uint32_t     ovr_wid_top_bottom;
    uint32_t     ov0_scale_cntl;
    uint32_t     mpp_tb_config;
    uint32_t     mpp_gp_config;
    uint32_t     subpic_cntl;
    uint32_t     viph_control;
    uint32_t     i2c_cntl_1;
    uint32_t     gen_int_cntl;
    uint32_t     cap0_trig_cntl;
    uint32_t     cap1_trig_cntl;
    uint32_t     bus_cntl;
    uint32_t     bus_cntl1;
    uint32_t     mem_cntl;
    uint32_t     config_cntl;
    uint32_t     mem_vga_wp_sel;
    uint32_t     mem_vga_rp_sel;
    uint32_t     surface_cntl;
    uint32_t	 dac_cntl2;
    uint32_t	 crtc_more_cntl;
    uint32_t	 dac_ext_cntl;
    uint32_t	 grph_buf_cntl;
    uint32_t	 vga_buf_cntl;

				/* Other registers to save for VT switches */
    uint32_t     dp_datatype;
    uint32_t     gen_reset_cntl;
    uint32_t     clock_cntl_index;
    uint32_t     amcgpio_en_reg;
    uint32_t     amcgpio_mask;
				/* CRTC registers */
    uint32_t     crtc_gen_cntl;
    uint32_t     crtc_ext_cntl;
    uint32_t     dac_cntl;
    uint32_t     crtc_h_total_disp;
    uint32_t     crtc_h_sync_strt_wid;
    uint32_t     crtc_v_total_disp;
    uint32_t     crtc_v_sync_strt_wid;
    uint32_t     crtc_offset;
    uint32_t     crtc_offset_cntl;
    uint32_t     crtc_pitch;
				/* CRTC2 registers */
    uint32_t     crtc2_gen_cntl;
				/* Flat panel registers */
    uint32_t     fp_crtc_h_total_disp;
    uint32_t     fp_crtc_v_total_disp;
    uint32_t     fp_gen_cntl;
    uint32_t     fp_h_sync_strt_wid;
    uint32_t     fp_horz_stretch;
    uint32_t     fp_panel_cntl;
    uint32_t     fp_v_sync_strt_wid;
    uint32_t     fp_vert_stretch;
    uint32_t     lvds_gen_cntl;
    uint32_t     tmds_crc;
				/* Computed values for PLL */
    uint32_t     dot_clock_freq;
    uint32_t     pll_output_freq;
    int        feedback_div;
    int        post_div;
				/* PLL registers */
    uint32_t     ppll_ref_div;
    uint32_t     ppll_div_3;
    uint32_t     htotal_cntl;
				/* DDA register */
    uint32_t     dda_config;
    uint32_t     dda_on_off;
    uint32_t     vga_dda_config;
    uint32_t     vga_dda_on_off;
				/* Pallet */
    Bool       palette_valid;
    uint32_t     palette[256];
} R128SaveRec, *R128SavePtr;

typedef struct {        /* All values in XCLKS    */
    int  ML;            /* Memory Read Latency    */
    int  MB;            /* Memory Burst Length    */
    int  Trcd;          /* RAS to CAS delay       */
    int  Trp;           /* RAS percentage         */
    int  Twr;           /* Write Recovery         */
    int  CL;            /* CAS Latency            */
    int  Tr2w;          /* Read to Write Delay    */
    int  Rloop;         /* Loop Latency           */
    int  Rloop_fudge;   /* Add to ML to get Rloop */
    char *name;
} R128RAMRec, *R128RAMPtr;

#define R128_TOTAL_REGS (VGA_TOTAL_REGS + sizeof(R128SaveRec))
static int R128MinBits(int val)
{
    int bits;

    if (!val) return 1;
    for (bits = 0; val; val >>= 1, ++bits);
    return bits;
}
static int R128Div(int n, int d)
{
    return (n + (d / 2)) / d;
}
static R128PLLRec pll;

static R128RAMRec ram[] = {        /* Memory Specifications
				   From RAGE 128 Software Development
				   Manual (Technical Reference Manual P/N
				   SDK-G04000 Rev 0.01), page 3-21.  */
    { 4, 4, 3, 3, 1, 3, 1, 16, 12, "128-bit SDR SGRAM 1:1" },
    { 4, 8, 3, 3, 1, 3, 1, 17, 13, "64-bit SDR SGRAM 1:1" },
    { 4, 4, 1, 2, 1, 2, 1, 16, 12, "64-bit SDR SGRAM 2:1" },
    { 4, 4, 3, 3, 2, 3, 1, 16, 12, "64-bit DDR SGRAM" },
};

static unsigned R128INPLL(int addr)
{
    OUTREG8(R128_CLOCK_CNTL_INDEX, addr & 0x1f);
    return INREG(R128_CLOCK_CNTL_DATA);
}

static void R128WaitForVerticalSync(void)
{
    volatile int i;

    OUTREG(R128_GEN_INT_STATUS, R128_VSYNC_INT_AK);
    for (i = 0; i < R128_TIMEOUT; i++) {
	if (INREG(R128_GEN_INT_STATUS) & R128_VSYNC_INT) break;
    }
}

/* Blank screen. */
static void R128Blank(void)
{
    OUTREGP(R128_CRTC_EXT_CNTL, R128_CRTC_DISPLAY_DIS, ~R128_CRTC_DISPLAY_DIS);
}

/* Unblank screen. */
static void R128Unblank(void)
{
    OUTREGP(R128_CRTC_EXT_CNTL, 0, ~R128_CRTC_DISPLAY_DIS);
}

static void R128RestoreCommonRegisters(R128SavePtr restore)
{
    OUTREG(R128_OVR_CLR,              restore->ovr_clr);
    OUTREG(R128_OVR_WID_LEFT_RIGHT,   restore->ovr_wid_left_right);
    OUTREG(R128_OVR_WID_TOP_BOTTOM,   restore->ovr_wid_top_bottom);
    OUTREG(R128_OV0_SCALE_CNTL,       restore->ov0_scale_cntl);
    OUTREG(R128_MPP_TB_CONFIG,        restore->mpp_tb_config );
    OUTREG(R128_MPP_GP_CONFIG,        restore->mpp_gp_config );
    OUTREG(R128_SUBPIC_CNTL,          restore->subpic_cntl);
    OUTREG(R128_VIPH_CONTROL,         restore->viph_control);
    OUTREG(R128_I2C_CNTL_1,           restore->i2c_cntl_1);
    OUTREG(R128_GEN_INT_CNTL,         restore->gen_int_cntl);
    OUTREG(R128_CAP0_TRIG_CNTL,       restore->cap0_trig_cntl);
    OUTREG(R128_CAP1_TRIG_CNTL,       restore->cap1_trig_cntl);
    OUTREG(R128_BUS_CNTL,             restore->bus_cntl);
    OUTREG(R128_BUS_CNTL1,            restore->bus_cntl1);
    OUTREG(R128_CONFIG_CNTL,          restore->config_cntl);
    OUTREG(R128_MEM_VGA_WP_SEL,	      restore->mem_vga_wp_sel);
    OUTREG(R128_MEM_VGA_RP_SEL,	      restore->mem_vga_rp_sel);

    if(chiptype == Radeon) {
        OUTREG(RADEON_SURFACE_CNTL,   restore->surface_cntl);
        OUTREG(RADEON_DAC_CNTL2,     restore->dac_cntl2);
        OUTREG(RADEON_CRTC_MORE_CNTL,restore->crtc_more_cntl);
        OUTREG(RADEON_DAC_EXT_CNTL,  restore->dac_ext_cntl);
        OUTREG(RADEON_GRPH_BUF_CNTL, restore->grph_buf_cntl);
        OUTREG(RADEON_VGA_BUF_CNTL,  restore->vga_buf_cntl);
    }

}

/* Write CRTC registers. */
static void R128RestoreCrtcRegisters(R128SavePtr restore)
{
    OUTREG(R128_CRTC_GEN_CNTL,        restore->crtc_gen_cntl);

    OUTREGP(R128_CRTC_EXT_CNTL, restore->crtc_ext_cntl,
	    R128_CRTC_VSYNC_DIS | R128_CRTC_HSYNC_DIS | R128_CRTC_DISPLAY_DIS);

    OUTREGP(R128_DAC_CNTL, restore->dac_cntl,
	    R128_DAC_RANGE_CNTL | R128_DAC_BLANKING);

    OUTREG(R128_CRTC_H_TOTAL_DISP,    restore->crtc_h_total_disp);
    OUTREG(R128_CRTC_H_SYNC_STRT_WID, restore->crtc_h_sync_strt_wid);
    OUTREG(R128_CRTC_V_TOTAL_DISP,    restore->crtc_v_total_disp);
    OUTREG(R128_CRTC_V_SYNC_STRT_WID, restore->crtc_v_sync_strt_wid);
    OUTREG(R128_CRTC_OFFSET,          restore->crtc_offset);
    OUTREG(R128_CRTC_OFFSET_CNTL,     restore->crtc_offset_cntl);
    OUTREG(R128_CRTC_PITCH,           restore->crtc_pitch);
}

/* Write flat panel registers */
#if 0
static void R128RestoreFPRegisters(R128SavePtr restore)
{
    uint32_t        tmp;

    OUTREG(R128_CRTC2_GEN_CNTL,       restore->crtc2_gen_cntl);
    OUTREG(R128_FP_CRTC_H_TOTAL_DISP, restore->fp_crtc_h_total_disp);
    OUTREG(R128_FP_CRTC_V_TOTAL_DISP, restore->fp_crtc_v_total_disp);
    OUTREG(R128_FP_GEN_CNTL,          restore->fp_gen_cntl);
    OUTREG(R128_FP_H_SYNC_STRT_WID,   restore->fp_h_sync_strt_wid);
    OUTREG(R128_FP_HORZ_STRETCH,      restore->fp_horz_stretch);
    OUTREG(R128_FP_PANEL_CNTL,        restore->fp_panel_cntl);
    OUTREG(R128_FP_V_SYNC_STRT_WID,   restore->fp_v_sync_strt_wid);
    OUTREG(R128_FP_VERT_STRETCH,      restore->fp_vert_stretch);
    OUTREG(R128_TMDS_CRC,             restore->tmds_crc);

    tmp = INREG(R128_LVDS_GEN_CNTL);
    if ((tmp & (R128_LVDS_ON | R128_LVDS_BLON)) ==
	(restore->lvds_gen_cntl & (R128_LVDS_ON | R128_LVDS_BLON))) {
	OUTREG(R128_LVDS_GEN_CNTL, restore->lvds_gen_cntl);
    } else {
	if (restore->lvds_gen_cntl & (R128_LVDS_ON | R128_LVDS_BLON)) {
	    OUTREG(R128_LVDS_GEN_CNTL, restore->lvds_gen_cntl & ~R128_LVDS_BLON);
//	    usleep(R128PTR(pScrn)->PanelPwrDly * 1000);
	    OUTREG(R128_LVDS_GEN_CNTL, restore->lvds_gen_cntl);
	} else {
	    OUTREG(R128_LVDS_GEN_CNTL, restore->lvds_gen_cntl | R128_LVDS_BLON);
//	    usleep(R128PTR(pScrn)->PanelPwrDly * 1000);
	    OUTREG(R128_LVDS_GEN_CNTL, restore->lvds_gen_cntl);
	}
    }
}
#endif
static void R128PLLWaitForReadUpdateComplete(void)
{
    int i = 0;

    /* FIXME: Certain revisions of R300 can't recover here.  Not sure of
       the cause yet, but this workaround will mask the problem for now.
       Other chips usually will pass at the very first test, so the
       workaround shouldn't have any effect on them. */
    for (i = 0;
	 (i < 10000 &&
	  INPLL(R128_PPLL_REF_DIV) & R128_PPLL_ATOMIC_UPDATE_R);
	 i++);
}

static void R128PLLWriteUpdate(void)
{
    OUTPLLP(R128_PPLL_REF_DIV, R128_PPLL_ATOMIC_UPDATE_W, 0xffff);
}

/* Write PLL registers. */
static void RADEONRestorePLLRegisters(R128SavePtr restore)
{
    OUTPLLP(R128_VCLK_ECP_CNTL,
	    RADEON_VCLK_SRC_SEL_CPUCLK,
	    ~(RADEON_VCLK_SRC_SEL_MASK));

    OUTPLLP(R128_PPLL_CNTL,
	    R128_PPLL_RESET
	    | R128_PPLL_ATOMIC_UPDATE_EN
	    | R128_PPLL_VGA_ATOMIC_UPDATE_EN,
	    ~(R128_PPLL_RESET
	      | R128_PPLL_ATOMIC_UPDATE_EN
	      | R128_PPLL_VGA_ATOMIC_UPDATE_EN));

    OUTREGP(R128_CLOCK_CNTL_INDEX,
	    R128_PLL_DIV_SEL,
	    ~(R128_PLL_DIV_SEL));

    OUTPLLP(R128_PPLL_REF_DIV,
	    restore->ppll_ref_div,
	    ~R128_PPLL_REF_DIV_MASK);

    OUTPLLP(R128_PPLL_DIV_3,
	    restore->ppll_div_3,
	    ~R128_PPLL_FB3_DIV_MASK);

    OUTPLLP(R128_PPLL_DIV_3,
	    restore->ppll_div_3,
	    ~R128_PPLL_POST3_DIV_MASK);

    R128PLLWriteUpdate();
    R128PLLWaitForReadUpdateComplete();

    OUTPLL(R128_HTOTAL_CNTL, restore->htotal_cntl);

    OUTPLLP(R128_PPLL_CNTL,
	    0,
	    ~(R128_PPLL_RESET
	      | R128_PPLL_SLEEP
	      | R128_PPLL_ATOMIC_UPDATE_EN
	      | R128_PPLL_VGA_ATOMIC_UPDATE_EN));

    usleep(5000); /* Let the clock to lock */

    OUTPLLP(R128_VCLK_ECP_CNTL,
	    RADEON_VCLK_SRC_SEL_PPLLCLK,
	    ~(RADEON_VCLK_SRC_SEL_MASK));
}

static void R128RestorePLLRegisters(R128SavePtr restore)
{
    OUTREGP(R128_CLOCK_CNTL_INDEX, R128_PLL_DIV_SEL, 0xffff);

    OUTPLLP(
	    R128_PPLL_CNTL,
	    R128_PPLL_RESET
	    | R128_PPLL_ATOMIC_UPDATE_EN,
	    0xffff);

    R128PLLWaitForReadUpdateComplete();
    OUTPLLP(R128_PPLL_REF_DIV,
	    restore->ppll_ref_div, ~R128_PPLL_REF_DIV_MASK);
    R128PLLWriteUpdate();

    R128PLLWaitForReadUpdateComplete();
    OUTPLLP(R128_PPLL_DIV_3,
	    restore->ppll_div_3, ~R128_PPLL_FB3_DIV_MASK);
    R128PLLWriteUpdate();
    OUTPLLP(R128_PPLL_DIV_3,
	    restore->ppll_div_3, ~R128_PPLL_POST3_DIV_MASK);
    R128PLLWriteUpdate();

    R128PLLWaitForReadUpdateComplete();
    OUTPLL(R128_HTOTAL_CNTL, restore->htotal_cntl);
    R128PLLWriteUpdate();

    OUTPLLP( R128_PPLL_CNTL, 0, ~R128_PPLL_RESET);

}

/* Write DDA registers. */
static void R128RestoreDDARegisters(R128SavePtr restore)
{
    OUTREG(R128_DDA_CONFIG, restore->dda_config);
    OUTREG(R128_DDA_ON_OFF, restore->dda_on_off);
//    OUTREG(R128_VGA_DDA_CONFIG, restore->vga_dda_config);
//    OUTREG(R128_VGA_DDA_ON_OFF, restore->vga_dda_on_off);
}

/* Write palette data. */
static void R128RestorePalette( R128SavePtr restore)
{
    int           i;

if (!restore->palette_valid) return;

    /* Select palette 0 (main CRTC) if using FP-enabled chip */
//    if (info->HasPanelRegs) PAL_SELECT(0);

    OUTPAL_START(0);
    for (i = 0; i < 256; i++) OUTPAL_NEXT_uint32_t(restore->palette[i]);
}

/* Write out state to define a new video mode.  */
static void R128RestoreMode(R128SavePtr restore)
{
    R128Blank();

    OUTREG(R128_AMCGPIO_MASK,     restore->amcgpio_mask);
    OUTREG(R128_AMCGPIO_EN_REG,   restore->amcgpio_en_reg);
    OUTREG(R128_CLOCK_CNTL_INDEX, restore->clock_cntl_index);
#if 0 /* works without, and it causes problems with it */
    OUTREG(R128_GEN_RESET_CNTL,   restore->gen_reset_cntl);
#endif
    OUTREG(R128_DP_DATATYPE,      restore->dp_datatype);

    R128RestoreCommonRegisters( restore);
    R128RestoreCrtcRegisters( restore);
//    if (info->HasPanelRegs)
//	R128RestoreFPRegisters(restore);
//    if (!info->HasPanelRegs || info->CRTOnly)
    switch(chiptype) {
        case Rage128:
            R128RestorePLLRegisters(restore);
            break;
        case Radeon:
            RADEONRestorePLLRegisters(restore);
            break;
    }
            
    if(chiptype == Rage128) {
	R128RestoreDDARegisters(restore);
    }
    R128RestorePalette(restore);
}

/* Read common registers. */
static void R128SaveCommonRegisters(R128SavePtr save)
{
    save->ovr_clr            = INREG(R128_OVR_CLR);
    save->ovr_wid_left_right = INREG(R128_OVR_WID_LEFT_RIGHT);
    save->ovr_wid_top_bottom = INREG(R128_OVR_WID_TOP_BOTTOM);
    save->ov0_scale_cntl     = INREG(R128_OV0_SCALE_CNTL);
    save->mpp_tb_config      = INREG(R128_MPP_TB_CONFIG);
    save->mpp_gp_config      = INREG(R128_MPP_GP_CONFIG);
    save->subpic_cntl        = INREG(R128_SUBPIC_CNTL);
    save->viph_control       = INREG(R128_VIPH_CONTROL);
    save->i2c_cntl_1         = INREG(R128_I2C_CNTL_1);
    save->gen_int_cntl       = INREG(R128_GEN_INT_CNTL);
    save->cap0_trig_cntl     = INREG(R128_CAP0_TRIG_CNTL);
    save->cap1_trig_cntl     = INREG(R128_CAP1_TRIG_CNTL);
    save->bus_cntl           = INREG(R128_BUS_CNTL);
    save->bus_cntl1          = INREG(R128_BUS_CNTL1);
    save->mem_cntl           = INREG(R128_MEM_CNTL);
    save->config_cntl        = INREG(R128_CONFIG_CNTL);
    save->mem_vga_wp_sel     = INREG(R128_MEM_VGA_WP_SEL);
    save->mem_vga_rp_sel     = INREG(R128_MEM_VGA_RP_SEL);

    if(chiptype==Radeon) {
    	save->surface_cntl   = INREG(RADEON_SURFACE_CNTL);
        save->dac_cntl2	     = INREG(RADEON_DAC_CNTL2);
        save->crtc_more_cntl = INREG(RADEON_CRTC_MORE_CNTL);
        save->dac_ext_cntl   = INREG(RADEON_DAC_EXT_CNTL);
        save->grph_buf_cntl  = INREG(RADEON_GRPH_BUF_CNTL);
        save->vga_buf_cntl  =  INREG(RADEON_VGA_BUF_CNTL);
    }
}

/* Read CRTC registers. */
static void R128SaveCrtcRegisters(R128SavePtr save)
{
    save->crtc_gen_cntl        = INREG(R128_CRTC_GEN_CNTL);
    save->crtc_ext_cntl        = INREG(R128_CRTC_EXT_CNTL);
    save->dac_cntl             = INREG(R128_DAC_CNTL);
    save->crtc_h_total_disp    = INREG(R128_CRTC_H_TOTAL_DISP);
    save->crtc_h_sync_strt_wid = INREG(R128_CRTC_H_SYNC_STRT_WID);
    save->crtc_v_total_disp    = INREG(R128_CRTC_V_TOTAL_DISP);
    save->crtc_v_sync_strt_wid = INREG(R128_CRTC_V_SYNC_STRT_WID);
    save->crtc_offset          = INREG(R128_CRTC_OFFSET);
    save->crtc_offset_cntl     = INREG(R128_CRTC_OFFSET_CNTL);
    save->crtc_pitch           = INREG(R128_CRTC_PITCH);
}

#if 0
/* Read flat panel registers */
static void R128SaveFPRegisters(R128SavePtr save)
{
    save->crtc2_gen_cntl       = INREG(R128_CRTC2_GEN_CNTL);
    save->fp_crtc_h_total_disp = INREG(R128_FP_CRTC_H_TOTAL_DISP);
    save->fp_crtc_v_total_disp = INREG(R128_FP_CRTC_V_TOTAL_DISP);
    save->fp_gen_cntl          = INREG(R128_FP_GEN_CNTL);
    save->fp_h_sync_strt_wid   = INREG(R128_FP_H_SYNC_STRT_WID);
    save->fp_horz_stretch      = INREG(R128_FP_HORZ_STRETCH);
    save->fp_panel_cntl        = INREG(R128_FP_PANEL_CNTL);
    save->fp_v_sync_strt_wid   = INREG(R128_FP_V_SYNC_STRT_WID);
    save->fp_vert_stretch      = INREG(R128_FP_VERT_STRETCH);
    save->lvds_gen_cntl        = INREG(R128_LVDS_GEN_CNTL);
    save->tmds_crc             = INREG(R128_TMDS_CRC);
}
#endif

/* Read PLL registers. */
static void R128SavePLLRegisters(R128SavePtr save)
{
    save->ppll_ref_div         = INPLL(R128_PPLL_REF_DIV);
    save->ppll_div_3           = INPLL(R128_PPLL_DIV_3);
    save->htotal_cntl          = INPLL(R128_HTOTAL_CNTL);
}

/* Read DDA registers. */
static void R128SaveDDARegisters(R128SavePtr save)
{
    save->dda_config           = INREG(R128_DDA_CONFIG);
    save->dda_on_off           = INREG(R128_DDA_ON_OFF);
    save->vga_dda_config           = INREG(R128_VGA_DDA_CONFIG);
    save->vga_dda_on_off           = INREG(R128_VGA_DDA_ON_OFF);
}

/* Read palette data. */
static void R128SavePalette(R128SavePtr save)
{
    int           i;

    /* Select palette 0 (main CRTC) if using FP-enabled chip */
//    if (info->HasPanelRegs) PAL_SELECT(0);

    INPAL_START(0);
    for (i = 0; i < 256; i++) save->palette[i] = INPAL_NEXT();
    save->palette_valid = 1;
}

/* Save state that defines current video mode. */
static void R128SaveMode(R128SavePtr save)
{
    R128SaveCommonRegisters(save);
    R128SaveCrtcRegisters(save);
//    if (R128PTR(pScrn)->HasPanelRegs)
//	R128SaveFPRegisters(save);
    R128SavePLLRegisters(save);
	if(chiptype == Rage128)
    	R128SaveDDARegisters(save);
    R128SavePalette(save);

    save->dp_datatype      = INREG(R128_DP_DATATYPE);
    save->gen_reset_cntl   = INREG(R128_GEN_RESET_CNTL);
    save->clock_cntl_index = INREG(R128_CLOCK_CNTL_INDEX);
    save->amcgpio_en_reg   = INREG(R128_AMCGPIO_EN_REG);
    save->amcgpio_mask     = INREG(R128_AMCGPIO_MASK);
}

static void R128InitCommonRegisters(R128SavePtr save)
{
    save->ovr_clr            = 0;
    save->ovr_wid_left_right = 0;
    save->ovr_wid_top_bottom = 0;
    save->ov0_scale_cntl     = 0;
    save->mpp_tb_config      = 0;
    save->mpp_gp_config      = 0;
    save->subpic_cntl        = 0;
    save->viph_control       = 0;
    save->i2c_cntl_1         = 0;
    save->gen_int_cntl       = 0;
    save->cap0_trig_cntl     = 0;
    save->cap1_trig_cntl     = 0;
    save->mem_vga_wp_sel     = 0;
    save->mem_vga_rp_sel     = 0;
    save->config_cntl        = INREG(R128_CONFIG_CNTL);
    save->bus_cntl           = BusCntl;
    save->bus_cntl1          = INREG(R128_BUS_CNTL1);
    if(chiptype == Radeon) {
        if(save->bus_cntl & RADEON_BUS_READ_BURST) 
            save->bus_cntl |=RADEON_BUS_RD_DISCARD_EN;
    }

    save->amcgpio_en_reg   = INREG(R128_AMCGPIO_EN_REG);
    save->amcgpio_mask     = INREG(R128_AMCGPIO_MASK);

    /*
     * If bursts are enabled, turn on discards and aborts
     */
    if (save->bus_cntl & (R128_BUS_WRT_BURST|R128_BUS_READ_BURST))
	save->bus_cntl |= R128_BUS_RD_DISCARD_EN | R128_BUS_RD_ABORT_EN;
}

/* Define CRTC registers for requested video mode. */
static Bool R128InitCrtcRegisters(R128SavePtr save,
   				  ModeTiming *mode, ModeInfo *info)
{
    int    format;
    int    hsync_start;
    int    hsync_wid;
    int    hsync_fudge;
    int    vsync_wid;
    int    bytpp;
    int    hsync_fudge_default[] = { 0x00, 0x12, 0x09, 0x09, 0x06, 0x05 };
    int    hsync_fudge_fp[]      = { 0x12, 0x11, 0x09, 0x09, 0x05, 0x05 };
    int    hsync_fudge_fp_crt[]  = { 0x12, 0x10, 0x08, 0x08, 0x04, 0x04 };

    dac6bits=0;

    switch (info->bitsPerPixel) {
    case 4:  format = 1; bytpp = 0; dac6bits = 1; break;
    case 8:  format = 2; bytpp = 1; dac6bits = 1; break;
    case 16:
            if(info->greenWeight==5) 
               format = 3; else format = 4;
            bytpp = 2; 
            break;
    case 24: format = 5; bytpp = 3; break;      /*  RGB */
    case 32: format = 6; bytpp = 4; break;      /* xRGB */
    default:
	return 0;
    }
    
    if (HasPanelRegs)
	if (CRTOnly) hsync_fudge = hsync_fudge_fp_crt[format-1];
	else               hsync_fudge = hsync_fudge_fp[format-1];
    else
                           hsync_fudge = hsync_fudge_default[format-1];

    save->crtc_gen_cntl = (R128_CRTC_EXT_DISP_EN
			  | R128_CRTC_EN
			  | (format << 8)
			  | ((mode->flags & DOUBLESCAN)
			     ? R128_CRTC_DBL_SCAN_EN
			     : 0)
			  | ((mode->flags & INTERLACED)
			     ? R128_CRTC_INTERLACE_EN
			     : 0));

    save->crtc_ext_cntl = R128_VGA_ATI_LINEAR | R128_XCRT_CNT_EN | R128_VGA_MEM_PS_EN;
    if(chiptype == Radeon) save->crtc_ext_cntl |= R128_CRTC_CRT_ON;
    save->dac_cntl      = (R128_DAC_MASK_ALL
			   | R128_DAC_VGA_ADR_EN
			   | (dac6bits ? 0 : R128_DAC_8BIT_EN));

    save->crtc_h_total_disp = ((((mode->CrtcHTotal / 8) - 1) & 0xffff)
			      | (((mode->CrtcHDisplay / 8) - 1) << 16));

    hsync_wid = (mode->CrtcHSyncEnd - mode->CrtcHSyncStart) / 8;
    if (!hsync_wid)       hsync_wid = 1;
    if (hsync_wid > 0x3f) hsync_wid = 0x3f;

    hsync_start = mode->CrtcHSyncStart - 8 + hsync_fudge;

    save->crtc_h_sync_strt_wid = ((hsync_start & 0xfff)
				 | (hsync_wid << 16)
				 | ((mode->flags & NHSYNC)
				    ? R128_CRTC_H_SYNC_POL
				    : 0));

#if 1
				/* This works for double scan mode. */
    save->crtc_v_total_disp = (((mode->CrtcVTotal - 1) & 0xffff)
			      | ((mode->CrtcVDisplay - 1) << 16));
#else
				/* This is what cce/nbmode.c example code
				   does -- is this correct? */
    save->crtc_v_total_disp = (((mode->CrtcVTotal - 1) & 0xffff)
			      | ((mode->CrtcVDisplay
				  * ((mode->Flags & DOUBLESCAN) ? 2 : 1) - 1)
				 << 16));
#endif

    vsync_wid = mode->CrtcVSyncEnd - mode->CrtcVSyncStart;
    if (!vsync_wid)       vsync_wid = 1;
    if (vsync_wid > 0x1f) vsync_wid = 0x1f;

    save->crtc_v_sync_strt_wid = (((mode->CrtcVSyncStart - 1) & 0xfff)
				 | (vsync_wid << 16)
				 | ((mode->flags & NVSYNC)
				    ? R128_CRTC_V_SYNC_POL
				    : 0));
    save->crtc_offset      = 0;
	/* The register reference says this bit should be 0,
	 * but on my RV280 it appears as if it should be 1
	 * for changing offset at vertical blank. */
    save->crtc_offset_cntl = 1<<16; 

	save->crtc_pitch       = info->width / 8;

    save->config_cntl |= R128_CFG_VGA_RAM_EN;
    
    if(chiptype == Radeon) {
        save->crtc_pitch |= save->crtc_pitch<<16;
        save->surface_cntl = RADEON_SURF_TRANSLATION_DIS;

        save->dac_cntl2	     = INREG(RADEON_DAC_CNTL2);
        save->crtc_more_cntl = INREG(RADEON_CRTC_MORE_CNTL);
        save->dac_ext_cntl   = INREG(RADEON_DAC_EXT_CNTL);
        save->grph_buf_cntl  = INREG(RADEON_GRPH_BUF_CNTL);
        save->vga_buf_cntl   =  INREG(RADEON_VGA_BUF_CNTL);

    }

#ifdef __PPC
    /* Change the endianness of the aperture */
    switch (info->bitsPerPixel) {
    case 15:
    case 16: save->config_cntl |= APER_0_BIG_ENDIAN_16BPP_SWAP; break;
    case 32: save->config_cntl |= APER_0_BIG_ENDIAN_32BPP_SWAP; break;
    default: break;
    }
#endif

    return 1;
}

#if 0
/* Define CRTC registers for requested video mode. */
static void R128InitFPRegisters(R128SavePtr orig, R128SavePtr save,
 				ModeTiming *mode, ModeInfo *info)
{
#if 0
    int   xres = mode->CrtcHDisplay;
    int   yres = mode->CrtcVDisplay;
    float Hratio, Vratio;

    if (CRTOnly) {
	save->crtc_ext_cntl  |= R128_CRTC_CRT_ON;
	save->crtc2_gen_cntl  = 0;
	save->fp_gen_cntl     = orig->fp_gen_cntl;
	save->fp_gen_cntl    &= ~(R128_FP_FPON |
				  R128_FP_CRTC_USE_SHADOW_VEND |
				  R128_FP_CRTC_HORZ_DIV2_EN |
				  R128_FP_CRTC_HOR_CRT_DIV2_DIS |
				  R128_FP_USE_SHADOW_EN);
	save->fp_gen_cntl    |= (R128_FP_SEL_CRTC2 |
				 R128_FP_CRTC_DONT_SHADOW_VPAR);
	save->fp_panel_cntl   = orig->fp_panel_cntl & ~R128_FP_DIGON;
	save->lvds_gen_cntl   = orig->lvds_gen_cntl & ~(R128_LVDS_ON |
							R128_LVDS_BLON);
	return;
    }


    if (xres > info->PanelXRes) xres = info->PanelXRes;
    if (yres > info->PanelYRes) yres = info->PanelYRes;

    Hratio = (float)xres/(float)info->PanelXRes;
    Vratio = (float)yres/(float)info->PanelYRes;

    save->fp_horz_stretch =
	(((((int)(Hratio * R128_HORZ_STRETCH_RATIO_MAX + 0.5))
	   & R128_HORZ_STRETCH_RATIO_MASK) << R128_HORZ_STRETCH_RATIO_SHIFT) |
	 (orig->fp_horz_stretch & (R128_HORZ_PANEL_SIZE |
				   R128_HORZ_FP_LOOP_STRETCH |
				   R128_HORZ_STRETCH_RESERVED)));
    save->fp_horz_stretch &= ~R128_HORZ_AUTO_RATIO_FIX_EN;
    if (Hratio == 1.0) save->fp_horz_stretch &= ~(R128_HORZ_STRETCH_BLEND |
						  R128_HORZ_STRETCH_ENABLE);
    else               save->fp_horz_stretch |=  (R128_HORZ_STRETCH_BLEND |
						  R128_HORZ_STRETCH_ENABLE);

    save->fp_vert_stretch =
	(((((int)(Vratio * R128_VERT_STRETCH_RATIO_MAX + 0.5))
	   & R128_VERT_STRETCH_RATIO_MASK) << R128_VERT_STRETCH_RATIO_SHIFT) |
	 (orig->fp_vert_stretch & (R128_VERT_PANEL_SIZE |
				   R128_VERT_STRETCH_RESERVED)));
    save->fp_vert_stretch &= ~R128_VERT_AUTO_RATIO_EN;
    if (Vratio == 1.0) save->fp_vert_stretch &= ~(R128_VERT_STRETCH_ENABLE |
						  R128_VERT_STRETCH_BLEND);
    else               save->fp_vert_stretch |=  (R128_VERT_STRETCH_ENABLE |
						  R128_VERT_STRETCH_BLEND);

    save->fp_gen_cntl = (orig->fp_gen_cntl & ~(R128_FP_SEL_CRTC2 |
					       R128_FP_CRTC_USE_SHADOW_VEND |
					       R128_FP_CRTC_HORZ_DIV2_EN |
					       R128_FP_CRTC_HOR_CRT_DIV2_DIS |
					       R128_FP_USE_SHADOW_EN));
    if (orig->fp_gen_cntl & R128_FP_DETECT_SENSE) {
	save->fp_gen_cntl |= (R128_FP_CRTC_DONT_SHADOW_VPAR |
			      R128_FP_TDMS_EN);
    }

    save->fp_panel_cntl        = orig->fp_panel_cntl;
    save->lvds_gen_cntl        = orig->lvds_gen_cntl;

    save->tmds_crc             = orig->tmds_crc;

    /* Disable CRT output by disabling CRT output and setting the CRT
       DAC to use CRTC2, which we set to 0's.  In the future, we will
       want to use the dual CRTC capabilities of the R128 to allow both
       the flat panel and external CRT to either simultaneously display
       the same image or display two different images. */
    save->crtc_ext_cntl  &= ~R128_CRTC_CRT_ON;
    save->dac_cntl       |= R128_DAC_CRT_SEL_CRTC2;
    save->crtc2_gen_cntl  = 0;

    /* WARNING: Be careful about turning on the flat panel */
#if 1
    save->lvds_gen_cntl  |= (R128_LVDS_ON | R128_LVDS_BLON);
#else
    save->fp_panel_cntl  |= (R128_FP_DIGON | R128_FP_BLON);
    save->fp_gen_cntl    |= (R128_FP_FPON);
#endif

    save->fp_crtc_h_total_disp = save->crtc_h_total_disp;
    save->fp_crtc_v_total_disp = save->crtc_v_total_disp;
    save->fp_h_sync_strt_wid   = save->crtc_h_sync_strt_wid;
    save->fp_v_sync_strt_wid   = save->crtc_v_sync_strt_wid;
#endif
}
#endif

/* Define PLL registers for requested video mode. */
static void RADEONInitPLLRegisters(R128SavePtr save, R128PLLPtr pll,
				 double dot_clock)
{
    unsigned long freq = dot_clock * 100;
    struct {
	int divider;
	int bitvalue;
    } *post_div,
      post_divs[]   = {
				/* From RAGE 128 VR/RAGE 128 GL Register
				   Reference Manual (Technical Reference
				   Manual P/N RRG-G04100-C Rev. 0.04), page
				   3-17 (PLL_DIV_[3:0]).  */
	{  1, 0 },              /* VCLK_SRC                 */
	{  2, 1 },              /* VCLK_SRC/2               */
	{  4, 2 },              /* VCLK_SRC/4               */
	{  8, 3 },              /* VCLK_SRC/8               */

	{  3, 4 },              /* VCLK_SRC/3               */
	{ 16, 5 },
        {  6, 6 },              /* VCLK_SRC/6               */
	{ 12, 7 },              /* VCLK_SRC/12              */
	{  0, 0 }
    };

    if (freq > pll->max_pll_freq)      freq = pll->max_pll_freq;

    for (post_div = &post_divs[0]; post_div->divider; ++post_div) {
	save->pll_output_freq = post_div->divider * freq;
	if (save->pll_output_freq >= pll->min_pll_freq
	    && save->pll_output_freq <= pll->max_pll_freq) break;
    }

	if (!post_div->divider) {
		post_div = post_divs + 5;
		save->pll_output_freq = post_div->divider * freq;
	}

    save->dot_clock_freq = freq;
    save->feedback_div   = R128Div(pll->reference_div * save->pll_output_freq,
				   pll->reference_freq);
    save->post_div       = post_div->divider;

    save->ppll_ref_div   = pll->reference_div;
    save->ppll_div_3     = (save->feedback_div | (post_div->bitvalue << 16));
    save->htotal_cntl    = 0;
}

static void R128InitPLLRegisters(R128SavePtr save, R128PLLPtr pll,
				 double dot_clock)
{
    unsigned long freq = dot_clock * 100;
    struct {
	int divider;
	int bitvalue;
    } *post_div,
      post_divs[]   = {
				/* From RAGE 128 VR/RAGE 128 GL Register
				   Reference Manual (Technical Reference
				   Manual P/N RRG-G04100-C Rev. 0.04), page
				   3-17 (PLL_DIV_[3:0]).  */
	{  1, 0 },              /* VCLK_SRC                 */
	{  2, 1 },              /* VCLK_SRC/2               */
	{  4, 2 },              /* VCLK_SRC/4               */
	{  8, 3 },              /* VCLK_SRC/8               */

	{  3, 4 },              /* VCLK_SRC/3               */
				/* bitvalue = 5 is reserved */
	{  6, 6 },              /* VCLK_SRC/6               */
	{ 12, 7 },              /* VCLK_SRC/12              */
	{  0, 0 }
    };

    if (freq > pll->max_pll_freq)      freq = pll->max_pll_freq;

    for (post_div = &post_divs[0]; post_div->divider; ++post_div) {
	save->pll_output_freq = post_div->divider * freq;
	if (save->pll_output_freq >= pll->min_pll_freq
	    && save->pll_output_freq <= pll->max_pll_freq) break;
    }

	if (!post_div->divider) {
		post_div = post_divs + 6;
		save->pll_output_freq = post_div->divider * freq;
	}

    save->dot_clock_freq = freq;
    save->feedback_div   = R128Div(pll->reference_div * save->pll_output_freq,
				   pll->reference_freq);
    save->post_div       = post_div->divider;

    save->ppll_ref_div   = pll->reference_div;
    save->ppll_div_3     = (save->feedback_div | (post_div->bitvalue << 16));
    save->htotal_cntl    = 0;
}

/* Define DDA registers for requested video mode. */
static Bool R128InitDDARegisters(R128SavePtr save,
				 R128PLLPtr pll, ModeInfo *info)
{
    int         DisplayFifoWidth = 128;
    int         DisplayFifoDepth = 32;
    int         XclkFreq;
    int         VclkFreq;
    int         XclksPerTransfer;
    int         XclksPerTransferPrecise;
    int         UseablePrecision;
    int         Roff;
    int         Ron;

    XclkFreq = pll->xclk;

    VclkFreq = R128Div(pll->reference_freq * save->feedback_div,
		       pll->reference_div * save->post_div);

    XclksPerTransfer = R128Div(XclkFreq * DisplayFifoWidth,
			       VclkFreq * (info->bytesPerPixel * 8));

    UseablePrecision = R128MinBits(XclksPerTransfer) + 1;

    XclksPerTransferPrecise = R128Div((XclkFreq * DisplayFifoWidth)
				      << (11 - UseablePrecision),
				      VclkFreq * (info->bytesPerPixel * 8));

    Roff  = XclksPerTransferPrecise * (DisplayFifoDepth - 4);

    Ron   = (4 * ram[r128_ramtype].MB
	     + 3 * (((ram[r128_ramtype].Trcd - 2)>0)?(ram[r128_ramtype].Trcd - 2):0)
	     + 2 * ram[r128_ramtype].Trp
	     + ram[r128_ramtype].Twr
	     + ram[r128_ramtype].CL
	     + ram[r128_ramtype].Tr2w
	     + XclksPerTransfer) << (11 - UseablePrecision);

    if (Ron + ram[r128_ramtype].Rloop >= Roff) {
	return 0;
    }

    save->dda_config = (XclksPerTransferPrecise
			| (UseablePrecision << 16)
			| (ram[r128_ramtype].Rloop << 20));

    save->dda_on_off = (Ron << 16) | Roff;

    return 1;
}


/* Define initial palette for requested video mode.  This doesn't do
   anything for XFree86 4.0. */
static void R128InitPalette(R128SavePtr save)
{
    int i;
    save->palette_valid = 1;
    for(i=0;i<256;i++) save->palette[i]=i | (i<<8) | (i<<16);
}

/* Define registers for a requested video mode. */
static Bool R128Init(ModeTiming *mode, ModeInfo *info, R128SavePtr save)
{
    double      dot_clock = mode->pixelClock/1000.0;

    R128InitCommonRegisters(save);
    if (!R128InitCrtcRegisters(save, mode, info)) return 0;
#if 0
    if (HasPanelRegs)
	R128InitFPRegisters(&info->SavedReg, save, mode, info);
#endif
    switch(chiptype) {
        case Rage128:
            R128InitPLLRegisters(save, &pll, dot_clock);
            break;
        case Radeon:
            RADEONInitPLLRegisters(save, &pll, dot_clock);
            break;
    }
    
    if (!R128InitDDARegisters(save, &pll, info))
	return 0;
//    if (!info->PaletteSavedOnVT) 
    R128InitPalette(save);

    return 1;
}

static int r128_init(int, int, int);
static void r128_unlock(void);
static void r128_lock(void);

static int r128_memory;
static int r128_is_linear, r128_linear_base, r128_mmio_base;

static CardSpecs *cardspecs;

static void r128_setpage(int page)
{
    page*=2;
    OUTREG(R128_MEM_VGA_WP_SEL, page | ((page+1)<<16));
    OUTREG(R128_MEM_VGA_RP_SEL, page | ((page+1)<<16));

}

static int __svgalib_r128_inlinearmode(void)
{
return r128_is_linear;
}

/* Fill in chipset specific mode information */

static void r128_getmodeinfo(int mode, vga_modeinfo *modeinfo)
{

    if(modeinfo->colors==16)return;

    modeinfo->maxpixels = r128_memory*1024/modeinfo->bytesperpixel;
    modeinfo->maxlogicalwidth = 4088;
    modeinfo->startaddressrange = r128_memory * 1024 - 1;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;
    modeinfo->flags |= HAVE_EXT_SET;

    if (modeinfo->bytesperpixel >= 1) {
	if(r128_linear_base)modeinfo->flags |= CAPABLE_LINEAR;
        if (__svgalib_r128_inlinearmode())
	    modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
    }
}

/* Read and save chipset-specific registers */

static int r128_saveregs(unsigned char regs[])
{ 
    r128_unlock();		
    R128SaveMode((R128SavePtr)(regs+VGA_TOTAL_REGS));
    return R128_TOTAL_REGS - VGA_TOTAL_REGS;
}

/* Set chipset-specific registers */

static void r128_setregs(const unsigned char regs[], int mode)
{  
    r128_unlock();		
    
    R128RestoreMode((R128SavePtr)(regs+VGA_TOTAL_REGS));

    R128Unblank();
}
/* Return nonzero if mode is available */

static int r128_modeavailable(int mode)
{
    struct vgainfo *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];
    if (r128_memory * 1024 < info->ydim * info->xbytes)
	return 0;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 0;
    }
    free(modetiming);
    free(modeinfo);

    return SVGADRV;
}

/* Local, called by r128_setmode(). */

static void r128_initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{ /* long k; */
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

    R128Init(modetiming, modeinfo, (R128SavePtr)(moderegs+VGA_TOTAL_REGS));

    return ;
}


static int r128_setmode(int mode, int prv_mode)
{
    unsigned char *moderegs;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {
	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    }
    if (!r128_modeavailable(mode))
	return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 1;
    }

    moderegs = malloc(R128_TOTAL_REGS);

    r128_initializemode(moderegs, modetiming, modeinfo, mode);
    free(modetiming);

    __svgalib_setregs(moderegs);	/* Set standard regs. */
    r128_setregs(moderegs, mode);		/* Set extended regs. */
    free(moderegs);

    free(modeinfo);
    return 0;
}


/* Unlock chipset-specific registers */
static void r128_unlock(void)
{
    __svgalib_outcrtc(0x11, __svgalib_incrtc(0x11) & 0x7f);    
}

static void r128_lock(void)
{
}

/* Indentify chipset, initialize and return non-zero if detected */
static int r128_test(void)
{
    if (r128_init(0,0,0) != 0)
        return 0;
    return 1;
}


/* Set display start address (not for 16 color modes) */

static void r128_setdisplaystart(int address)
{ 
//  int naddress=address >> 2;
//  __svgalib_outcrtc(0x0c,(naddress>>8)&0xff);
//  __svgalib_outcrtc(0x0d,(naddress)&0xff);
  OUTREG(R128_CRTC_OFFSET, address);
}

/* Set logical scanline length (usually multiple of 8) */

static void r128_setlogicalwidth(int width)
{   
    int offset = width >> 3;
 
    if(CI.bytesperpixel>1) {
       offset = offset/CI.bytesperpixel;
    }
    __svgalib_outcrtc(0x13,offset&0xff);
    OUTREG(R128_CRTC_PITCH, offset);
}

static int r128_linear(int op, int param)
{
    if (op==LINEAR_ENABLE){r128_is_linear=1; return 0;};
    if (op==LINEAR_DISABLE){r128_is_linear=0; return 0;};
    if (op==LINEAR_QUERY_BASE) return r128_linear_base;
    if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY) return 0;		/* No granularity or range. */
        else return -1;		/* Unknown function. */
}

static int r128_match_programmable_clock(int clock)
{
return clock ;
}

static int r128_map_clock(int bpp, int clock)
{
return clock ;
}

static int r128_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
return htiming;
}

static unsigned int cur_colors[16*2];

static int r128_cursor( int cmd, int p1, int p2, int p3, int p4, void *p5) {
    int i, j;
    unsigned long *b3;
    unsigned long l1, l2;
    
    switch(cmd){
        case CURSOR_INIT:
            return 1;
        case CURSOR_HIDE:
            OUTREG(R128_CRTC_GEN_CNTL,INREG(R128_CRTC_GEN_CNTL) & ~R128_CRTC_CUR_EN );
            break;
        case CURSOR_SHOW:
            OUTREG(R128_CRTC_GEN_CNTL,INREG(R128_CRTC_GEN_CNTL) | R128_CRTC_CUR_EN );
            break;
        case CURSOR_POSITION:
            OUTREG(R128_CUR_HORZ_VERT_OFF, R128_CUR_LOCK | 0);
            OUTREG(R128_CUR_HORZ_VERT_POSN, p2|(p1<<16));
            break;
        case CURSOR_SELECT:
            i=r128_memory*1024-(p1+1)*4096;
            OUTREG(R128_CUR_OFFSET,i);
            OUTREG(R128_CUR_CLR0,cur_colors[p1*2]);
            OUTREG(R128_CUR_CLR1,cur_colors[p1*2+1]);
            break;
        case CURSOR_IMAGE:
            i=r128_memory*1024-(p1+1)*4096;
            b3=(unsigned long *)p5;
            switch(p2) {
                case 0:
                    cur_colors[p1*2]=p3;
                    cur_colors[p1*2+1]=p4;

                    for(j=0;j<32;j++) {
                        l1=*(b3+j); /* source */
                        l2=*(b3+32+j); /* mask */
                        l1=BE32(l1);
                        l2=BE32(l2);
                        *(unsigned long *)(LINEAR_POINTER+i+16*j)=~l2;
                        *(unsigned long *)(LINEAR_POINTER+i+16*j+4)=0xffffffff;
                        *(unsigned long *)(LINEAR_POINTER+i+16*j+8)=l1&l2;
                        *(unsigned long *)(LINEAR_POINTER+i+16*j+12)=0;
                        *(unsigned long *)(LINEAR_POINTER+i+512+16*j)=0xffffffff;
                        *(unsigned long *)(LINEAR_POINTER+i+512+16*j+4)=0xffffffff;
                        *(unsigned long *)(LINEAR_POINTER+i+512+16*j+8)=0;
                        *(unsigned long *)(LINEAR_POINTER+i+512+16*j+12)=0;
                    }
                break;
            }
            break;
    }
    return 0;
}       

static Emulation r128_emul = 
{
	0,
	0,
	0,
	0,
	0,
        0,
	0,				/* screenoff */
	0,				/* screenon */
	R128WaitForVerticalSync,	/* waitretrace */
};

static int r128_ext_set( unsigned what, va_list params )
{
    int param2;
    param2 = va_arg(params, int);

    switch (what) {
        case VGA_EXT_AVAILABLE:
	    switch (param2) {
                case VGA_AVAIL_SET:
	            return VGA_EXT_AVAILABLE | VGA_EXT_SET | VGA_EXT_CLEAR | VGA_EXT_RESET;
	        case VGA_AVAIL_ACCEL:
	            return 0;
	        case VGA_AVAIL_FLAGS:
	            return VGA_CLUT8;
	    }
	    break;
        case VGA_EXT_SET:
	    if (param2 & VGA_CLUT8) OUTREGP( R128_DAC_CNTL, R128_DAC_8BIT_EN, R128_DAC_MASK_ALL );
            return 1;
            break;
    }
    return 0;
}

/* Function table (exported) */

DriverSpecs __svgalib_r128_driverspecs =
{
    r128_saveregs,
    r128_setregs,
    r128_unlock,
    r128_lock,
    r128_test,
    r128_init,
    r128_setpage,
    NULL,
    NULL,
    r128_setmode,
    r128_modeavailable,
    r128_setdisplaystart,
    r128_setlogicalwidth,
    r128_getmodeinfo,
    0,				/* old blit funcs */
    0,
    0,
    0,
    0,
    r128_ext_set,		/* ext_set */
    0,				/* accel */
    r128_linear,
    0,				/* accelspecs, filled in during init. */
    &r128_emul,			/* Emulation */
    r128_cursor,
};

#define VENDOR_ID 0x1002

/* Initialize chipset (called after detection) */
static int r128_init(int force, int par1, int par2)
{
    unsigned long buf[64];
    int found=0, id, NOBIOS=0;
    unsigned char *BIOS_POINTER;
    char chipnames[2][9]={"Rage 128", "Radeon"};

    r128_memory=0;
	chiptype=-1;
    if (force) {
		r128_memory = par1;
        chiptype = par2;
    } else {

    };

    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);

    if(found) return -1;
    
    id=(buf[0]>>16)&0xffff;
    
    if( (id==0x4c45) ||
        (id==0x4c46) ||
        (id==0x4d46) ||
        (id==0x4d4c) ||
        ((id>>8)==0x50) ||
        ((id>>8)==0x52) ||
        ((id>>8)==0x53) ||
        ((id>>4)==0x544) ||
        ((id>>4)==0x545) ||
		0) chiptype=Rage128;
        
    if( 
		(id==0x4242) ||
		(id==0x4336) ||
		(id==0x4337) ||
		(id==0x4437) ||
        (id==0x7c37) ||
		((id>=0x4c57)&&(id<0x4d00)) ||
        ((id>>4)==0x546) ||
        ((id>>8)==0x31) ||
        ((id>>8)==0x3E) ||
        ((id>>8)==0x41) ||
        ((id>>8)==0x49) ||
        ((id>>8)==0x4a) ||
        ((id>>8)==0x4e) ||
        ((id>>8)==0x51) ||
        ((id>>8)==0x55) ||
        ((id>>8)==0x58) ||
        ((id>>8)==0x59) ||
        ((id>>8)==0x5b) ||
        ((id>>8)==0x5c) ||
        ((id>>8)==0x5d)
		) chiptype = Radeon;

	if( (id == 0x4158) ||
		(id == 0x5354)
		) return -1; /* Mach64/Mach32 */
	
    if(chiptype==-1) return -1;
    
    r128_linear_base=buf[4]&0xffffff00;
    r128_mmio_base=buf[6]&0xffffff00;

    __svgalib_mmio_base=r128_mmio_base;
    __svgalib_mmio_size=16*1024;

    map_mmio();
    if (MMIO_POINTER == MAP_FAILED)
        return -1;

    if(!r128_memory) r128_memory = INREG(R128_CONFIG_MEMSIZE) / 1024;

	BusCntl            = INREG(R128_BUS_CNTL);
    HasPanelRegs	= 0;
    CRTOnly		= 1;
    r128_ramtype	= 1;

#ifndef __PPC    
    if(__svgalib_secondary) 
#endif
        NOBIOS=1;

    if(NOBIOS) {
        int x_mpll_ref_fb_div, xclk_cntl, Nx, M;
        int PostDivSet[] = {0, 1, 2, 4, 8, 3, 6, 12};
    
        switch(chiptype) {
            case Rage128:
                pll.reference_freq = 2950;
                pll.min_pll_freq   = 12500;
                pll.max_pll_freq   = 25000;
                pll.reference_div =	INPLL(R128_PPLL_REF_DIV) & R128_PPLL_REF_DIV_MASK;
    
                x_mpll_ref_fb_div = INPLL(R128_X_MPLL_REF_FB_DIV);
                xclk_cntl = INPLL(R128_XCLK_CNTL) & 0x7;
                Nx = (x_mpll_ref_fb_div & 0x00FF00) >> 8;
                M =  (x_mpll_ref_fb_div & 0x0000FF);
                pll.xclk =  R128Div((2 * Nx * pll.reference_freq),
                                      (M * PostDivSet[xclk_cntl]));
                break;
            case Radeon:
                pll.reference_freq = 2700;
                pll.min_pll_freq   = 12500;
                pll.max_pll_freq   = 35000;
                pll.reference_div  = 67;
                pll.xclk 	   = 16667;
                break;
        }
    } else 
#define R128_BIOS16(x) (*(unsigned short *)(BIOS_POINTER + x))
#define R128_BIOS32(x) (*(unsigned int *)(BIOS_POINTER + x))
    {
        uint16_t bios_header;
        uint16_t pll_info_block;
        BIOS_POINTER = mmap(0, 64*1024, PROT_READ | PROT_WRITE, MAP_SHARED, __svgalib_mem_fd,
       			    0xc0000);
        if (BIOS_POINTER == MAP_FAILED)
            return -1;
        bios_header    = R128_BIOS16(0x48);
        pll_info_block = R128_BIOS16(bios_header + 0x30);
        pll.reference_freq = R128_BIOS16(pll_info_block + 0x0e);
        pll.reference_div  = R128_BIOS16(pll_info_block + 0x10);
        pll.min_pll_freq   = R128_BIOS32(pll_info_block + 0x12);
        pll.max_pll_freq   = R128_BIOS32(pll_info_block + 0x16);
        pll.xclk           = R128_BIOS16(pll_info_block + 0x08);
        munmap(BIOS_POINTER, 64*1024);
    }
#if 0
fprintf(stderr,"pll: %i %i %i %i %i\n",pll.reference_freq,pll.reference_div,
    pll.min_pll_freq,    pll.max_pll_freq, pll.xclk);
#endif

    r128_mapio();
	
    if (__svgalib_driver_report) {
	fprintf(stderr,"Using ATI %s driver, %iKB.\n",chipnames[chiptype],r128_memory);
    };
    
	__svgalib_modeinfo_linearset |= IS_LINEAR;
	
    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = r128_memory;
    cardspecs->maxPixelClock4bpp = 75000;	
    cardspecs->maxPixelClock8bpp = 250000;	
    cardspecs->maxPixelClock16bpp = 250000;	
    cardspecs->maxPixelClock24bpp = 250000;
    cardspecs->maxPixelClock32bpp = 250000;
    cardspecs->flags = INTERLACE_DIVIDE_VERT | CLOCK_PROGRAMMABLE;
    cardspecs->maxHorizontalCrtc = 4080;
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->nClocks =0;
    cardspecs->mapClock = r128_map_clock;
    cardspecs->mapHorizontalCrtc = r128_map_horizontal_crtc;
    cardspecs->matchProgrammableClock=r128_match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_r128_driverspecs;
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=r128_linear_base;
    __svgalib_linear_mem_size=r128_memory*0x400;
    if(chiptype==Radeon) {
        __svgalib_banked_mem_base=r128_linear_base;
        __svgalib_r128_driverspecs.__svgalib_setpage = __svgalib_emul_setpage;
    }

    return 0;
}

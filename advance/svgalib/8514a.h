/* 
 * 8514a.h: common header for 8514/A-like (S3, Mach) graphic engines
 * 
 * Extracted from:
 * 
 * ATI Mach32 driver Copyright 1995 Michael Weller
 * eowmob@exp-math.uni-essen.de mat42b@aixrs1.hrz.uni-essen.de
 * eowmob@pollux.exp-math.uni-essen.de
 */

#ifndef _8514A_H
#define _8514A_H

#define CMD		0x9AE8
#define ALU_FG_FN	0xBAEE
#define ALU_BG_FN	0xB6EE
#define EXT_SCISSOR_B	0xE6EE
#define EXT_SCISSOR_L	0xD2EE
#define EXT_SCISSOR_R	0xE2EE
#define EXT_SCISSOR_T	0xDEEE
#define DP_CONFIG	0xCEEE
#define FRGD_MIX	0xBAE8
#define BKGD_MIX	0xB6E8
#define FRGD_COLOR	0xA6E8
#define BKGD_COLOR	0xA2E8
#define CUR_X		0x86E8
#define CUR_Y		0x82E8
#define MAJ_AXIS_PCNT	0x96E8
#define MULTI_FUNC_CNTL	0xBEE8
#define EXT_FIFO_STATUS	0x9AEE
#define ADVFUNC_CNTL	0x4AE8	/* S3 */
#define SUBSYS_CNTL	0x42E8
#define SUBSYS_STAT	0x42E8
#define SCRATCH_PAD_0	0x52EE
#define DESTX_DIASTP    0x8EE8
#define DESTY_AXSTP	0x8AE8
#define R_SRC_X		0xDAEE
#define SRC_X		0x8EE8
#define SRC_Y		0x8AE8
#define SRC_X_START	0xB2EE
#define SRC_X_END	0xBEEE
#define SRC_Y_DIR	0xC2EE
#define	SCAN_TO_X	0xCAEE
#define DEST_X_START	0xA6EE
#define DEST_X_END	0xAAEE
#define DEST_Y_END	0xAEEE
#define GE_STAT		0x9AE8
#define CONF_STAT1	0x12EE
#define CONF_STAT2	0x16EE
#define MISC_OPTIONS	0x36EE
#define MEM_CFG		0x5EEE
#define MEM_BNDRY	0x42EE
#define LOCAL_CNTL	0x32EE
#define CHIP_ID		0xFAEE
#define EXT_GE_CONF	0x7AEE
#define R_EXT_GE_CONF	0x8EEE
#define DISP_CNTL	0x22E8
#define CLOCK_SEL	0x4AEE
#define GE_PITCH	0x76EE
#define GE_OFFSET_HI	0x72EE
#define GE_OFFSET_LO	0x6EEE
#define CRT_PITCH	0x26EE
#define CRT_OFFSET_HI	0x2EEE
#define CRT_OFFSET_LO	0x2AEE
#define H_DISP		0x06E8
#define H_TOTAL		0x02E8
#define H_SYNC_WID	0x0EE8
#define H_SYNC_STRT	0x0AE8
#define V_DISP		0x16E8
#define V_SYNC_STRT	0x1AE8
#define V_SYNC_WID	0x1EE8
#define V_TOTAL		0x12E8
#define	R_H_TOTAL       0xB2EE
#define	R_H_SYNC_STRT	0xB6EE
#define	R_H_SYNC_WID	0xBAEE
#define	R_V_TOTAL	0xC2EE
#define	R_V_DISP	0xC6EE
#define	R_V_SYNC_STRT	0xCAEE
#define	R_V_SYNC_WID	0xD2EE
#define	SHADOW_SET	0x5AEE
#define SHADOW_CTL	0x46EE
#define MISC_CTL	0x7EEE
#define R_MISC_CTL	0x92EE
#define LINEDRAW	0xFEEE
#define LINEDRAW_INDEX	0x9AEE
#define LINEDRAW_OPT	0xA2EE
#define PIX_TRANS	0xE2E8
#define DEST_CMP_FN	0xEEEE
#define CMP_COLOR	0xB2E8
#define RD_MASK		0xAEE8

#endif				/* _8514A_H */

 /***************************************************************************\
|*                                                                           *|
|*        Copyright (c) 1996-1998 NVIDIA, Corp.  All rights reserved.        *|
|*                                                                           *|
|*     NOTICE TO USER:   The source code  is copyrighted under  U.S. and     *|
|*     international laws.   NVIDIA, Corp. of Sunnyvale, California owns     *|
|*     the copyright  and as design patents  pending  on the design  and     *|
|*     interface  of the NV chips.   Users and possessors of this source     *|
|*     code are hereby granted  a nonexclusive,  royalty-free  copyright     *|
|*     and  design  patent license  to use this code  in individual  and     *|
|*     commercial software.                                                  *|
|*                                                                           *|
|*     Any use of this source code must include,  in the user documenta-     *|
|*     tion and  internal comments to the code,  notices to the end user     *|
|*     as follows:                                                           *|
|*                                                                           *|
|*     Copyright (c) 1996-1998  NVIDIA, Corp.    NVIDIA  design  patents     *|
|*     pending in the U.S. and foreign countries.                            *|
|*                                                                           *|
|*     NVIDIA, CORP.  MAKES  NO REPRESENTATION ABOUT  THE SUITABILITY OF     *|
|*     THIS SOURCE CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT     *|
|*     EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  NVIDIA, CORP. DISCLAIMS     *|
|*     ALL WARRANTIES  WITH REGARD  TO THIS SOURCE CODE,  INCLUDING  ALL     *|
|*     IMPLIED   WARRANTIES  OF  MERCHANTABILITY  AND   FITNESS   FOR  A     *|
|*     PARTICULAR  PURPOSE.   IN NO EVENT SHALL NVIDIA, CORP.  BE LIABLE     *|
|*     FOR ANY SPECIAL, INDIRECT, INCIDENTAL,  OR CONSEQUENTIAL DAMAGES,     *|
|*     OR ANY DAMAGES  WHATSOEVER  RESULTING  FROM LOSS OF USE,  DATA OR     *|
|*     PROFITS,  WHETHER IN AN ACTION  OF CONTRACT,  NEGLIGENCE OR OTHER     *|
|*     TORTIOUS ACTION, ARISING OUT  OF OR IN CONNECTION WITH THE USE OR     *|
|*     PERFORMANCE OF THIS SOURCE CODE.                                      *|
|*                                                                           *|
 \***************************************************************************/

/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/nv/nv3ref.h,v 1.1.2.3 1998/01/24 00:04:39 robin Exp $ */

#ifndef __NV3REF_H__
#define __NV3REF_H__


/* Magic values to lock/unlock extended regs */
#define UNLOCK_EXT_MAGIC 0x57
#define LOCK_EXT_MAGIC 0x99 /* Any value other than 0x57 will do */

#define LOCK_EXT_INDEX 0x6

/* Extended offset and start address */
#define NV_PCRTC_REPAINT0                                    0x19
#define NV_PCRTC_REPAINT0_OFFSET_10_8                        7:5 
#define NV_PCRTC_REPAINT0_START_ADDR_20_16                   4:0

/* Horizonal extended bits */
#define NV_PCRTC_HORIZ_EXTRA                                 0x2d
#define NV_PCRTC_HORIZ_EXTRA_INTER_HALF_START_8              4:4
#define NV_PCRTC_HORIZ_EXTRA_HORIZ_RETRACE_START_8           3:3
#define NV_PCRTC_HORIZ_EXTRA_HORIZ_BLANK_START_8             2:2
#define NV_PCRTC_HORIZ_EXTRA_DISPLAY_END_8                   1:1
#define NV_PCRTC_HORIZ_EXTRA_DISPLAY_TOTAL_8                 0:0

/* Assorted extra bits */
#define NV_PCRTC_EXTRA                                       0x25
#define NV_PCRTC_EXTRA_OFFSET_11                             5:5
#define NV_PCRTC_EXTRA_HORIZ_BLANK_END_6                     4:4
#define NV_PCRTC_EXTRA_VERT_BLANK_START_10                   3:3
#define NV_PCRTC_EXTRA_VERT_RETRACE_START_10                 2:2
#define NV_PCRTC_EXTRA_VERT_DISPLAY_END_10                   1:1
#define NV_PCRTC_EXTRA_VERT_TOTAL_10                         0:0


/* Controls how much data the refresh fifo requests */
#define NV_PCRTC_FIFO_CONTROL                                0x1b
#define NV_PCRTC_FIFO_CONTROL_UNDERFLOW_WARN                 7:7
#define NV_PCRTC_FIFO_CONTROL_BURST_LENGTH                   2:0
#define NV_PCRTC_FIFO_CONTROL_BURST_LENGTH_8                 0x0
#define NV_PCRTC_FIFO_CONTROL_BURST_LENGTH_32                0x1
#define NV_PCRTC_FIFO_CONTROL_BURST_LENGTH_64                0x2
#define NV_PCRTC_FIFO_CONTROL_BURST_LENGTH_128               0x3
#define NV_PCRTC_FIFO_CONTROL_BURST_LENGTH_256               0x4

/* When the fifo occupancy falls below *twice* the watermark,
 * the refresh fifo will start to be refilled. If this value is 
 * too low, you will get junk on the screen. Too high, and performance
 * will suffer. Watermark in units of 8 bytes
 */
#define NV_PCRTC_FIFO                                        0x20
#define NV_PCRTC_FIFO_RESET                                  7:7
#define NV_PCRTC_FIFO_WATERMARK                              5:0


/* Various flags */
#define NV_PCRTC_REPAINT1                                    0x1a
#define NV_PCRTC_REPAINT1_HSYNC                              7:7
#define NV_PCRTC_REPAINT1_HYSNC_DISABLE                      0x01
#define NV_PCRTC_REPAINT1_HYSNC_ENABLE                       0x00
#define NV_PCRTC_REPAINT1_VSYNC                              6:6
#define NV_PCRTC_REPAINT1_VYSNC_DISABLE                      0x01
#define NV_PCRTC_REPAINT1_VYSNC_ENABLE                       0x00
#define NV_PCRTC_REPAINT1_COMPATIBLE_TEXT                    4:4
#define NV_PCRTC_REPAINT1_COMPATIBLE_TEXT_ENABLE             0x01
#define NV_PCRTC_REPAINT1_COMPATIBLE_TEXT_DISABLE            0x00
#define NV_PCRTC_REPAINT1_LARGE_SCREEN                       2:2 
#define NV_PCRTC_REPAINT1_LARGE_SCREEN_DISABLE               0x01
#define NV_PCRTC_REPAINT1_LARGE_SCREEN_ENABLE                0x00 /* >=1280 */
#define NV_PCRTC_REPAINT1_PALETTE_WIDTH                      1:1
#define NV_PCRTC_REPAINT1_PALETTE_WIDTH_8BITS                0x00
#define NV_PCRTC_REPAINT1_PALETTE_WIDTH_6BITS                0x01


#define NV_PCRTC_GRCURSOR0                                   0x30
#define NV_PCRTC_GRCURSOR0_START_ADDR_21_16                  5:0

#define NV_PCRTC_GRCURSOR1                                   0x31
#define NV_PCRTC_GRCURSOR1_START_ADDR_15_11                  7:3
#define NV_PCRTC_GRCURSOR1_SCAN_DBL                          1:1
#define NV_PCRTC_GRCURSOR1_SCAN_DBL_DISABLE                  0
#define NV_PCRTC_GRCURSOR1_SCAN_DBL_ENABLE                   1
#define NV_PCRTC_GRCURSOR1_CURSOR                            0:0
#define NV_PCRTC_GRCURSOR1_CURSOR_DISABLE                    0 
#define NV_PCRTC_GRCURSOR1_CURSOR_ENABLE                     1

#define NV_PCRTC_SCREEN                                      0x41


/* Controls what the format of the framebuffer is */
#define NV_PCRTC_PIXEL                       0x28
#define NV_PCRTC_PIXEL_MODE                  7:7
#define NV_PCRTC_PIXEL_MODE_TV               0x01
#define NV_PCRTC_PIXEL_MODE_VGA              0x00
#define NV_PCRTC_PIXEL_TV_MODE               6:6
#define NV_PCRTC_PIXEL_TV_MODE_NTSC          0x00
#define NV_PCRTC_PIXEL_TV_MODE_PAL           0x01
#define NV_PCRTC_PIXEL_TV_HORIZ_ADJUST       5:3
#define NV_PCRTC_PIXEL_FORMAT                1:0
#define NV_PCRTC_PIXEL_FORMAT_VGA            0x00
#define NV_PCRTC_PIXEL_FORMAT_8BPP           0x01
#define NV_PCRTC_PIXEL_FORMAT_16BPP          0x02
#define NV_PCRTC_PIXEL_FORMAT_32BPP          0x03

#define NV_PEXTDEV			      0x00101fff:0x00101000
#define NV_PEXTDEV_0			                 0x00101000

#define NV_PRAMDAC                            0x00680FFF:0x00680000 /* RW--D */

#define NV_PRAMDAC_VPLL_COEFF                            0x00680508 /* RW-4R */
#define NV_PRAMDAC_VPLL_COEFF_MDIV                              7:0 /* RWIUF */
#define NV_PRAMDAC_VPLL_COEFF_NDIV                             15:8 /* RWIUF */
#define NV_PRAMDAC_VPLL_COEFF_PDIV                            18:16 /* RWIVF */
#define NV_PRAMDAC_VPLLB_COEFF                           0x00680578 /* RW-4R */


#define NV_PRAMDAC_PLL_COEFF_SELECT                      0x0068050C /* RW-4R */
#define NV_PRAMDAC_PLL_COEFF_SELECT_DLL_BYPASS                  4:4 /* RWIVF */
#define NV_PRAMDAC_PLL_COEFF_SELECT_DLL_BYPASS_FALSE     0x00000000 /* RWI-V */
#define NV_PRAMDAC_PLL_COEFF_SELECT_DLL_BYPASS_TRUE      0x00000001 /* RW--V */
#define NV_PRAMDAC_PLL_COEFF_SELECT_MPLL_SOURCE                 8:8 /* RWIVF */
#define NV_PRAMDAC_PLL_COEFF_SELECT_MPLL_SOURCE_DEFAULT  0x00000000 /* RWI-V */
#define NV_PRAMDAC_PLL_COEFF_SELECT_MPLL_SOURCE_PROG     0x00000001 /* RW--V */
#define NV_PRAMDAC_PLL_COEFF_SELECT_MPLL_BYPASS               12:12 /* RWIVF */
#define NV_PRAMDAC_PLL_COEFF_SELECT_MPLL_BYPASS_FALSE    0x00000000 /* RWI-V */
#define NV_PRAMDAC_PLL_COEFF_SELECT_MPLL_BYPASS_TRUE     0x00000001 /* RW--V */
#define NV_PRAMDAC_PLL_COEFF_SELECT_VPLL_SOURCE               16:16 /* RWIVF */
#define NV_PRAMDAC_PLL_COEFF_SELECT_VPLL_SOURCE_DEFAULT  0x00000000 /* RWI-V */
#define NV_PRAMDAC_PLL_COEFF_SELECT_VPLL_SOURCE_PROG     0x00000001 /* RW--V */
#define NV_PRAMDAC_PLL_COEFF_SELECT_VPLL_BYPASS               20:20 /* RWIVF */
#define NV_PRAMDAC_PLL_COEFF_SELECT_VPLL_BYPASS_FALSE    0x00000000 /* RWI-V */
#define NV_PRAMDAC_PLL_COEFF_SELECT_VPLL_BYPASS_TRUE     0x00000001 /* RW--V */
#define NV_PRAMDAC_PLL_COEFF_SELECT_PCLK_SOURCE               25:24 /* RWIVF */
#define NV_PRAMDAC_PLL_COEFF_SELECT_PCLK_SOURCE_VPLL     0x00000000 /* RWI-V */
#define NV_PRAMDAC_PLL_COEFF_SELECT_PCLK_SOURCE_VIP      0x00000001 /* RW--V */
#define NV_PRAMDAC_PLL_COEFF_SELECT_PCLK_SOURCE_XTALOSC  0x00000002 /* RW--V */
#define NV_PRAMDAC_PLL_COEFF_SELECT_VCLK_RATIO                28:28 /* RWIVF */
#define NV_PRAMDAC_PLL_COEFF_SELECT_VCLK_RATIO_DB1       0x00000000 /* RWI-V */
#define NV_PRAMDAC_PLL_COEFF_SELECT_VCLK_RATIO_DB2       0x00000001 /* RW--V */


/* Various flags for DAC. BPC controls the width of the palette */

#define NV_PRAMDAC_GENERAL_CONTROL                       0x00680600 /* RW-4R */
#define NV_PRAMDAC_GENERAL_CONTROL_FF_COEFF                     1:0 /* RWIVF */
#define NV_PRAMDAC_GENERAL_CONTROL_FF_COEFF_DEF          0x00000000 /* RWI-V */
#define NV_PRAMDAC_GENERAL_CONTROL_IDC_MODE                     4:4 /* RWIVF */
#define NV_PRAMDAC_GENERAL_CONTROL_IDC_MODE_GAMMA        0x00000000 /* RWI-V */
#define NV_PRAMDAC_GENERAL_CONTROL_IDC_MODE_INDEX        0x00000001 /* RW--V */
#define NV_PRAMDAC_GENERAL_CONTROL_VGA_STATE                    8:8 /* RWIVF */
#define NV_PRAMDAC_GENERAL_CONTROL_VGA_STATE_NOTSE       0x00000000 /* RWI-V */
#define NV_PRAMDAC_GENERAL_CONTROL_VGA_STATE_SEL         0x00000001 /* RW--V */
#define NV_PRAMDAC_GENERAL_CONTROL_565_MODE                   12:12 /* RWIVF */
#define NV_PRAMDAC_GENERAL_CONTROL_565_MODE_NOTSEL       0x00000000 /* RWI-V */
#define NV_PRAMDAC_GENERAL_CONTROL_565_MODE_SEL          0x00000001 /* RW--V */
#define NV_PRAMDAC_GENERAL_CONTROL_BLK_PEDSTL                 16:16 /* RWIVF */
#define NV_PRAMDAC_GENERAL_CONTROL_BLK_PEDSTL_OFF        0x00000000 /* RWI-V */
#define NV_PRAMDAC_GENERAL_CONTROL_BLK_PEDSTL_ON         0x00000001 /* RW--V */
#define NV_PRAMDAC_GENERAL_CONTROL_TERMINATION                17:17 /* RWIVF */
#define NV_PRAMDAC_GENERAL_CONTROL_TERMINATION_37OHM     0x00000000 /* RWI-V */
#define NV_PRAMDAC_GENERAL_CONTROL_TERMINATION_75OHM     0x00000001 /* RW--V */
#define NV_PRAMDAC_GENERAL_CONTROL_BPC                        20:20 /* RWIVF */
#define NV_PRAMDAC_GENERAL_CONTROL_BPC_6BITS             0x00000000 /* RWI-V */
#define NV_PRAMDAC_GENERAL_CONTROL_BPC_8BITS             0x00000001 /* RW--V */
#define NV_PRAMDAC_GENERAL_CONTROL_DAC_SLEEP                  24:24 /* RWIVF */
#define NV_PRAMDAC_GENERAL_CONTROL_DAC_SLEEP_DIS         0x00000000 /* RWI-V */
#define NV_PRAMDAC_GENERAL_CONTROL_DAC_SLEEP_EN          0x00000001 /* RW--V */
#define NV_PRAMDAC_GENERAL_CONTROL_PALETTE_CLK                28:28 /* RWIVF */
#define NV_PRAMDAC_GENERAL_CONTROL_PALETTE_CLK_EN        0x00000000 /* RWI-V */
#define NV_PRAMDAC_GENERAL_CONTROL_PALETTE_CLK_DIS       0x00000001 /* RW--V */


#define NV_PRAMDAC_GRCURSOR_START_POS                    0x00680300 /* RW-4R */
#define NV_PRAMDAC_GRCURSOR_START_POS_X                        11:0 /* RWXSF */
#define NV_PRAMDAC_GRCURSOR_START_POS_Y                       27:16 /* RWXSF */

#define NV_PMC                                0x00000FFF:0x00000000 /* RW--D */
#define NV_PMC_INTR_0                                    0x00000100 /* RW-4R */
#define NV_PMC_INTR_0_PAUDIO                                    0:0 /* R--VF */
#define NV_PMC_INTR_0_PAUDIO_NOT_PENDING                 0x00000000 /* R---V */
#define NV_PMC_INTR_0_PAUDIO_PENDING                     0x00000001 /* R---V */
#define NV_PMC_INTR_0_PMEDIA                                    4:4 /* R--VF */
#define NV_PMC_INTR_0_PMEDIA_NOT_PENDING                 0x00000000 /* R---V */
#define NV_PMC_INTR_0_PMEDIA_PENDING                     0x00000001 /* R---V */
#define NV_PMC_INTR_0_PFIFO                                     8:8 /* R--VF */
#define NV_PMC_INTR_0_PFIFO_NOT_PENDING                  0x00000000 /* R---V */
#define NV_PMC_INTR_0_PFIFO_PENDING                      0x00000001 /* R---V */
#define NV_PMC_INTR_0_PGRAPH0                                 12:12 /* R--VF */
#define NV_PMC_INTR_0_PGRAPH0_NOT_PENDING                0x00000000 /* R---V */
#define NV_PMC_INTR_0_PGRAPH0_PENDING                    0x00000001 /* R---V */
#define NV_PMC_INTR_0_PGRAPH1                                 13:13 /* R--VF */
#define NV_PMC_INTR_0_PGRAPH1_NOT_PENDING                0x00000000 /* R---V */
#define NV_PMC_INTR_0_PGRAPH1_PENDING                    0x00000001 /* R---V */
#define NV_PMC_INTR_0_PVIDEO                                  16:16 /* R--VF */
#define NV_PMC_INTR_0_PVIDEO_NOT_PENDING                 0x00000000 /* R---V */
#define NV_PMC_INTR_0_PVIDEO_PENDING                     0x00000001 /* R---V */
#define NV_PMC_INTR_0_PTIMER                                  20:20 /* R--VF */
#define NV_PMC_INTR_0_PTIMER_NOT_PENDING                 0x00000000 /* R---V */
#define NV_PMC_INTR_0_PTIMER_PENDING                     0x00000001 /* R---V */
#define NV_PMC_INTR_0_PFB                                     24:24 /* R--VF */
#define NV_PMC_INTR_0_PFB_NOT_PENDING                    0x00000000 /* R---V */
#define NV_PMC_INTR_0_PFB_PENDING                        0x00000001 /* R---V */
#define NV_PMC_INTR_0_PBUS                                    28:28 /* R--VF */
#define NV_PMC_INTR_0_PBUS_NOT_PENDING                   0x00000000 /* R---V */
#define NV_PMC_INTR_0_PBUS_PENDING                       0x00000001 /* R---V */
#define NV_PMC_INTR_0_SOFTWARE                                31:31 /* RWIVF */
#define NV_PMC_INTR_0_SOFTWARE_NOT_PENDING               0x00000000 /* RWI-V */
#define NV_PMC_INTR_0_SOFTWARE_PENDING                   0x00000001 /* RW--V */

#define NV_PMC_INTR_EN_0                                 0x00000140 /* RW-4R */
#define NV_PMC_INTR_EN_0_INTA                                   1:0 /* RWIVF */
#define NV_PMC_INTR_EN_0_INTA_DISABLED                   0x00000000 /* RWI-V */
#define NV_PMC_INTR_EN_0_INTA_HARDWARE                   0x00000001 /* RW--V */
#define NV_PMC_INTR_EN_0_INTA_SOFTWARE                   0x00000002 /* RW--V */

#define NV_PMC_ENABLE                                    0x00000200 /* RW-4R */

#define NV_PFIFO                              0x00003FFF:0x00002000 /* RW--D */
#define NV_PFIFO_INTR_0                                  0x00002100 /* RW-4R */
#define NV_PFIFO_INTR_0_CACHE_ERROR                             0:0 /* RWXVF */
#define NV_PFIFO_INTR_0_CACHE_ERROR_NOT_PENDING          0x00000000 /* R---V */
#define NV_PFIFO_INTR_0_CACHE_ERROR_PENDING              0x00000001 /* R---V */
#define NV_PFIFO_INTR_0_CACHE_ERROR_RESET                0x00000001 /* -W--V */
#define NV_PFIFO_INTR_0_RUNOUT                                  4:4 /* RWXVF */
#define NV_PFIFO_INTR_0_RUNOUT_NOT_PENDING               0x00000000 /* R---V */
#define NV_PFIFO_INTR_0_RUNOUT_PENDING                   0x00000001 /* R---V */
#define NV_PFIFO_INTR_0_RUNOUT_RESET                     0x00000001 /* -W--V */
#define NV_PFIFO_INTR_0_RUNOUT_OVERFLOW                         8:8 /* RWXVF */
#define NV_PFIFO_INTR_0_RUNOUT_OVERFLOW_NOT_PENDING      0x00000000 /* R---V */
#define NV_PFIFO_INTR_0_RUNOUT_OVERFLOW_PENDING          0x00000001 /* R---V */
#define NV_PFIFO_INTR_0_RUNOUT_OVERFLOW_RESET            0x00000001 /* -W--V */
#define NV_PFIFO_INTR_0_DMA_PUSHER                            12:12 /* RWXVF */
#define NV_PFIFO_INTR_0_DMA_PUSHER_NOT_PENDING           0x00000000 /* R---V */
#define NV_PFIFO_INTR_0_DMA_PUSHER_PENDING               0x00000001 /* R---V */
#define NV_PFIFO_INTR_0_DMA_PUSHER_RESET                 0x00000001 /* -W--V */
#define NV_PFIFO_INTR_0_DMA_PTE                               16:16 /* RWXVF */
#define NV_PFIFO_INTR_0_DMA_PTE_NOT_PENDING              0x00000000 /* R---V */
#define NV_PFIFO_INTR_0_DMA_PTE_PENDING                  0x00000001 /* R---V */
#define NV_PFIFO_INTR_0_DMA_PTE_RESET                    0x00000001 /* -W--V */
#define NV_PFIFO_INTR_EN_0                               0x00002140 /* RW-4R */
#define NV_PFIFO_INTR_EN_0_CACHE_ERROR                          0:0 /* RWIVF */
#define NV_PFIFO_INTR_EN_0_CACHE_ERROR_DISABLED          0x00000000 /* RWI-V */
#define NV_PFIFO_INTR_EN_0_CACHE_ERROR_ENABLED           0x00000001 /* RW--V */
#define NV_PFIFO_INTR_EN_0_RUNOUT                               4:4 /* RWIVF */
#define NV_PFIFO_INTR_EN_0_RUNOUT_DISABLED               0x00000000 /* RWI-V */
#define NV_PFIFO_INTR_EN_0_RUNOUT_ENABLED                0x00000001 /* RW--V */
#define NV_PFIFO_INTR_EN_0_RUNOUT_OVERFLOW                      8:8 /* RWIVF */
#define NV_PFIFO_INTR_EN_0_RUNOUT_OVERFLOW_DISABLED      0x00000000 /* RWI-V */
#define NV_PFIFO_INTR_EN_0_RUNOUT_OVERFLOW_ENABLED       0x00000001 /* RW--V */
#define NV_PFIFO_CONFIG_0                                0x00002200 /* RW-4R */
#define NV_PFIFO_RAMHT                                   0x00002210 /* RW-4R */
#define NV_PFIFO_RAMHT_BASE_ADDRESS                           15:12 /* RWXVF */
#define NV_PFIFO_RAMHT_SIZE                                   17:16 /* RWXVF */
#define NV_PFIFO_RAMHT_SIZE_4K                           0x00000000 /* RWI-V */
#define NV_PFIFO_RAMHT_SIZE_8K                           0x00000001 /* RW--V */
#define NV_PFIFO_RAMHT_SIZE_16K                          0x00000002 /* RW--V */
#define NV_PFIFO_RAMHT_SIZE_32K                          0x00000003 /* RW--V */
#define NV_PFIFO_RAMFC                                   0x00002214 /* RW-4R */
#define NV_PFIFO_RAMFC_BASE_ADDRESS                            15:9 /* RWXVF */
#define NV_PFIFO_RAMRO                                   0x00002218 /* RW-4R */
#define NV_PFIFO_RAMRO_BASE_ADDRESS                            15:9 /* RWXVF */
#define NV_PFIFO_RAMRO_SIZE                                   16:16 /* RWXVF */
#define NV_PFIFO_RAMRO_SIZE_512                          0x00000000 /* RWI-V */
#define NV_PFIFO_RAMRO_SIZE_8K                           0x00000001 /* RW--V */
#define NV_PFIFO_CACHES                                  0x00002500 /* RW-4R */
#define NV_PFIFO_CACHES_REASSIGN                                0:0 /* RWIVF */
#define NV_PFIFO_CACHES_REASSIGN_DISABLED                0x00000000 /* RWI-V */
#define NV_PFIFO_CACHES_REASSIGN_ENABLED                 0x00000001 /* RW--V */
#define NV_PFIFO_CACHE0_PUSH0                            0x00003000 /* RW-4R */
#define NV_PFIFO_CACHE0_PUSH0_ACCESS                            0:0 /* RWIVF */
#define NV_PFIFO_CACHE0_PUSH0_ACCESS_DISABLED            0x00000000 /* RWI-V */
#define NV_PFIFO_CACHE0_PUSH0_ACCESS_ENABLED             0x00000001 /* RW--V */
#define NV_PFIFO_CACHE1_PUSH0                            0x00003200 /* RW-4R */
#define NV_PFIFO_CACHE1_PUSH0_ACCESS                            0:0 /* RWIVF */
#define NV_PFIFO_CACHE1_PUSH0_ACCESS_DISABLED            0x00000000 /* RWI-V */
#define NV_PFIFO_CACHE1_PUSH0_ACCESS_ENABLED             0x00000001 /* RW--V */
#define NV_PFIFO_CACHE0_PUSH1                            0x00003004 /* RW-4R */
#define NV_PFIFO_CACHE0_PUSH1_CHID                              6:0 /* RWXUF */
#define NV_PFIFO_CACHE1_PUSH1                            0x00003204 /* RW-4R */
#define NV_PFIFO_CACHE1_PUSH1_CHID                              6:0 /* RWXUF */
#define NV_PFIFO_CACHE1_DMA0                             0x00003220 /* RW-4R */
#define NV_PFIFO_CACHE1_DMA1                             0x00003224 /* RW-4R */
#define NV_PFIFO_CACHE1_DMA2                             0x00003228 /* RW-4R */
#define NV_PFIFO_CACHE0_PULL0                            0x00003040 /* RW-4R */
#define NV_PFIFO_CACHE0_PULL0_ACCESS                            0:0 /* RWIVF */
#define NV_PFIFO_CACHE0_PULL0_ACCESS_DISABLED            0x00000000 /* RWI-V */
#define NV_PFIFO_CACHE0_PULL0_ACCESS_ENABLED             0x00000001 /* RW--V */
#define NV_PFIFO_CACHE1_PULL0                            0x00003240 /* RW-4R */
#define NV_PFIFO_CACHE1_PULL0_ACCESS                            0:0 /* RWIVF */
#define NV_PFIFO_CACHE1_PULL0_ACCESS_DISABLED            0x00000000 /* RWI-V */
#define NV_PFIFO_CACHE1_PULL0_ACCESS_ENABLED             0x00000001 /* RW--V */
#define NV_PFIFO_CACHE1_PULL1                            0x00003250 /* RW-4R */
#define NV_PFIFO_CACHE1_PULL1_CTX                               4:4 /* RWXVF */
#define NV_PFIFO_CACHE1_PULL1_CTX_CLEAN                  0x00000000 /* RW--V */
#define NV_PFIFO_CACHE1_PULL1_CTX_DIRTY                  0x00000001 /* RW--V */
#define NV_PFIFO_CACHE1_PUT                              0x00003210 /* RW-4R */
#define NV_PFIFO_CACHE1_PUT_ADDRESS                             6:2 /* RWXUF */
#define NV_PFIFO_CACHE1_GET                              0x00003270 /* RW-4R */
#define NV_PFIFO_CACHE1_GET_ADDRESS                             6:2 /* RWXUF */
#define NV_PFIFO_CACHE1_CTX(i)                  (0x00003280+(i)*16) /* RW-4A */
#define NV_PFIFO_CACHE1_CTX__SIZE_1                               8 /*       */
#define NV_PFIFO_RUNOUT_PUT                              0x00002410 /* RW-4R */
#define NV_PFIFO_RUNOUT_GET                              0x00002420 /* RW-4R */
#define NV_PFIFO_RUNOUT_STATUS                           0x00002400 /* R--4R */

#define NV_PGRAPH                             0x00401FFF:0x00400000 /* RW--D */
#define NV_PGRAPH_DEBUG_0                                0x00400080 /* RW-4R */
#define NV_PGRAPH_DEBUG_0_STATE                                 0:0 /* CW-VF */
#define NV_PGRAPH_DEBUG_0_STATE_NORMAL                   0x00000000 /* CW--V */
#define NV_PGRAPH_DEBUG_0_STATE_RESET                    0x00000001 /* -W--V */
#define NV_PGRAPH_DEBUG_0_BULK_READS                            4:4 /* RWIVF */
#define NV_PGRAPH_DEBUG_0_BULK_READS_DISABLED            0x00000000 /* RWI-V */
#define NV_PGRAPH_DEBUG_0_BULK_READS_ENABLED             0x00000001 /* RW--V */
#define NV_PGRAPH_DEBUG_0_WRITE_ONLY_ROPS_2D                  20:20 /* RWIVF */
#define NV_PGRAPH_DEBUG_0_WRITE_ONLY_ROPS_2D_DISABLED    0x00000000 /* RWI-V */
#define NV_PGRAPH_DEBUG_0_WRITE_ONLY_ROPS_2D_ENABLED     0x00000001 /* RW--V */
#define NV_PGRAPH_DEBUG_0_DRAWDIR_AUTO                        24:24 /* RWIVF */
#define NV_PGRAPH_DEBUG_0_DRAWDIR_AUTO_DISABLED          0x00000000 /* RWI-V */
#define NV_PGRAPH_DEBUG_0_DRAWDIR_AUTO_ENABLED           0x00000001 /* RW--V */
#define NV_PGRAPH_DEBUG_1                                0x00400084 /* RW-4R */
#define NV_PGRAPH_DEBUG_1_VOLATILE_RESET                        0:0 /* RWIVF */
#define NV_PGRAPH_DEBUG_1_VOLATILE_RESET_NOT_LAST        0x00000000 /* RWI-V */
#define NV_PGRAPH_DEBUG_1_VOLATILE_RESET_LAST            0x00000001 /* RW--V */
#define NV_PGRAPH_DEBUG_1_INSTANCE                            16:16 /* RWIVF */
#define NV_PGRAPH_DEBUG_1_INSTANCE_DISABLED              0x00000000 /* RWI-V */
#define NV_PGRAPH_DEBUG_1_INSTANCE_ENABLED               0x00000001 /* RW--V */
#define NV_PGRAPH_DEBUG_1_CTX                                 20:20 /* RWIVF */
#define NV_PGRAPH_DEBUG_1_CTX_DISABLED                   0x00000000 /* RWI-V */
#define NV_PGRAPH_DEBUG_1_CTX_ENABLED                    0x00000001 /* RW--V */
#define NV_PGRAPH_DEBUG_2                                0x00400088 /* RW-4R */
#define NV_PGRAPH_DEBUG_2_AVOID_RMW_BLEND                       0:0 /* RWIVF */
#define NV_PGRAPH_DEBUG_2_AVOID_RMW_BLEND_DISABLED       0x00000000 /* RWI-V */
#define NV_PGRAPH_DEBUG_2_AVOID_RMW_BLEND_ENABLED        0x00000001 /* RW--V */
#define NV_PGRAPH_DEBUG_2_DPWR_FIFO                             8:8 /* RWIVF */
#define NV_PGRAPH_DEBUG_2_DPWR_FIFO_DISABLED             0x00000000 /* RWI-V */
#define NV_PGRAPH_DEBUG_2_DPWR_FIFO_ENABLED              0x00000001 /* RW--V */
#define NV_PGRAPH_DEBUG_2_VOLATILE_RESET                      28:28 /* RWIVF */
#define NV_PGRAPH_DEBUG_2_VOLATILE_RESET_DISABLED        0x00000000 /* RWI-V */
#define NV_PGRAPH_DEBUG_2_VOLATILE_RESET_ENABLED         0x00000001 /* RW--V */
#define NV_PGRAPH_DEBUG_3                                0x0040008C /* RW-4R */
#define NV_PGRAPH_DEBUG_3_HONOR_ALPHA                         24:24 /* RWIVF */
#define NV_PGRAPH_DEBUG_3_HONOR_ALPHA_DISABLED           0x00000000 /* RWI-V */
#define NV_PGRAPH_DEBUG_3_HONOR_ALPHA_ENABLED            0x00000001 /* RW--V */

#define NV_PGRAPH_INTR_0                                 0x00400100 /* RW-4R */
#define NV_PGRAPH_INTR_0_RESERVED                               0:0 /* RW-VF */
#define NV_PGRAPH_INTR_0_RESERVED_NOT_PENDING            0x00000000 /* R---V */
#define NV_PGRAPH_INTR_0_RESERVED_PENDING                0x00000001 /* R---V */
#define NV_PGRAPH_INTR_0_RESERVED_RESET                  0x00000001 /* -W--V */
#define NV_PGRAPH_INTR_0_CONTEXT_SWITCH                         4:4 /* RWIVF */
#define NV_PGRAPH_INTR_0_CONTEXT_SWITCH_NOT_PENDING      0x00000000 /* R-I-V */
#define NV_PGRAPH_INTR_0_CONTEXT_SWITCH_PENDING          0x00000001 /* R---V */
#define NV_PGRAPH_INTR_0_CONTEXT_SWITCH_RESET            0x00000001 /* -W--V */
#define NV_PGRAPH_INTR_0_VBLANK                                 8:8 /* RWIVF */
#define NV_PGRAPH_INTR_0_VBLANK_NOT_PENDING              0x00000000 /* R-I-V */
#define NV_PGRAPH_INTR_0_VBLANK_PENDING                  0x00000001 /* R---V */
#define NV_PGRAPH_INTR_0_VBLANK_RESET                    0x00000001 /* -W--V */
#define NV_PGRAPH_INTR_0_RANGE                                12:12 /* RWIVF */
#define NV_PGRAPH_INTR_0_RANGE_NOT_PENDING               0x00000000 /* R-I-V */
#define NV_PGRAPH_INTR_0_RANGE_PENDING                   0x00000001 /* R---V */
#define NV_PGRAPH_INTR_0_RANGE_RESET                     0x00000001 /* -W--V */
#define NV_PGRAPH_INTR_0_METHOD_COUNT                         16:16 /* RWIVF */
#define NV_PGRAPH_INTR_0_METHOD_COUNT_NOT_PENDING        0x00000000 /* R-I-V */
#define NV_PGRAPH_INTR_0_METHOD_COUNT_PENDING            0x00000001 /* R---V */
#define NV_PGRAPH_INTR_0_METHOD_COUNT_RESET              0x00000001 /* -W--V */
#define NV_PGRAPH_INTR_0_FORMAT                               20:20 /* RWIVF */
#define NV_PGRAPH_INTR_0_FORMAT_NOT_PENDING              0x00000000 /* R-I-V */
#define NV_PGRAPH_INTR_0_FORMAT_PENDING                  0x00000001 /* R---V */
#define NV_PGRAPH_INTR_0_FORMAT_RESET                    0x00000001 /* -W--V */
#define NV_PGRAPH_INTR_0_COMPLEX_CLIP                         24:24 /* RWIVF */
#define NV_PGRAPH_INTR_0_COMPLEX_CLIP_NOT_PENDING        0x00000000 /* R-I-V */
#define NV_PGRAPH_INTR_0_COMPLEX_CLIP_PENDING            0x00000001 /* R---V */
#define NV_PGRAPH_INTR_0_COMPLEX_CLIP_RESET              0x00000001 /* -W--V */
#define NV_PGRAPH_INTR_0_NOTIFY                               28:28 /* RWIVF */
#define NV_PGRAPH_INTR_0_NOTIFY_NOT_PENDING              0x00000000 /* R-I-V */
#define NV_PGRAPH_INTR_0_NOTIFY_PENDING                  0x00000001 /* R---V */
#define NV_PGRAPH_INTR_0_NOTIFY_RESET                    0x00000001 /* -W--V */

#define NV_PGRAPH_INTR_1                                 0x00400104 /* RW-4R */
#define NV_PGRAPH_INTR_1_METHOD                                 0:0 /* RWIVF */
#define NV_PGRAPH_INTR_1_METHOD_NOT_PENDING              0x00000000 /* R-I-V */
#define NV_PGRAPH_INTR_1_METHOD_PENDING                  0x00000001 /* R---V */
#define NV_PGRAPH_INTR_1_METHOD_RESET                    0x00000001 /* -W--V */
#define NV_PGRAPH_INTR_1_DATA                                   4:4 /* RWIVF */
#define NV_PGRAPH_INTR_1_DATA_NOT_PENDING                0x00000000 /* R-I-V */
#define NV_PGRAPH_INTR_1_DATA_PENDING                    0x00000001 /* R---V */
#define NV_PGRAPH_INTR_1_DATA_RESET                      0x00000001 /* -W--V */
#define NV_PGRAPH_INTR_1_DOUBLE_NOTIFY                        12:12 /* RWIVF */
#define NV_PGRAPH_INTR_1_DOUBLE_NOTIFY_NOT_PENDING       0x00000000 /* R-I-V */
#define NV_PGRAPH_INTR_1_DOUBLE_NOTIFY_PENDING           0x00000001 /* R---V */
#define NV_PGRAPH_INTR_1_DOUBLE_NOTIFY_RESET             0x00000001 /* -W--V */
#define NV_PGRAPH_INTR_1_CTXSW_NOTIFY                         16:16 /* RWIVF */
#define NV_PGRAPH_INTR_1_CTXSW_NOTIFY_NOT_PENDING        0x00000000 /* R-I-V */
#define NV_PGRAPH_INTR_1_CTXSW_NOTIFY_PENDING            0x00000001 /* R---V */
#define NV_PGRAPH_INTR_1_CTXSW_NOTIFY_RESET              0x00000001 /* -W--V */

#define NV_PGRAPH_INTR_EN_0                              0x00400140 /* RW-4R */

#define NV_PGRAPH_INTR_EN_1                              0x00400144 /* RW-4R */

#define NV_PGRAPH_CTX_CACHE(i)                   (0x004001a0+(i)*4) /* RW-4A */
#define NV_PGRAPH_CTX_CACHE__SIZE_1                               8 /*       */

#define NV_PGRAPH_CTX_SWITCH                             0x00400180 /* RW-4R */

#define NV_PGRAPH_CTX_CONTROL                            0x00400190 /* RW-4R */
#define NV_PGRAPH_CTX_CONTROL_MINIMUM_TIME                      1:0 /* RWIVF */
#define NV_PGRAPH_CTX_CONTROL_MINIMUM_TIME_33US          0x00000000 /* RWI-V */
#define NV_PGRAPH_CTX_CONTROL_MINIMUM_TIME_262US         0x00000001 /* RW--V */
#define NV_PGRAPH_CTX_CONTROL_MINIMUM_TIME_2MS           0x00000002 /* RW--V */
#define NV_PGRAPH_CTX_CONTROL_MINIMUM_TIME_17MS          0x00000003 /* RW--V */
#define NV_PGRAPH_CTX_CONTROL_TIME                              8:8 /* RWIVF */
#define NV_PGRAPH_CTX_CONTROL_TIME_EXPIRED               0x00000000 /* RWI-V */
#define NV_PGRAPH_CTX_CONTROL_TIME_NOT_EXPIRED           0x00000001 /* RW--V */
#define NV_PGRAPH_CTX_CONTROL_CHID                            16:16 /* RWIVF */
#define NV_PGRAPH_CTX_CONTROL_CHID_INVALID               0x00000000 /* RWI-V */
#define NV_PGRAPH_CTX_CONTROL_CHID_VALID                 0x00000001 /* RW--V */
#define NV_PGRAPH_CTX_CONTROL_SWITCH                          20:20 /* R--VF */
#define NV_PGRAPH_CTX_CONTROL_SWITCH_UNAVAILABLE         0x00000000 /* R---V */
#define NV_PGRAPH_CTX_CONTROL_SWITCH_AVAILABLE           0x00000001 /* R---V */
#define NV_PGRAPH_CTX_CONTROL_SWITCHING                       24:24 /* RWIVF */
#define NV_PGRAPH_CTX_CONTROL_SWITCHING_IDLE             0x00000000 /* RWI-V */
#define NV_PGRAPH_CTX_CONTROL_SWITCHING_BUSY             0x00000001 /* RW--V */
#define NV_PGRAPH_CTX_CONTROL_DEVICE                          28:28 /* RWIVF */
#define NV_PGRAPH_CTX_CONTROL_DEVICE_DISABLED            0x00000000 /* RWI-V */
#define NV_PGRAPH_CTX_CONTROL_DEVICE_ENABLED             0x00000001 /* RW--V */

#define NV_PGRAPH_CTX_USER                               0x00400194 /* RW-4R */

#define NV_PGRAPH_FIFO                                   0x004006A4 /* RW-4R */
#define NV_PGRAPH_FIFO_ACCESS                                   0:0 /* RWIVF */
#define NV_PGRAPH_FIFO_ACCESS_DISABLED                   0x00000000 /* RW--V */
#define NV_PGRAPH_FIFO_ACCESS_ENABLED                    0x00000001 /* RWI-V */

#define NV_PGRAPH_STATUS                                 0x004006B0 /* R--4R */

#define NV_PGRAPH_CLIP_MISC                              0x004006A0 /* RW-4R */
#define NV_PGRAPH_SRC_CANVAS_MIN                         0x00400550 /* RW-4R */
#define NV_PGRAPH_DST_CANVAS_MIN                         0x00400558 /* RW-4R */
#define NV_PGRAPH_SRC_CANVAS_MAX                         0x00400554 /* RW-4R */
#define NV_PGRAPH_DST_CANVAS_MAX                         0x0040055C /* RW-4R */
#define NV_PGRAPH_CLIP0_MIN                              0x00400690 /* RW-4R */
#define NV_PGRAPH_CLIP1_MIN                              0x00400698 /* RW-4R */
#define NV_PGRAPH_CLIP0_MAX                              0x00400694 /* RW-4R */
#define NV_PGRAPH_CLIP1_MAX                              0x0040069C /* RW-4R */
#define NV_PGRAPH_DMA                                    0x00400680 /* RW-4R */
#define NV_PGRAPH_NOTIFY                                 0x00400684 /* RW-4R */
#define NV_PGRAPH_INSTANCE                               0x00400688 /* RW-4R */
#define NV_PGRAPH_MEMFMT                                 0x0040068C /* RW-4R */
#define NV_PGRAPH_BOFFSET0                               0x00400630 /* RW-4R */
#define NV_PGRAPH_BOFFSET1                               0x00400634 /* RW-4R */
#define NV_PGRAPH_BOFFSET2                               0x00400638 /* RW-4R */
#define NV_PGRAPH_BOFFSET3                               0x0040063C /* RW-4R */
#define NV_PGRAPH_BPITCH0                                0x00400650 /* RW-4R */
#define NV_PGRAPH_BPITCH1                                0x00400654 /* RW-4R */
#define NV_PGRAPH_BPITCH2                                0x00400658 /* RW-4R */
#define NV_PGRAPH_BPITCH3                                0x0040065C /* RW-4R */

#define NV_PGRAPH_BPIXEL                                 0x004006a8 /* RW-4R */
#define NV_PGRAPH_BPIXEL_DEPTH0_FMT                             1:0 /* RWXVF */
#define NV_PGRAPH_BPIXEL_DEPTH0_FMT_Y16_BITS             0x00000000 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH0_FMT_BITS_8               0x00000001 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH0_FMT_BITS_16              0x00000002 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH0_FMT_BITS_32              0x00000003 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH0                                 2:2 /* RWXVF */
#define NV_PGRAPH_BPIXEL_DEPTH0_NOT_VALID                0x00000000 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH0_VALID                    0x00000001 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH1_FMT                             5:4 /* RWXVF */
#define NV_PGRAPH_BPIXEL_DEPTH1_FMT_Y16_BITS             0x00000000 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH1_FMT_BITS_8               0x00000001 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH1_FMT_BITS_16              0x00000002 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH1_FMT_BITS_32              0x00000003 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH1                                 6:6 /* RWXVF */
#define NV_PGRAPH_BPIXEL_DEPTH1_NOT_VALID                0x00000000 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH1_VALID                    0x00000001 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH2_FMT                             9:8 /* RWXVF */
#define NV_PGRAPH_BPIXEL_DEPTH2_FMT_Y16_BITS             0x00000000 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH2_FMT_BITS_8               0x00000001 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH2_FMT_BITS_16              0x00000002 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH2_FMT_BITS_32              0x00000003 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH2                               10:10 /* RWXVF */
#define NV_PGRAPH_BPIXEL_DEPTH2_NOT_VALID                0x00000000 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH2_VALID                    0x00000001 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH3_FMT                           13:12 /* RWXVF */
#define NV_PGRAPH_BPIXEL_DEPTH3_FMT_Y16_BITS             0x00000000 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH3_FMT_BITS_8               0x00000001 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH3_FMT_BITS_16              0x00000002 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH3_FMT_BITS_32              0x00000003 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH3                               14:14 /* RWXVF */
#define NV_PGRAPH_BPIXEL_DEPTH3_NOT_VALID                0x00000000 /* RW--V */
#define NV_PGRAPH_BPIXEL_DEPTH3_VALID                    0x00000001 /* RW--V */

#define NV_PGRAPH_PATT_COLOR0_0                          0x00400600 /* RW-4R */
#define NV_PGRAPH_PATT_COLOR0_1                          0x00400604 /* RW-4R */
#define NV_PGRAPH_PATT_COLOR1_0                          0x00400608 /* RW-4R */
#define NV_PGRAPH_PATT_COLOR1_1                          0x0040060C /* RW-4R */
#define NV_PGRAPH_PATTERN(i)                     (0x00400610+(i)*4) /* RW-4A */
#define NV_PGRAPH_PATTERN_SHAPE                          0x00400618 /* RW-4R */
#define NV_PGRAPH_PATTERN_SHAPE_VALUE                           1:0 /* RWXVF */
#define NV_PGRAPH_PATTERN_SHAPE_VALUE_8X8                0x00000000 /* RW--V */
#define NV_PGRAPH_PATTERN_SHAPE_VALUE_64X1               0x00000001 /* RW--V */
#define NV_PGRAPH_PATTERN_SHAPE_VALUE_1X64               0x00000002 /* RW--V */
#define NV_PGRAPH_MONO_COLOR0                            0x0040061C /* RW-4R */
#define NV_PGRAPH_ROP3                                   0x00400624 /* RW-4R */
#define NV_PGRAPH_PLANE_MASK                             0x00400628 /* RW-4R */
#define NV_PGRAPH_CHROMA                                 0x0040062C /* RW-4R */
#define NV_PGRAPH_BETA                                   0x00400640 /* RW-4R */
#define NV_PGRAPH_CONTROL_OUT                            0x00400644 /* RW-4R */
#define NV_PGRAPH_ABS_X_RAM(i)                   (0x00400400+(i)*4) /* RW-4A */
#define NV_PGRAPH_ABS_X_RAM__SIZE_1                              32 /*       */
#define NV_PGRAPH_ABS_Y_RAM(i)                   (0x00400480+(i)*4) /* RW-4A */
#define NV_PGRAPH_ABS_Y_RAM__SIZE_1                              32 /*       */

#define NV_PGRAPH_XY_LOGIC_MISC0                         0x00400514 /* RW-4R */
#define NV_PGRAPH_XY_LOGIC_MISC1                         0x00400518 /* RW-4R */
#define NV_PGRAPH_XY_LOGIC_MISC2                         0x0040051C /* RW-4R */
#define NV_PGRAPH_XY_LOGIC_MISC1_DVDY_VALUE              0x00000000 /* RWI-V */
#define NV_PGRAPH_XY_LOGIC_MISC3                         0x00400520 /* RW-4R */
#define NV_PGRAPH_X_MISC                                 0x00400500 /* RW-4R */
#define NV_PGRAPH_Y_MISC                                 0x00400504 /* RW-4R */
#define NV_PGRAPH_ABS_UCLIP_XMIN                         0x0040053C /* RW-4R */
#define NV_PGRAPH_ABS_UCLIP_XMAX                         0x00400544 /* RW-4R */
#define NV_PGRAPH_ABS_UCLIP_YMIN                         0x00400540 /* RW-4R */
#define NV_PGRAPH_ABS_UCLIP_YMAX                         0x00400548 /* RW-4R */
#define NV_PGRAPH_ABS_UCLIPA_XMIN                        0x00400560 /* RW-4R */
#define NV_PGRAPH_ABS_UCLIPA_XMAX                        0x00400568 /* RW-4R */
#define NV_PGRAPH_ABS_UCLIPA_YMIN                        0x00400564 /* RW-4R */
#define NV_PGRAPH_ABS_UCLIPA_YMAX                        0x0040056C /* RW-4R */
#define NV_PGRAPH_SOURCE_COLOR                           0x0040050C /* RW-4R */
#define NV_PGRAPH_EXCEPTIONS                             0x00400508 /* RW-4R */
#define NV_PGRAPH_ABS_ICLIP_XMAX                         0x00400534 /* RW-4R */
#define NV_PGRAPH_ABS_ICLIP_YMAX                         0x00400538 /* RW-4R */
#define NV_PGRAPH_CLIPX_0                                0x00400524 /* RW-4R */
#define NV_PGRAPH_CLIPX_1                                0x00400528 /* RW-4R */
#define NV_PGRAPH_CLIPY_0                                0x0040052c /* RW-4R */
#define NV_PGRAPH_CLIPY_1                                0x00400530 /* RW-4R */
#define NV_PGRAPH_DMA_INTR_0                             0x00401100 /* RW-4R */
#define NV_PGRAPH_DMA_INTR_0_INSTANCE                           0:0 /* RWXVF */
#define NV_PGRAPH_DMA_INTR_0_INSTANCE_NOT_PENDING        0x00000000 /* R---V */
#define NV_PGRAPH_DMA_INTR_0_INSTANCE_PENDING            0x00000001 /* R---V */
#define NV_PGRAPH_DMA_INTR_0_INSTANCE_RESET              0x00000001 /* -W--V */
#define NV_PGRAPH_DMA_INTR_0_PRESENT                            4:4 /* RWXVF */
#define NV_PGRAPH_DMA_INTR_0_PRESENT_NOT_PENDING         0x00000000 /* R---V */
#define NV_PGRAPH_DMA_INTR_0_PRESENT_PENDING             0x00000001 /* R---V */
#define NV_PGRAPH_DMA_INTR_0_PRESENT_RESET               0x00000001 /* -W--V */
#define NV_PGRAPH_DMA_INTR_0_PROTECTION                         8:8 /* RWXVF */
#define NV_PGRAPH_DMA_INTR_0_PROTECTION_NOT_PENDING      0x00000000 /* R---V */
#define NV_PGRAPH_DMA_INTR_0_PROTECTION_PENDING          0x00000001 /* R---V */
#define NV_PGRAPH_DMA_INTR_0_PROTECTION_RESET            0x00000001 /* -W--V */
#define NV_PGRAPH_DMA_INTR_0_LINEAR                           12:12 /* RWXVF */
#define NV_PGRAPH_DMA_INTR_0_LINEAR_NOT_PENDING          0x00000000 /* R---V */
#define NV_PGRAPH_DMA_INTR_0_LINEAR_PENDING              0x00000001 /* R---V */
#define NV_PGRAPH_DMA_INTR_0_LINEAR_RESET                0x00000001 /* -W--V */
#define NV_PGRAPH_DMA_INTR_0_NOTIFY                           16:16 /* RWXVF */
#define NV_PGRAPH_DMA_INTR_0_NOTIFY_NOT_PENDING          0x00000000 /* R---V */
#define NV_PGRAPH_DMA_INTR_0_NOTIFY_PENDING              0x00000001 /* R---V */
#define NV_PGRAPH_DMA_INTR_0_NOTIFY_RESET                0x00000001 /* -W--V */
#define NV_PGRAPH_DMA_INTR_EN_0                          0x00401140 /* RW-4R */
#define NV_PGRAPH_DMA_CONTROL                            0x00401210 /* RW-4R */

#define NV_PFB                                0x00100FFF:0x00100000 /* RW--D */
#define NV_PFB_BOOT_0                                    0x00100000 /* RW-4R */
#define NV_PFB_BOOT_0_RAM_AMOUNT                                1:0 /* RWIVF */
#define NV_PFB_BOOT_0_RAM_AMOUNT_1MB                     0x00000000 /* RW--V */
#define NV_PFB_BOOT_0_RAM_AMOUNT_2MB                     0x00000001 /* RW--V */
#define NV_PFB_BOOT_0_RAM_AMOUNT_4MB                     0x00000002 /* RW--V */

#define NV_PFB_CONFIG_0                                  0x00100200 /* RW-4R */
#define NV_PFB_CONFIG_0_RESOLUTION                              5:0 /* RWIVF */
#define NV_PFB_CONFIG_0_RESOLUTION_320_PIXELS            0x0000000a /* RW--V */
#define NV_PFB_CONFIG_0_RESOLUTION_400_PIXELS            0x0000000d /* RW--V */
#define NV_PFB_CONFIG_0_RESOLUTION_480_PIXELS            0x0000000f /* RW--V */
#define NV_PFB_CONFIG_0_RESOLUTION_512_PIXELS            0x00000010 /* RW--V */
#define NV_PFB_CONFIG_0_RESOLUTION_640_PIXELS            0x00000014 /* RW--V */
#define NV_PFB_CONFIG_0_RESOLUTION_800_PIXELS            0x00000019 /* RW--V */
#define NV_PFB_CONFIG_0_RESOLUTION_960_PIXELS            0x0000001e /* RW--V */
#define NV_PFB_CONFIG_0_RESOLUTION_1024_PIXELS           0x00000020 /* RW--V */
#define NV_PFB_CONFIG_0_RESOLUTION_1152_PIXELS           0x00000024 /* RW--V */
#define NV_PFB_CONFIG_0_RESOLUTION_1280_PIXELS           0x00000028 /* RW--V */
#define NV_PFB_CONFIG_0_RESOLUTION_1600_PIXELS           0x00000032 /* RW--V */
#define NV_PFB_CONFIG_0_RESOLUTION_DEFAULT               0x00000014 /* RWI-V */
#define NV_PFB_CONFIG_0_PIXEL_DEPTH                             9:8 /* RWIVF */
#define NV_PFB_CONFIG_0_PIXEL_DEPTH_8_BITS               0x00000001 /* RW--V */
#define NV_PFB_CONFIG_0_PIXEL_DEPTH_16_BITS              0x00000002 /* RW--V */
#define NV_PFB_CONFIG_0_PIXEL_DEPTH_32_BITS              0x00000003 /* RW--V */
#define NV_PFB_CONFIG_0_PIXEL_DEPTH_DEFAULT              0x00000001 /* RWI-V */
#define NV_PFB_CONFIG_0_TILING                                12:12 /* RWIVF */
#define NV_PFB_CONFIG_0_TILING_ENABLED                   0x00000000 /* RW--V */
#define NV_PFB_CONFIG_0_TILING_DISABLED                  0x00000001 /* RWI-V */

#define NV_PFB_BOOT_10                                   0x0010020C /* RW-4R */


#define NV_PRAMIN                             0x00FFFFFF:0x00C00000
#define NV_PNVM                               0x00BFFFFF:0x00800000
#define NV_CHAN0                              0x0080ffff:0x00800000

#define NV_UROP                               0x00421FFF:0x00420000 /* -W--D */
#define NV_UCHROMA                            0x00431FFF:0x00430000 /* -W--D */
#define NV_UPLANE                             0x00441FFF:0x00440000 /* -W--D */
#define NV_UCLIP                              0x00451FFF:0x00450000 /* -W--D */
#define NV_UPATT                              0x00461FFF:0x00460000 /* -W--D */
#define NV_ULINE                              0x00491FFF:0x00490000 /* -W--D */
#define NV_ULIN                               0x004A1FFF:0x004A0000 /* -W--D */
#define NV_UTRI                               0x004B1FFF:0x004B0000 /* -W--D */
#define NV_URECT                              0x00471FFF:0x00470000 /* -W--D */
#define NV_UBLIT                              0x00501FFF:0x00500000 /* -W--D */
#define NV_UBITMAP                            0x00521FFF:0x00520000 /* -W--D */

#define NV_PVGA0			      0x000C0000
#define NV_PVGA1			      0x00601000
#define NV_PVGA2			      0x00681000

#endif

#define INREG(addr) v_readl(addr)
#define OUTREG(addr, val) v_writel(val, addr)

#define INREG8(addr) v_readb(addr)
#define OUTREG8(addr, val) v_writeb(val, addr)

#define INREG16(addr) v_readw(addr)
#define OUTREG16(addr, val) v_writew(val, addr)

#define START_ADDR_HI        0x0C /* p246 */
#define START_ADDR_LO        0x0D /* p247 */
#define VERT_SYNC_END        0x11 /* p249 */
#define EXT_VERT_TOTAL       0x30 /* p257 */
#define EXT_VERT_DISPLAY     0x31 /* p258 */
#define EXT_VERT_SYNC_START  0x32 /* p259 */
#define EXT_VERT_BLANK_START 0x33 /* p260 */
#define EXT_HORIZ_TOTAL      0x35 /* p261 */
#define EXT_HORIZ_BLANK      0x39 /* p261 */
#define EXT_START_ADDR       0x40 /* p262 */
#define EXT_START_ADDR_ENABLE    0x80
#define EXT_OFFSET           0x41 /* p263 */
#define EXT_START_ADDR_HI    0x42 /* p263 */
#define INTERLACE_CNTL       0x70 /* p264 */
#define INTERLACE_ENABLE         0x80
#define INTERLACE_DISABLE        0x00

#define IO_CTNL            0x80
#define EXTENDED_ATTR_CNTL     0x02
#define EXTENDED_CRTC_CNTL     0x01

#define ADDRESS_MAPPING    0x10
#define PAGE_TO_LOCAL_MEM_ENABLE 0x10
#define GTT_MEM_MAP_ENABLE     0x08
#define PACKED_MODE_ENABLE     0x04
#define LINEAR_MODE_ENABLE     0x02
#define PAGE_MAPPING_ENABLE    0x01

#define DRAM_ROW_CNTL_HI   0x3002

#define BITBLT_CNTL        0x7000c
#define COLEXP_MODE            0x30
#define COLEXP_8BPP            0x00
#define COLEXP_16BPP           0x10
#define COLEXP_24BPP           0x20
#define COLEXP_RESERVED        0x30
#define BITBLT_STATUS          0x01
#define DISPLAY_CNTL       0x70008
#define VGA_WRAP_MODE          0x02
#define VGA_WRAP_AT_256KB      0x00
#define VGA_NO_WRAP            0x02
#define GUI_MODE               0x01
#define STANDARD_VGA_MODE      0x00
#define HIRES_MODE             0x01
#define PIXPIPE_CONFIG_0   0x70009
#define DAC_8_BIT              0x80
#define DAC_6_BIT              0x00
#define HW_CURSOR_ENABLE       0x10
#define EXTENDED_PALETTE       0x01
#define PIXPIPE_CONFIG_1   0x7000a
#define DISPLAY_COLOR_MODE     0x0F
#define DISPLAY_VGA_MODE       0x00
#define DISPLAY_8BPP_MODE      0x02
#define DISPLAY_15BPP_MODE     0x04
#define DISPLAY_16BPP_MODE     0x05
#define DISPLAY_24BPP_MODE     0x06
#define DISPLAY_32BPP_MODE     0x07
#define PIXPIPE_CONFIG_2   0x7000b
#define DISPLAY_GAMMA_ENABLE   0x08
#define DISPLAY_GAMMA_DISABLE  0x00

#define VCLK2_VCO_M        0x6008 /* treat as 16 bit? (includes msbs) */
#define VCLK2_VCO_N        0x600a
#define VCLK2_VCO_DIV_SEL  0x6012

#define CURSOR_CONTROL     0x70080
#define CURSOR_ORIGIN_SCREEN   0x00
#define CURSOR_ORIGIN_DISPLAY  0x10
#define CURSOR_MODE            0x07
#define CURSOR_MODE_DISABLE    0x00
#define CURSOR_MODE_32_4C_AX   0x01
#define CURSOR_MODE_64_3C      0x04
#define CURSOR_MODE_64_4C_AX   0x05
#define CURSOR_MODE_64_4C      0x06
#define CURSOR_MODE_RESERVED   0x07
#define CURSOR_BASEADDR    0x70084
#define CURSOR_X_LO        0x70088
#define CURSOR_X_HI        0x70089
#define CURSOR_Y_LO        0x7008A
#define CURSOR_Y_HI        0x7008B

#define FWATER_BLC       0x20d8
#define MM_BURST_LENGTH     0x00700000
#define MM_FIFO_WATERMARK   0x0001F000
#define LM_BURST_LENGTH     0x00000700
#define LM_FIFO_WATERMARK   0x0000001F
 

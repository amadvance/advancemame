#if 1
//#define mb()    __asm__ __volatile__ ("lock; addl $0,0(%%esp)": : :"memory")
#define mb()    __asm__ __volatile__ ("": : :"memory")
#define rage_inb(a) v_readb(a)
#define rage_inl(a) v_readl(a)
#define rage_outb(a,v) do {v_writeb(v,a); mb();}while(0)
#define rage_outl(a,v) do {v_writel(v,a); mb();}while(0)
#else
#define rage_inb(a) port_in(a)
#define rage_inl(a) port_inl(a)
#define rage_outb(a,v) port_out_r(a,v)
#define rage_outl(a,v) port_outl_r(a,v)
#endif

#define SPARSE_IO 0
#define BLOCK_IO 1

#define _ByteMask(__Byte)	((unsigned char)(-1) << (8 * (__Byte)))
#define GetByte(_Value, _Byte)	GetBits(_Value, _ByteMask(_Byte))

#define GetReg(_Register, _Index)                               \
        (                                                       \
                rage_outb(_Register, _Index),                        \
                rage_inb(_Register + 1)                              \
        )

#define ATIIOPort(_PortTag)							\
        (((ATIIODecoding == SPARSE_IO) ?					\
          (((_PortTag) & SPARSE_IO_SELECT) | ((_PortTag) & IO_BYTE_SELECT)) :	\
          (((_PortTag) & BLOCK_IO_SELECT)  | ((_PortTag) & IO_BYTE_SELECT))) |	\
         ATIIOBase)

#define ATTRX			0x03c0u
#define ATTRD			0x03c1u
#define SEQX			0x03c4u
#define SEQD			0x03c5u
#define GRAX			0x03ceu
#define GRAD			0x03cfu
#define GENS1			0x03dau
#define CRTX			0x03d4u
#define CRTD			0x03d5u

#define SPARSE_IO_BASE		0x03fcu
#define SPARSE_IO_SELECT	0xfc00u
#define BLOCK_IO_BASE		0xff00u
#define BLOCK_IO_SELECT		0x00fcu
#define IO_BYTE_SELECT		0x0003u

#define _UnitOf(___Value)	((((___Value) ^ ((___Value) - 1)) + 1) >> 1)
#define GetBits(__Value, _Mask)	(((__Value) & (_Mask)) / _UnitOf(_Mask))
#define SetBits(__Value, _Mask)	(((__Value) * _UnitOf(_Mask)) & (_Mask))
#define IOPortTag(_SparseIOSelect, _BlockIOSelect)	\
	(SetBits(_SparseIOSelect, SPARSE_IO_SELECT) | \
	 SetBits(_BlockIOSelect, BLOCK_IO_SELECT))
#define SparseIOTag(_IOSelect)	IOPortTag(_IOSelect, (unsigned)(-1))
#define BlockIOTag(_IOSelect)	IOPortTag((unsigned)(-1), _IOSelect)

#define CRTC_H_TOTAL_DISP	IOPortTag(0x00u, 0x00u)
#define CRTC_H_SYNC_STRT_WID	IOPortTag(0x01u, 0x01u)
#define CRTC_V_TOTAL_DISP	IOPortTag(0x02u, 0x02u)
#define CRTC_V_SYNC_STRT_WID	IOPortTag(0x03u, 0x03u)
#define CRTC_VLINE_CRNT_VLINE	IOPortTag(0x04u, 0x04u)
#define CRTC_OFF_PITCH		IOPortTag(0x05u, 0x05u)

#define SCRATCH_REG0		IOPortTag(0x10u, 0x20u)
#define OVR_WID_LEFT_RIGHT	IOPortTag(0x09u, 0x11u)
#define OVR_WID_TOP_BOTTOM	IOPortTag(0x0au, 0x12u)
#define CLOCK_CNTL		IOPortTag(0x12u, 0x24u)
#define BUS_CNTL		IOPortTag(0x13u, 0x28u)
#define MEM_VGA_WP_SEL		IOPortTag(0x15u, 0x2du)
#define MEM_VGA_RP_SEL		IOPortTag(0x16u, 0x2eu)
#define DAC_CNTL		IOPortTag(0x18u, 0x31u)
#define DAC_REGS		IOPortTag(0x17u, 0x30u)	/* 4 separate bytes */
#define CRTC_GEN_CNTL		IOPortTag(0x07u, 0x07u)
#define DSP_CONFIG		BlockIOTag(0x08u)	/* VTB/GTB/LT */
#define DSP_ON_OFF		BlockIOTag(0x09u)	/* VTB/GTB/LT */
#define VGA_DSP_CONFIG		BlockIOTag(0x13u)	/* VTB/GTB/LT */
#define VGA_DSP_ON_OFF		BlockIOTag(0x14u)	/* VTB/GTB/LT */
#define HW_DEBUG		BlockIOTag(0x1fu)
#define OVR_CLR			IOPortTag(0x08u, 0x10u)
#define MEM_INFO		IOPortTag(0x14u, 0x2cu) /* Renamed MEM_CNTL */
#define CONFIG_CNTL		IOPortTag(0x1au, 0x37u)
#define GEN_TEST_CNTL		IOPortTag(0x19u, 0x34u)
#define CRTC_INT_CNTL		IOPortTag(0x06u, 0x06u)
#define CONFIG_STATUS64_0	IOPortTag(0x1cu, 0x39u)

#define MEM_BUF_CNTL		BlockIOTag(0x0bu)	/* VTB/GTB/LT */
#define SHARED_CNTL		BlockIOTag(0x0cu)	/* VTB/GTB/LT */
#define SHARED_MEM_CONFIG	BlockIOTag(0x0du)	/* GT3 */

#define CUR_CLR0                IOPortTag(0x0bu, 0x18u)
#define CUR_CLR1                IOPortTag(0x0cu, 0x19u)
#define CUR_OFFSET              IOPortTag(0x0du, 0x1au)
#define CUR_HORZ_VERT_POSN      IOPortTag(0x0eu, 0x1bu)
#define CUR_HORZ_VERT_OFF       IOPortTag(0x0fu, 0x1cu)

#define CONFIG_CHIP_ID		IOPortTag(0x1bu, 0x38u) /* Read */
#define CFG_CHIP_TYPE0			0x000000fful
#define CFG_CHIP_TYPE1			0x0000ff00ul
#define CFG_CHIP_TYPE			0x0000fffful
#define CFG_CHIP_CLASS			0x00ff0000ul
#define CFG_CHIP_REV			0xff000000ul
#define CFG_CHIP_VERSION		0x07000000ul	/* 264xT */
#define CFG_CHIP_FOUNDRY		0x38000000ul	/* 264xT */
#define CFG_CHIP_REVISION		0xc0000000ul	/* 264xT */
#define CFG_MEM_TYPE_T			0x00000007ul
#define CFG_MEM_TYPE			0x00000038ul

#define DAC_8BIT_EN			0x00000100ul

#define CRTC_OFFSET			0x000ffffful
#define CRTC_H_TOTAL			0x000001fful
#define CRTC_H_DISP			0x01ff0000ul
#define CRTC_H_SYNC_STRT		0x000000fful
#define CRTC_H_SYNC_DLY			0x00000700ul
#define CRTC_H_SYNC_POL			0x00200000ul
#define CRTC_H_SYNC_WID			0x001f0000ul
#define CRTC_H_SYNC_STRT_HI		0x00001000ul
#define CRTC_V_TOTAL			0x000007fful
#define CRTC_V_DISP			0x07ff0000ul
#define CRTC_V_SYNC_STRT		0x000007fful
#define CRTC_V_SYNC_WID			0x001f0000ul
#define CRTC_V_SYNC_POL			0x00200000ul
#define CRTC_PIX_WIDTH			0x00000700ul
#define CRTC_PIX_WIDTH_1BPP			0x00000000ul
#define CRTC_PIX_WIDTH_4BPP			0x00000100ul
#define CRTC_PIX_WIDTH_8BPP			0x00000200ul
#define CRTC_PIX_WIDTH_15BPP			0x00000300ul
#define CRTC_PIX_WIDTH_16BPP			0x00000400ul
#define CRTC_PIX_WIDTH_24BPP			0x00000500ul
#define CRTC_PIX_WIDTH_32BPP			0x00000600ul
#define CRTC_VBLANK			0x00000001ul
#define CRTC_VBLANK_INT_EN		0x00000002ul
#define CRTC_VBLANK_INT			0x00000004ul
#define CRTC_VLINE_INT_EN		0x00000008ul
#define CRTC_VLINE_INT			0x00000010ul
#define CRTC_VLINE_SYNC			0x00000020ul
#define CRTC_FRAME			0x00000040ul
/*	?				0x0000ff80ul */
#define CRTC_SNAPSHOT_INT_EN		0x00000080ul	/* GT3 */
#define CRTC_SNAPSHOT_INT		0x00000100ul	/* GT3 */
#define CRTC_I2C_INT_EN			0x00000200ul	/* GT3 */
#define CRTC_I2C_INT			0x00000400ul	/* GT3 */
#define CRTC_CAPBUF0_INT_EN		0x00010000ul	/* VT/GT */
#define CRTC_CAPBUF0_INT		0x00020000ul	/* VT/GT */
#define CRTC_CAPBUF1_INT_EN		0x00040000ul	/* VT/GT */
#define CRTC_CAPBUF1_INT		0x00080000ul	/* VT/GT */
#define CRTC_OVERLAY_EOF_INT_EN		0x00100000ul	/* VT/GT */
#define CRTC_OVERLAY_EOF_INT		0x00200000ul	/* VT/GT */
#define CRTC_ONESHOT_CAP_INT_EN		0x00400000ul	/* VT/GT */
#define CRTC_ONESHOT_CAP_INT		0x00800000ul	/* VT/GT */
#define CRTC_BUSMASTER_EOL_INT_EN	0x01000000ul	/* VTB/GTB/LT */
#define CRTC_BUSMASTER_EOL_INT		0x02000000ul	/* VTB/GTB/LT */
#define CRTC_GP_INT_EN			0x04000000ul	/* VTB/GTB/LT */
#define CRTC_GP_INT			0x08000000ul	/* VTB/GTB/LT */
/*	?				0xf0000000ul */
#define CRTC_VBLANK_BIT2		0x80000000ul	/* GT3 */
#define CRTC_INT_ENS	/* *** UPDATE ME *** */		\
		(					\
			CRTC_VBLANK_INT_EN |		\
			CRTC_VLINE_INT_EN |		\
			CRTC_SNAPSHOT_INT_EN |		\
			CRTC_I2C_INT_EN |		\
			CRTC_CAPBUF0_INT_EN |		\
			CRTC_CAPBUF1_INT_EN |		\
			CRTC_OVERLAY_EOF_INT_EN |	\
			CRTC_ONESHOT_CAP_INT_EN |	\
			CRTC_BUSMASTER_EOL_INT_EN |	\
			CRTC_GP_INT_EN |		\
			0				\
		)
#define CRTC_INT_ACKS	/* *** UPDATE ME *** */		\
		(					\
			CRTC_VBLANK_INT |		\
			CRTC_VLINE_INT |		\
			CRTC_SNAPSHOT_INT |		\
			CRTC_I2C_INT |			\
			CRTC_CAPBUF0_INT |		\
			CRTC_CAPBUF1_INT |		\
			CRTC_OVERLAY_EOF_INT |		\
			CRTC_ONESHOT_CAP_INT |		\
			CRTC_BUSMASTER_EOL_INT |	\
			CRTC_GP_INT |			\
			0				\
		)
#define CRTC_DBL_SCAN_EN		0x00000001ul
#define CRTC_INTERLACE_EN		0x00000002ul
#define CRTC_HSYNC_DIS			0x00000004ul
#define CRTC_VSYNC_DIS			0x00000008ul
#define CRTC_CSYNC_EN			0x00000010ul
#define CRTC_PIX_BY_2_EN		0x00000020ul
#define CRTC_DISPLAY_DIS		0x00000040ul
#define CRTC_VGA_XOVERSCAN		0x00000080ul
#define CRTC_PIX_WIDTH			0x00000700ul
#define CRTC_PIX_WIDTH_1BPP			0x00000000ul
#define CRTC_PIX_WIDTH_4BPP			0x00000100ul
#define CRTC_PIX_WIDTH_8BPP			0x00000200ul
#define CRTC_PIX_WIDTH_15BPP			0x00000300ul
#define CRTC_PIX_WIDTH_16BPP			0x00000400ul
#define CRTC_PIX_WIDTH_24BPP			0x00000500ul
#define CRTC_PIX_WIDTH_32BPP			0x00000600ul
/*	?					0x00000700ul */
#define CRTC_BYTE_PIX_ORDER		0x00000800ul
#define CRTC_FIFO_OVERFILL		0x0000c000ul	/* VT/GT */
#define CRTC_FIFO_LWM			0x000f0000ul
#define CRTC_VGA_128KAP_PAGING		0x00100000ul	/* VT/GT */
#define CRTC_DISPREQ_ONLY		0x00200000ul	/* VT/GT */
#define CRTC_VFC_SYNC_TRISTATE		0x00200000ul	/* VTB/GTB/LT */
#define CRTC_LOCK_REGS			0x00400000ul	/* VT/GT */
#define CRTC_SYNC_TRISTATE		0x00800000ul	/* VT/GT */
#define CRTC_EXT_DISP_EN		0x01000000ul
#define CRTC_EN				0x02000000ul
#define CRTC_DISP_REQ_EN		0x04000000ul
#define CRTC_VGA_LINEAR			0x08000000ul
#define CRTC_VSYNC_FALL_EDGE		0x10000000ul
#define CRTC_VGA_TEXT_132		0x20000000ul
#define CRTC_CNT_EN			0x40000000ul
#define CRTC_CUR_B_TEST			0x80000000ul
#define DSP_OFF				0x000007fful
#define VGA_DSP_OFF			DSP_OFF
#define CRTC_PITCH			0xffc00000ul
#define CTL_MEM_SIZE			0x00000007ul	
#define CTL_MEM_SIZEB			0x0000000ful	/* VTB/GTB/LT */
#define PLL_WR_EN			0x00000200ul	/* For internal PLL */
#define PLL_ADDR			0x00007c00ul	/* For internal PLL */
#define BUS_HOST_ERR_INT_EN		0x00400000ul
#define BUS_HOST_ERR_INT		0x00800000ul
#define BUS_APER_REG_DIS		0x00000010ul	/* VTB/GTB/LT */
#define CLOCK_SELECT			0x0000000ful
#define CLOCK_DIVIDER			0x00000030ul
#define CLOCK_STROBE			0x00000040ul

#define GEN_CUR_EN			0x00000080ul

#define BUS_WS				0x0000000ful
#define BUS_DBL_RESYNC			0x00000001ul	/* VTB/GTB/LT */
#define BUS_MSTR_RESET			0x00000002ul	/* VTB/GTB/LT */
#define BUS_FLUSH_BUF			0x00000004ul	/* VTB/GTB/LT */
#define BUS_STOP_REQ_DIS		0x00000008ul	/* VTB/GTB/LT */
#define BUS_ROM_WS			0x000000f0ul
#define BUS_APER_REG_DIS		0x00000010ul	/* VTB/GTB/LT */
#define BUS_EXTRA_PIPE_DIS		0x00000020ul	/* VTB/GTB/LT */
#define BUS_MASTER_DIS			0x00000040ul	/* VTB/GTB/LT */
#define BUS_ROM_WRT_EN			0x00000080ul	/* GT3 */
#define BUS_ROM_PAGE			0x00000f00ul
#define BUS_ROM_DIS			0x00001000ul
#define BUS_IO_16_EN			0x00002000ul	/* GX */
#define BUS_PCI_READ_RETRY_EN		0x00002000ul	/* VTB/GTB/LT */
#define BUS_DAC_SNOOP_EN		0x00004000ul
#define BUS_PCI_RETRY_EN		0x00008000ul	/* VT/GT */
#define BUS_PCI_WRT_RETRY_EN		0x00008000ul	/* VTB/GTB/LT */
#define BUS_FIFO_WS			0x000f0000ul
#define BUS_RETRY_WS			0x000f0000ul	/* VTB/GTB/LT */
#define BUS_FIFO_ERR_INT_EN		0x00100000ul
#define BUS_MSTR_RD_MULT		0x00100000ul	/* VTB/GTB/LT */
#define BUS_FIFO_ERR_INT		0x00200000ul
#define BUS_MSTR_RD_LINE		0x00200000ul	/* VTB/GTB/LT */
#define BUS_HOST_ERR_INT_EN		0x00400000ul
#define BUS_SUSPEND			0x00400000ul	/* GT3 */
#define BUS_HOST_ERR_INT		0x00800000ul
#define BUS_LAT16X			0x00800000ul	/* GT3 */
#define BUS_PCI_DAC_WS			0x07000000ul
#define BUS_RD_DISCARD_EN		0x01000000ul	/* VTB/GTB/LT */
#define BUS_RD_ABORT_EN			0x02000000ul	/* VTB/GTB/LT */
#define BUS_MSTR_WS			0x04000000ul	/* VTB/GTB/LT */
#define BUS_PCI_DAC_DLY			0x08000000ul
#define BUS_EXT_REG_EN			0x08000000ul	/* VT/GT */
#define BUS_PCI_MEMW_WS			0x10000000ul
#define BUS_MSTR_DISCONNECT_EN		0x10000000ul	/* VTB/GTB/LT */
#define BUS_PCI_BURST_DEC		0x20000000ul	/* GX/CX */
#define BUS_BURST			0x20000000ul	/* 264xT */
#define BUS_WRT_BURST			0x20000000ul	/* VTB/GTB/LT */
#define BUS_RDY_READ_DLY		0xc0000000ul
#define BUS_READ_BURST			0x40000000ul	/* VTB/GTB/LT */
#define BUS_RDY_READ_DLY_B		0x80000000ul	/* VTB/GTB/LT */

#define CFG_MEM_VGA_AP_EN		0x00000004ul
#define DSP_XCLKS_PER_QW		0x00003ffful
#define VGA_DSP_XCLKS_PER_QW		DSP_XCLKS_PER_QW
#define DSP_FLUSH_WB			0x00008000ul
#define DSP_LOOP_LATENCY		0x000f0000ul
#define DSP_PRECISION			0x00700000ul
#define DSP_OFF				0x000007fful
#define DSP_ON				0x07ff0000ul

#define PLL_VCLK_CNTL		0x05u
#define PLL_VCLK_POST_DIV	0x06u
#define PLL_VCLK0_FB_DIV	0x07u
#define PLL_VCLK0_XDIV		0x10u
#define PLL_XCLK_CNTL		0x0bu	/* VT/GT */
#define PLL_VCLK_RESET		0x04u
#define PLL_XCLK_SRC_SEL	0x07u	/* VTB/GTB/LT */
#define PLL_MCLK_FB_DIV		0x04u
#define PLL_MFB_TIMES_4_2B	0x08u
#define PLL_REF_DIV		0x02u

#define CTL_MEM_TRAS			0x00070000ul	/* VTB/GTB/LT */
#define CTL_MEM_TRCD			0x00000c00ul	/* VTB/GTB/LT */
#define CTL_MEM_TCRD			0x00001000ul	/* VTB/GTB/LT */
#define CTL_MEM_TRP			0x00000300ul	/* VTB/GTB/LT */

#define CTL_MEM_BNDRY			0x00030000ul
#define CTL_MEM_BNDRY_EN		0x00040000ul

#define ATI_CHIP_88800GXC 16    /* Mach64 */
#define ATI_CHIP_88800GXD 17    /* Mach64 */
#define ATI_CHIP_88800GXE 18    /* Mach64 */
#define ATI_CHIP_88800GXF 19    /* Mach64 */
#define ATI_CHIP_88800GX  20    /* Mach64 */
#define ATI_CHIP_88800CX  21    /* Mach64 */
#define ATI_CHIP_264CT    22    /* Mach64 */
#define ATI_CHIP_264ET    23    /* Mach64 */
#define ATI_CHIP_264VT    24    /* Mach64 */
#define ATI_CHIP_264GT    25    /* Mach64 */
#define ATI_CHIP_264VTB   26    /* Mach64 */
#define ATI_CHIP_264GTB   27    /* Mach64 */
#define ATI_CHIP_264VT3   28    /* Mach64 */
#define ATI_CHIP_264GTDVD 29    /* Mach64 */
#define ATI_CHIP_264LT    30    /* Mach64 */
#define ATI_CHIP_264VT4   31    /* Mach64 */
#define ATI_CHIP_264GT2C  32    /* Mach64 */
#define ATI_CHIP_264GTPRO 33    /* Mach64 */
#define ATI_CHIP_264LTPRO 34    /* Mach64 */
#define ATI_CHIP_Mach64   35    /* Mach64 */

#define MEM_264_NONE            0
#define MEM_264_DRAM            1
#define MEM_264_EDO             2
#define MEM_264_PSEUDO_EDO      3
#define MEM_264_SDRAM           4
#define MEM_264_SGRAM           5
#define MEM_264_TYPE_6          6
#define MEM_264_TYPE_7          7

#define ATIGetMach64PLLReg(_Index)                              \
        (                                                       \
                ATIAccessMach64PLLReg(_Index, 0),           \
                rage_inb(ATIIOPortCLOCK_CNTL + 2)                    \
        )
#define ATIPutMach64PLLReg(_Index, _Value)                      \
                ATIAccessMach64PLLReg(_Index, 1);            \
                rage_outb(ATIIOPortCLOCK_CNTL + 2, _Value)          


#if 1

#define OUT(val, data) port_out(data, io_base+val)
#define OUT16(val, data) port_outw(data, io_base+val)
#define OUT32(val, data) port_outl(data, io_base+val)
#define IN(val) port_in(io_base+val)
#define IN16(val) port_inw(io_base+val)
#define IN32(val) port_inl(io_base+val)

#else

#define OUT(val, data) *(MMIO_POINTER+0x20000+val)=data
#define OUT16(val, data) *(unsigned short*)(MMIO_POINTER+0x20000+val)=data
#define OUT32(val, data) *(unsigned int*)(MMIO_POINTER+0x20000+val)=data
#define IN(val) *(MMIO_POINTER+0x20000+val)
#define IN16(val) *(unsigned short*)(MMIO_POINTER+0x20000+val)
#define IN32(val) *(unsigned int*)(MMIO_POINTER+0x20000+val)

#endif

#define FIFO_SIZE	0x1f

/* IO register offsets. */
#define FIFO_SWAP_NO    0x00 /* FIFO. No byte swap. */
#define FIFO_SWAP_END   0x04 /* FIFO. Swap bytes 3<>0, 2<>1. */
#define FIFO_SWAP_INHW  0x08 /* FIFO. Swap bytes 3<>2, 1<>0. */
#define FIFO_SWAP_HW    0x0c /* FIFO. Swap half-words. */
#define FIFOINFREE      0x40 /* Input FIFO free entry count. */
#define FIFOOUTVALID    0x41 /* Output FIFO valid entry count. */
#define COMM            0x42 /* dual 4 bit communications ports */
#define MEMENDIAN       0x43 /* set byte swapping on PCI mem accesses */
#define INTR            0x44 /* which interrupts occurred */
#define INTREN          0x46 /* enable different interrupts */
#define DEBUGREG        0x48 /* soft resets, RISC hold/single step */
#define LOWWATERMARK    0x49 /* Input FIFO low water mark for interrupt */
#define STATUS          0x4A /* specifies which blocks of the V2000 are busy */
#define XBUSCTL         0x4B /* XBus control register */
#define PCITEST         0x4C /* PCI test */
#define DMACMDPTR       0x50 /* DMA command list pointer */
#define DMA_ADDRESS     0x54 /* DMA data address */
#define DMA_COUNT       0x58 /* DMA remaining transfer count */
#define STATEINDEX      0x60 /* state index info */
#define STATEDATA       0x64 /* state data info */
#define SCLKPLL         0x68 /* system clock PLL control register */
#define SCRATCH         0x70 /* 16-bit BIOS scratch space */
#define MODEREG         0x72 /* Mode -- to differentiate from old MODE */
#define MODE_           MODEREG
#define	MODE		MODEREG
#define BANKSELECT      0x74 /* Local memory to A0000 mapping */
#define	BANKSELECT_PHYSADDR	((unsigned long)(0xA0000))
#define CRTCTEST        0x80 /* CRTC test register */
#define CRTCCTL         0x84 /* CRTC mode */
#define CRTCHORZ        0x88 /* CRTC horizontal timing */
#define CRTCVERT        0x8c /* CRTC vertical timing */
#define FRAMEBASEB      0x90 /* Stereoscopic frame base b address */
#define FRAMEBASEA      0x94 /* Frame base A address */
#define CRTCOFFSET      0x98 /* CRTC StrideOffset */
#define CRTCSTATUS      0x9c /* CRTC video scan position */
#define DRAMCTL         0xa0 /* DRAM timing */
#define MEMDIAG         0xa4 /* Memory diagnostic register #1 */
#define CURSORBASE      0xac /* cursor base address bits [23:10] aligne to 1024 byte boundary */
#define PALETTE         0xb0 /* Access to DAC */
#define PCLKPLL         0xc0 /* external device 0 */
#define VINEVENBASE     0xd0 /* video input even field base address */
#define VINODDBASE      0xd4 /* video input odd field base address */
#define WRITEINTR0ADDR  0xd8 /* Memory write interrupt address0 */
#define WRITEINTR1ADDR  0xdc /* Memory write interrupt address1 */
#define DEVICE0_V2x000  0xf0 /* external device 1 (PLL) */

/* 
 * PCLKPLL/SCLKPLL register bit defn
 */
#define PCLKPLLPMASK	0xffffe1ff
#define SCLKPLLPMASK	0xffffe1ff
#define MCLKPLLPMASK	0xfffe1fff
#define PLLPCLKP	9
#define PLLSCLKP	9
#define PLLMCLKP	13
#define PLLPCLKN	13
#define PLLSCLKN	17
#define PLLPCLKDOUBLE	26	/* bit 26 in PClkPLL register */

#define DIRECTPCLKMASK	0x00400000
#define DIRECTMCLKMASK	0x00800000

#define VGASTDCLOCK     0x100000
#define EXTRADIV2       0x200000 

#define PLLINCLKFREQ	14318 /* PLL input clk freq in KHz */

/*
 * memory controller 
 */
#define MCLK_BYPASSEDGEFREQ 90000 /* in KHz */

/*
 * Microcode commands
 */
#define	CMD_SETPALETTE	0x21

/*
 * MMIO registers
 */

#define MMIO_FIFOINFREE	    0x20040
#define MMIO_COMM           0x20042
#define MMIO_FIFOOUTVALID   0x20041
#define MMIO_INTR           0x20044 /* which interrupts occurred */
#define MMIO_DMACMDPTR      0x20050
#define MMIO_CRTCHORZ       0x20088 /* CRTC horizontal timing */
#define MMIO_CRTCVERT       0x2008c /* CRTC vertical timing */
#define MMIO_CRTCSTATUS	    0x2009c 
#define MMIO_DACRAMWRITEADR 0x200b0 /* Palette Write Index */
#define MMIO_DACRAMDATA     0x200b1 /* Palette Data */
#define MMIO_VINEVENBASE    0x200d0 /* video input even field base address */
#define MMIO_VINODDBASE     0x200d4 /* video input odd field base address */
#define MMIO_WRITEINTR0ADDR 0x200d8 /* Memory write interrupt address0 */
#define MMIO_WRITEINTR1ADDR 0x200dc /* Memory write interrupt address1 */

/* IO register flag bits */
/* _MASK defined for multi-bit values */
/* _ADDR defined for registers accessible from RISC */

/* COMM */
#define SYSSTATUS_MASK  0x0f /* host->RISC comm */
#define SYSSTATUS_SHIFT 0
#define RISCSTATUS_MASK 0xf0 /* RISC->host comm r/o */
#define RISCSTATUS_SHIFT 4
 
/* MEMENDIAN */
#define MEMENDIAN_NO    0       /* No byte swap. */
#define MEMENDIAN_END   1       /* Swap bytes 3<>0, 2<>1. */
#define MEMENDIAN_INHW  2       /* Swap bytes 3<>2, 1<>0. */
#define MEMENDIAN_HW    3       /* Swap half-words. */
#define MEMENDIAN_MASK  3
#define MEMENDIAN_SHIFT 0

#define DMABUSY         0x80    /* DMA busy r/o */
#define DMACMDPTR_DMABUSY       0x1     /* corresponding bit in other reg */

/* INTR */
#define VERTINTR        0x01 /* vert retrace */
#define FIFOLOWINTR     0x02 /* free entries rose above low water */
#define RISCINTR        0x04 /* RISC firmware interrupt */
#define HALTINTR        0x08 /* RISC halted */
#define FIFOERRORINTR   0x10 /* FIFO under/over flow */
#define DMAERRORINTR    0x20 /* PCI error during DMA */
#define DMAINTR         0x40 /* DMA done interrupt */
#define XINTR           0x80 /* external device pass thru intr */
#define VIDEOINEVENINTR	0x100 /* Video input even field interrupt */
#define VIDEOINODDINTR	0x100 /* Video input even field interrupt */

/* INTREN */
#define VERTINTREN      0x01 /* vert retrace */
#define FIFOLOWINTREN   0x02 /* free entries rose above low water */
#define RISCINTREN      0x04 /* RISC firmware interrupt */
#define HALTINTREN      0x08 /* RISC halted */
#define FIFOERRORINTREN 0x10 /* FIFO under/over flow */
#define DMAERRORINTREN  0x20 /* PCI error during DMA */
#define DMAINTREN       0x40 /* DMA done interrupt */
#define XINTREN         0x80 /* external device pass thru intr */

/* DEBUG */
#define SOFTRESET       0x01 /* soft reset chip */
#define HOLDRISC        0x02 /* stop RISC when set */
#define STEPRISC        0x04 /* single step RISC */
#define DIRECTSCLK      0x08 /* disable internal divide by 2 for sys clk */
#define SOFTVGARESET    0x10 /* assert VGA reset */
#define SOFTXRESET      0x20 /* assert XReset output to ext devices */

/* MODE_ register */
#define VESA_MODE       0x01 /* enable 0xA0000 in native mode */
#define VGA_MODE        0x02 /* VGA mode if set else native mode */
#define VGA_32          0x04 /* enable VGA 32 bit accesses */
#define DMA_EN          0x08 /* enable DMA accesses */

#define NATIVE_MODE     0    /* not VESA and not VGA */

/* DRAM register */
#define DRAMCTL_ADDR    0xffe00500

/* CRTC registers */
#define CRTCTEST_ADDR   0xffe00400
#define CRTCCTL_ADDR    0xffe00420
#define CRTCHORZ_ADDR   0xffe00440
#define CRTCVERT_ADDR   0xffe00460
#define FRAMEBASEB_ADDR 0xffe00480
#define FRAMEBASEA_ADDR 0xffe004a0
#define CRTCOFFSET_ADDR 0xffe004c0
#define CRTCSTATUS_ADDR 0xffe004e0

#define CRTCTEST_VIDEOLATENCY_MASK      0x1F
#define CRTCTEST_NOTVBLANK      0x10000
#define CRTCTEST_VBLANK         0x40000

#define CRTCCTL_SCRNFMT_MASK    0xF
#define CRTCCTL_VIDEOFIFOSIZE128        0x10
#define CRTCCTL_ENABLEDDC       0x20
#define CRTCCTL_DDCOUTPUT       0x40
#define CRTCCTL_DDCDATA         0x80
#define CRTCCTL_VSYNCHI         0x100
#define CRTCCTL_HSYNCHI         0x200
#define CRTCCTL_VSYNCENABLE     0x400
#define CRTCCTL_HSYNCENABLE     0x800
#define CRTCCTL_VIDEOENABLE     0x1000
#define CRTCCTL_STEREOSCOPIC    0x2000
#define CRTCCTL_FRAMEDISPLAYED  0x4000
#define CRTCCTL_FRAMEBUFFERBGR  0x8000
#define CRTCCTL_EVENFRAME       0x10000
#define CRTCCTL_LINEDOUBLE      0x20000
#define CRTCCTL_FRAMESWITCHED   0x40000
#define CRTCCTL_VIDEOFIFOSIZE256        0x800010

#define CRTCHORZ_ACTIVE_MASK    	0xFF
#define CRTCHORZ_ACTIVE_SHIFT    	0
#define CRTCHORZ_BACKPORCH_MASK 	0x7E00
#define CRTCHORZ_BACKPORCH_SHIFT 	11
#define CRTCHORZ_SYNC_MASK      	0x1F0000L
#define CRTCHORZ_SYNC_SHIFT      	16
#define CRTCHORZ_FRONTPORCH_MASK    0xE00000L
#define CRTCHORZ_FRONTPORCH_SHIFT   20

#define CRTCVERT_ACTIVE_MASK    	0x7FF
#define CRTCVERT_BACKPORCH_MASK 	0x1F800
#define CRTCVERT_SYNC_MASK      	0xE0000
#define CRTCVERT_FRONTPORCH_MASK    0x03F00000

#define CRTCOFFSET_MASK         0xFFFF

#define CRTCSTATUS_HORZCLOCKS_MASK      0xFF
#define CRTCSTATUS_HORZ_MASK    0x600
#define CRTCSTATUS_HORZ_FPORCH  0x200
#define CRTCSTATUS_HORZ_SYNC    0x600
#define CRTCSTATUS_HORZ_BPORCH  0x400
#define CRTCSTATUS_HORZ_ACTIVE  0x000
#define CRTCSTATUS_SCANLINESLEFT_MASK   0x003FF800
#define CRTCSTATUS_VERT_MASK    0xC00000
#define CRTCSTATUS_VERT_FPORCH  0x400000
#define CRTCSTATUS_VERT_SYNC    0xC00000
#define CRTCSTATUS_VERT_BPORCH  0x800000
#define CRTCSTATUS_VERT_ACTIVE  0x000000

/* RAMDAC registers - avail through I/O space */

#define DACRAMWRITEADR  0xb0
#define DACRAMDATA      0xb1
#define DACPIXELMSK     0xb2
#define DACRAMREADADR   0xb3
#define DACOVSWRITEADR  0xb4
#define DACOVSDATA      0xb5
#define DACCOMMAND0     0xb6
#define DACOVSREADADR   0xb7
#define DACCOMMAND1     0xb8
#define DACCOMMAND2     0xb9
#define DACSTATUS       0xba
#define DACCOMMAND3     0xba    /* accessed via unlocking/indexing */
#define DACCURSORDATA   0xbb
#define DACCURSORXLOW   0xbc
#define DACCURSORXHIGH  0xbd
#define DACCURSORYLOW   0xbe
#define DACCURSORYHIGH  0xbf

#define BT_CO_COLORWR_ADDR	DACOVSWRITEADR
#define BT_CO_COLORDATA		DACOVSDATA
#define BT_PTR_ROWOFFSET	32
#define BT_PTR_COLUMNOFFSET	32

/* PCLKPLL register */
#define PLLDEV          DEVICE0
#define VOUTEN			0x00080000L	/* bit 19 */
#define VGASCLKOVER2	0x00100000L	/* bit 20 */
#define PCLKSTARTEN		0x00800000L	/* bit 23 */

/* Some state indices */
#define STATEINDEX_IR   128
#define STATEINDEX_PC   129
#define STATEINDEX_S1   130

/* PCI configuration registers. */
#define CONFIGIOREG        0xE0000014
#define CONFIGENABLE       0xE0000004
#ifdef USEROM
#define CONFIGROMREG       0xE0000030
#endif

/* Cache parameters. */
#define ICACHESIZE       2048 /* I cache size. */
#define ICACHELINESIZE     32 /* I cache line size. */
#ifndef ICACHE_ONOFF_MASK
#define ICACHE_ONOFF_MASK  (((v_u32)1<<17)|(1<<3))
#define ICACHE_ON          ((0<<17)|(0<<3))
#define ICACHE_OFF         (((v_u32)1<<17)|(1<<3))
#endif

/* Video registers */
#define BT829_DEV                 DEVICE0
#define VIDEO_DECODER_DEV_ENABLE  0x4
#define VIDEO_DECODER_DEV_DISABLE 0x0

#define VINBASE_MASK     0x1FFFFFL
#define VINMAXVERT_SHIFT 24
#define VINSTRIDE_SHIFT  27
#define VINQSIZE_SHIFT   30

#define VINORDER_SHIFT   24
#define ACTIVE_LOW        0
#define ACTIVE_HI         1L
#define VINHSYNCHI_SHIFT 26
#define VINVSYNCHI_SHIFT 27
#define VINACTIVE_SHIFT  28
#define VINNOODD_SHIFT   29
#define VINENABLE_SHIFT  30

/* directly accessable RAMDAC registers */
#define BT485_WRITE_ADDR        0x00
#define BT485_RAMDAC_DATA       0x01
#define BT485_PIXEL_MASK        0x02
#define BT485_READ_ADDR         0x03
#define BT485_CURS_WR_ADDR      0x04
#define BT485_CURS_DATA         0x05
#define BT485_COMMAND_REG_0     0x06
#define BT485_CURS_RD_ADDR      0x07
#define BT485_COMMAND_REG_1     0x08
#define BT485_COMMAND_REG_2     0x09
#define BT485_STATUS_REG        0x0a
#define BT485_CURS_RAM_DATA     0x0b
#define BT485_CURS_X_LOW        0x0c
#define BT485_CURS_X_HIGH       0x0d
#define BT485_CURS_Y_LOW        0x0e
#define BT485_CURS_Y_HIGH       0x0f

/* indirectly accessable ramdac registers */
#define BT485_COMMAND_REG_3     0x01

/* bits in command register 0 */
#define BT485_CR0_EXTENDED_REG_ACCESS   0x80
#define BT485_CR0_SCLK_SLEEP_DISABLE    0x40
#define BT485_CR0_BLANK_PEDESTAL        0x20
#define BT485_CR0_SYNC_ON_BLUE          0x10
#define BT485_CR0_SYNC_ON_GREEN         0x08
#define BT485_CR0_SYNC_ON_RED           0x04
#define BT485_CR0_8_BIT_DAC             0x02
#define BT485_CR0_SLEEP_ENABLE          0x01

/* bits in command register 1 */
#define BT485_CR1_24BPP             0x00
#define BT485_CR1_16BPP             0x20
#define BT485_CR1_8BPP              0x40
#define BT485_CR1_4BPP              0x60
#define BT485_CR1_1BPP              0x80
#define BT485_CR1_BYPASS_CLUT       0x10
#define BT485_CR1_565_16BPP         0x08
#define BT485_CR1_555_16BPP         0x00
#define BT485_CR1_1_TO_1_16BPP      0x04
#define BT485_CR1_2_TO_1_16BPP      0x00
#define BT485_CR1_PD7_PIXEL_SWITCH  0x02
#define BT485_CR1_PIXEL_PORT_CD     0x01
#define BT485_CR1_PIXEL_PORT_AB     0x00

/* bits in command register 2 */
#define BT485_CR2_SCLK_DISABLE      0x80
#define BT485_TEST_PATH_SELECT      0x40
#define BT485_PIXEL_INPUT_GATE      0x20
#define BT485_PIXEL_CLK_SELECT      0x10
#define BT485_INTERLACE_SELECT      0x08
#define BT485_16BPP_CLUT_PACKED     0x04
#define BT485_X_WINDOW_CURSOR       0x03
#define BT485_2_COLOR_CURSOR        0x02
#define BT485_3_COLOR_CURSOR        0x01
#define BT485_DISABLE_CURSOR        0x00
#define BT485_CURSOR_MASK           0x03

/* bits in command register 3 */
#define BT485_4BPP_NIBBLE_SWAP      0x10
#define BT485_CLOCK_DOUBLER         0x08
#define BT485_64_BY_64_CURSOR       0x04
#define BT485_32_BY_32_CURSOR       0x00
#define BT485_SIZE_MASK             0x04

/* special constants for the Brooktree BT485 RAMDAC */
#define BT485_INPUT_LIMIT           110000000

typedef enum {
  V_PIXFMT_DSTFMT=0,
  V_PIXFMT_332=1,       /**/
#define V_PIXFMT_233 V_PIXFMT_332
  V_PIXFMT_8I=2,        /**/
  V_PIXFMT_8A=3,
  V_PIXFMT_565=4,       /**/
  V_PIXFMT_4444=5,      /**/
  V_PIXFMT_1555=6,      /**/
  /* 7 reserved */
  V_PIXFMT_4I_565=8,
  V_PIXFMT_4I_4444=9,
  V_PIXFMT_4I_1555=10,
  /* 11 reserved */
  V_PIXFMT_8888=12,     /**/
  V_PIXFMT_Y0CRY1CB=13
#define V_PIXFMT_Y0CBY1CR V_PIXFMT_Y0CRY1CB
  /* 14 reserved */
  /* 15 reserved */
} vpixfmt;


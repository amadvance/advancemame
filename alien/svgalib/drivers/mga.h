typedef int Bool;

#define TRUE 1
#define FALSE 0

#define INREG8(addr)		v_readb(addr)
#define INREG16(addr)		v_readw(addr)
#define INREG(addr)		v_readl(addr)
#define OUTREG8(addr, val)	v_writeb(val, addr)
#define OUTREG16(addr, val)	v_writew(val, addr)
#define OUTREG(addr, val)	v_writel(val, addr)

#define RAMDAC_OFFSET 0x3c00
#define MGA1064_INDEX 0
#define MGA1064_DATA 0x0a

typedef struct {
    unsigned char	ExtVga[6];
    unsigned char 	DacClk[6];
    unsigned char       DacRegs[32];
    unsigned int	Option;
    unsigned int	opmode;
} MGARegRec, *MGARegPtr;

#define MIL_TOTAL_REGS sizeof(MGARegRec)

#define inMGAdreg(reg) INREG8(RAMDAC_OFFSET + (reg))

#define outMGAdreg(reg, val) OUTREG8(RAMDAC_OFFSET + (reg), val)

#define inMGAdac(reg) \
        (outMGAdreg(MGA1064_INDEX, reg), inMGAdreg(MGA1064_DATA))

#define outMGAdac(reg, val) \
        (outMGAdreg(MGA1064_INDEX, reg), outMGAdreg(MGA1064_DATA, val))

#define inTi3026dreg(reg) INREG8(RAMDAC_OFFSET + (reg))

#define outTi3026dreg(reg, val) OUTREG8(RAMDAC_OFFSET + (reg), val)

#define inTi3026(reg) \
	(outTi3026dreg(TVP3026_INDEX, reg), inTi3026dreg(TVP3026_DATA))

#define outTi3026(reg, mask, val) \
	do { /* note: mask and reg may get evaluated twice */ \
	    unsigned char tmp = (mask) ? (inTi3026(reg) & (mask)) : 0; \
	    outTi3026dreg(TVP3026_INDEX, reg); \
	    outTi3026dreg(TVP3026_DATA, tmp | (val)); \
	} while (0)

/* TVP3026 direct registers */

#define TVP3026_INDEX           0x00
#define TVP3026_WADR_PAL        0x00
#define TVP3026_COL_PAL         0x01
#define TVP3026_PIX_RD_MSK      0x02
#define TVP3026_RADR_PAL        0x03
#define TVP3026_CUR_COL_ADDR    0x04
#define TVP3026_CUR_COL_DATA    0x05
#define TVP3026_DATA            0x0a
#define TVP3026_CUR_RAM         0x0b
#define TVP3026_CUR_XLOW        0x0c
#define TVP3026_CUR_XHI         0x0d
#define TVP3026_CUR_YLOW        0x0e
#define TVP3026_CUR_YHI         0x0f

/* TVP3026 indirect registers */

#define TVP3026_SILICON_REV     0x01
#define TVP3026_CURSOR_CTL      0x06
#define TVP3026_LATCH_CTL       0x0f
#define TVP3026_TRUE_COLOR_CTL  0x18
#define TVP3026_MUX_CTL         0x19
#define TVP3026_CLK_SEL         0x1a
#define TVP3026_PAL_PAGE        0x1c
#define TVP3026_GEN_CTL         0x1d
#define TVP3026_MISC_CTL        0x1e
#define TVP3026_GEN_IO_CTL      0x2a
#define TVP3026_GEN_IO_DATA     0x2b
#define TVP3026_PLL_ADDR        0x2c
#define TVP3026_PIX_CLK_DATA    0x2d
#define TVP3026_MEM_CLK_DATA    0x2e
#define TVP3026_LOAD_CLK_DATA   0x2f
#define TVP3026_KEY_RED_LOW     0x32
#define TVP3026_KEY_RED_HI      0x33
#define TVP3026_KEY_GREEN_LOW   0x34
#define TVP3026_KEY_GREEN_HI    0x35
#define TVP3026_KEY_BLUE_LOW    0x36
#define TVP3026_KEY_BLUE_HI     0x37
#define TVP3026_KEY_CTL         0x38
#define TVP3026_MCLK_CTL        0x39
#define TVP3026_SENSE_TEST      0x3a
#define TVP3026_TEST_DATA       0x3b
#define TVP3026_CRC_LSB         0x3c
#define TVP3026_CRC_MSB         0x3d
#define TVP3026_CRC_CTL         0x3e
#define TVP3026_ID              0x3f
#define TVP3026_RESET           0xff

#define MGAREG_ZORG             0x1c0c
#define MGAGDAC_XVREFCTRL               0x18

#define MGA1064_MUL_CTL         0x19
#define MGA1064_MUL_CTL_8bits           0x0
#define MGA1064_MUL_CTL_15bits          0x01
#define MGA1064_MUL_CTL_16bits          0x02
#define MGA1064_MUL_CTL_24bits          0x03
#define MGA1064_MUL_CTL_32bits          0x04
#define MGA1064_MUL_CTL_2G8V16bits              0x05
#define MGA1064_MUL_CTL_G16V16bits              0x06
#define MGA1064_MUL_CTL_32_24bits               0x07

#define MGA1064_SYS_PLL_M       0x2c
#define MGA1064_SYS_PLL_N       0x2d
#define MGA1064_SYS_PLL_P       0x2e
#define MGA1064_SYS_PLL_STAT    0x2f

#define MGA1064_PIX_PLLA_M      0x44
#define MGA1064_PIX_PLLA_N      0x45
#define MGA1064_PIX_PLLA_P      0x46
#define MGA1064_PIX_PLLB_M      0x48
#define MGA1064_PIX_PLLB_N      0x49
#define MGA1064_PIX_PLLB_P      0x4a
#define MGA1064_PIX_PLLC_M      0x4c
#define MGA1064_PIX_PLLC_N      0x4d
#define MGA1064_PIX_PLLC_P      0x4e

#define MGA1064_VID_PLL_P       0x8D
#define MGA1064_VID_PLL_M       0x8E
#define MGA1064_VID_PLL_N       0x8F


#define OPTION_MASK		0xffeffeff
#define PCI_OPTION_REG		0x40

//#define OPTION1_MASK    0xFFFFFEFF
#define OPTION2_MASK    0xFFFFFFFF
#define OPTION3_MASK    0xFFFFFFFF
#define OPTION1_MASK    0xFFFC0FF

#define MGAREG_C2CTL            0x3c10
#define MGAREG_C2HPARAM         0x3c14
#define MGAREG_C2HSYNC          0x3c18
#define MGAREG_C2VPARAM         0x3c1c
#define MGAREG_C2VSYNC          0x3c20
#define MGAREG_C2STARTADD0      0x3c28

#define MGAREG_C2OFFSET         0x3c40
#define MGAREG_C2DATACTL        0x3c4c

#define MGA1064_DISP_CTL        0x8a
#define MGA1064_SYNC_CTL        0x8b
#define MGA1064_PWR_CTL         0xa0


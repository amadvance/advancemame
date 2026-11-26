// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/* 3DO M2 BDA Triangle Engine */

#include "driver.h"
#include "includes/konamim2.h"

#include <math.h>
#include <stdbool.h>

typedef enum
{
	REG_WRITE,
	REG_INVALID,
	REG_SET,
	REG_CLEAR,
} te_reg_wmode;

typedef enum
{
	TE_STOPPED,
	TE_PAUSED,
	TE_RUNNING,
} te_state;

typedef enum
{
	PIP_RAM_WORDS           = 256,
	TEXTURE_RAM_WORDS       = 4096,
	PIP_RAM_BYTEMASK        = PIP_RAM_WORDS * 4 - 1,
	TEXTURE_RAM_BYTEMASK    = TEXTURE_RAM_WORDS * 4 - 1,
} misc;

typedef enum
{
	INST_WRITE_REG  = 0x10000000,
	INST_VTX_SHORT  = 0x20000000,
	INST_VTX_LONG   = 0x30000000,
	INST_VTX_POINT  = 0x40000000,
	INST_MASK       = 0xf0000000,
} inst_type;

typedef enum
{
	VTX_FLAG_SHAD   = 0x00010000,
	VTX_FLAG_TEXT   = 0x00020000,
	VTX_FLAG_PRSP   = 0x00040000,
	VTX_FLAG_NEW    = 0x00080000,
	VTX_FLAG_RM     = 0x00100000,
} vtx_flag;

typedef struct
{
	float x, y;
	float r, g, b, a;
	float w;
	float uw, vw;
	float iv[8];
} se_vtx;

typedef struct
{
	float y23;
	float y31;
	float y12;
	float x23;
	float x31;
	float x12;
	float xstep_long;
	float iAria;
} slope_params;

typedef struct  { UINT8 r, g, b, a; } rgba;

static union
{
	struct
	{
		UINT32 te_master_mode;
		UINT32 reserved;
		UINT32 teicntl_data;
		UINT32 teicntl;
		UINT32 tedcntl_data;
		UINT32 tedcntl;
		UINT32 iwp;
		UINT32 irp;
		UINT32 int_enable;
		UINT32 int_status;
		UINT32 vertex_ctrl;
	};
	UINT32 m_regs[11];
} m_gc;

static union
{
	struct
	{
		se_vtx  vertices[3];
		UINT32    reserved[16];
		UINT32    vertex_state;
	};
	UINT32 m_regs[68];
} m_se;

static struct
{
	union
	{
		struct
		{
			UINT32 es_cntl;
			UINT32 es_capaddr;
			UINT32 es_capdata;
		};
		UINT32 m_regs[3];
	};
	union
	{
		struct
		{
			UINT32 x1;
			UINT32 y1;
			UINT32 r1;
			UINT32 g1;
			UINT32 b1;
			UINT32 a1;
			UINT32 w1;
			UINT32 uw1;
			UINT32 vw1;
			UINT32 x2;
			UINT32 y2;
			UINT32 y3;
			UINT32 xstep_0;
			UINT32 xstep_1;
			UINT32 xstep_long;
			UINT32 xystep_0;
			UINT32 xystep_1;
			UINT32 xystep_long;
			UINT32 dy_0;
			UINT32 dy_1;
			UINT32 dy_long;
			UINT32 ddx_r;
			UINT32 ddx_g;
			UINT32 ddx_b;
			UINT32 ddx_a;
			UINT32 ddx_w;
			UINT32 ddx_uw;
			UINT32 ddx_vw;
			UINT32 slope_r;
			UINT32 slope_g;
			UINT32 slope_b;
			UINT32 slope_a;
			UINT32 slope_uw;
			UINT32 slope_vw;
			UINT32 slope_w;
			UINT32 r2l;
		};
		UINT32 m_buffer_regs[36];
	};
} m_es;

static union
{
	struct
	{
		UINT32 tex_cntl;
		UINT32 texld_cntl;
		UINT32 tex_addr_cntl;
		UINT32 tex_pip_cntl;
		UINT32 tex_tab_cntl;
		union
		{
			UINT32 tex_lod_base0;
			UINT32 texld_dstbase;
			UINT32 tex_mm_dstbase;
		};
		UINT32 tex_lod_base1;
		UINT32 tex_lod_base2;
		UINT32 tex_lod_base3;
		union
		{
			UINT32 texld_srcbase;
			UINT32 tex_mm_srcbase;
		};
		union
		{
			UINT32 tex_bytecnt;
			UINT32 tex_rowcnt;
			UINT32 tex_texcnt;
		};
		UINT32 uv_max;
		UINT32 uv_mask;
		UINT32 tex_srctype01;
		UINT32 tex_srctype23;
		UINT32 tex_exptype;
		union
		{
			UINT32 tex_srcconst0;
			UINT32 tex_clrconst;
			UINT32 tex_catconst0;
		};
		union
		{
			UINT32 tex_srcconst1;
			UINT32 tex_pipconst1;
		};
		union
		{
			UINT32 tex_srcconst2;
			UINT32 tex_catiConst0;
		};
		union
		{
			UINT32 tex_srcconst3;
			UINT32 tex_caticonst1;
		};
		UINT32 tex_srcexp;
	};
	UINT32 m_regs[21];
} m_tm;

static union
{
	struct
	{
		UINT32 snoop;
		UINT32 supergen_ctrl;
		UINT32 usergen_ctrl;
		UINT32 discard_ctrl;
		UINT32 status;
		UINT32 int_ctrl;
		UINT32 fbclip;
		UINT32 x_winclip;
		UINT32 y_winclip;
		UINT32 dst_ctrl;
		UINT32 dst_baseaddr;
		UINT32 dst_xstride;
		UINT32 src_ctrl;
		UINT32 src_baseaddr;
		UINT32 src_xstride;
		UINT32 src_offset;
		UINT32 z_ctrl;
		UINT32 z_baseaddr;
		UINT32 z_offset;
		UINT32 z_clip;
		UINT32 ssbdsb_ctrl;
		UINT32 const_in;
		UINT32 txt_mult_cntl;
		UINT32 txt_coef_const0;
		UINT32 txt_coef_const1;
		UINT32 src_mult_cntl;
		UINT32 src_coef_const0;
		UINT32 src_coef_const1;
		UINT32 alu_ctrl;
		UINT32 src_alpha_ctrl;
		UINT32 dst_alpha_ctrl;
		UINT32 dst_alpha_const;
		UINT32 dither_mat_a;
		UINT32 dither_mat_b;
	};
	UINT32 m_regs[34];
} m_db;

static struct
{
	UINT32    x;
	UINT32    y;
	UINT32    w;

	rgba    ti;
	UINT8 ssb;

	rgba    src;
	UINT8 dsb;

	rgba    srcpath;
	rgba    texpath;
	rgba    blend;
	rgba    dst;
} m_dbstate;

static te_state m_state;

static UINT32 m_pipram[PIP_RAM_WORDS];
static UINT32 m_tram[TEXTURE_RAM_WORDS];

static void te_set_interrupt(UINT32 mask);
static void te_update_interrupts(void);
static void te_master_mode_w(UINT32 data, te_reg_wmode wmode);
static void te_teicntl_w(UINT32 data, te_reg_wmode wmode);
static void te_tedcntl_w(UINT32 data, te_reg_wmode wmode);
static void te_add_vertex(const se_vtx *vtx, UINT32 flags);
static void te_calculate_slope(const slope_params *sp, float q1, float q2, float q3, float *slope_out, float *ddx_out);
static void te_setup_triangle(UINT32 flags);
static void te_walk_edges(UINT32 wrange);
static void te_texcoord_gen(UINT32 wrange, UINT32 uw, UINT32 vw, UINT32 w,
								UINT32 * uo, UINT32 * vo, UINT32 * wo);
static UINT32 te_lod_calc(UINT32 u0, UINT32 v0, UINT32 u1, UINT32 v1);
static UINT32 te_get_tram_bitdepth(void);
static void te_get_texture_color(UINT32 u, UINT32 v, UINT32 lod,
									UINT32 * r, UINT32 * g, UINT32 * b, UINT32 * a, UINT32 * s);
static void te_addr_calc(UINT32 u, UINT32 v, UINT32 lod,
							UINT32 * texaddr, UINT32 * texbit, UINT32 * tdepth);
static void te_get_texel(UINT32 tex_addr, UINT32 tex_bit, UINT32 tdepth,
							UINT32 * r, UINT32 * g, UINT32 * b, UINT32 * a, UINT32 * ssb);
static void te_texture_blend(
	UINT32 ri, UINT32 gi, UINT32 bi, UINT32 ai,
	UINT32 rt, UINT32 gt, UINT32 bt, UINT32 at, UINT32 ssbt,
	UINT32 *ro, UINT32 *go, UINT32 *bo, UINT32 *ao, UINT32 *ssbo);
static void te_select_lerp( UINT32 sel,
								UINT32 ri, UINT32 gi, UINT32 bi, UINT32 ai,
								UINT32 rt, UINT32 gt, UINT32 bt, UINT32 at, UINT32 ssbt,
								UINT32 * ar, UINT32 * ag, UINT32 * ab );
static void te_select_mul(UINT32 sel, UINT32 ai, UINT32 at, UINT32 ssbt,
							   UINT32 * a );
static void te_write_dst_pixel(void);
static void te_destination_blend(UINT32 x, UINT32 y, UINT32 w, const rgba * ti_color, UINT8 ssb);
static void te_select_tex_pixel(void);
static void te_select_src_pixel(void);
static UINT8 te_dither(UINT8 in, UINT8 dithval);
static UINT8 te_get_src_coef(UINT8 cti, UINT8 dm2const0, UINT8 dm2const1);
static UINT8 te_get_tex_coef(UINT8 cs, UINT8 dm1const0, UINT8 dm1const1);
static void te_select_alpha_dsb(void);
static UINT8 te_color_blend(UINT8 ct, UINT8 cti, UINT8 cs, UINT8 csrc,
								UINT8 dm10, UINT8 dm11,
								UINT8 dm20, UINT8 dm21);
static UINT8 te_alu_calc(UINT16 a, UINT16 b);
static void te_walk_span(UINT32 wrange, bool omit_right,
							 UINT32 y, UINT32 xs, UINT32 xe,
							 INT32 r, INT32 g, INT32 b, INT32 a,
							 UINT32 uw, UINT32 vw,
							 UINT32 w);
static UINT32 te_irp_fetch(void);
static float te_irp_fetch_float(void);
static void te_illegal_inst(void);
static void te_execute(void);
static UINT8 te_read_tram8(offs_t address);
static UINT16 te_read_tram16(offs_t address);
static UINT32 te_read_tram32(offs_t address);
static void te_write_tram8(offs_t address, UINT8 data);
static void te_write_tram16(offs_t address, UINT16 data);
static void te_write_tram32(offs_t address, UINT32 data);
static UINT8 te_read_pipram8(offs_t address);
static UINT16 te_read_pipram16(offs_t address);
static UINT32 te_read_pipram32(offs_t address);
static void te_write_pipram32(offs_t address, UINT32 data);
static UINT32 te_readbits_from_ram(UINT32 * src_addr, UINT32 * bit_offs, UINT32 bits);
static void te_load_texture(void);

static INT32 te_clamp(INT32 value, INT32 min, INT32 max)
{
	return value < min ? min : (value > max ? max : value);
}

static INT32 te_sign_extend(UINT32 value, unsigned bits)
{
	UINT32 sign = 1U << (bits - 1);
	return (value ^ sign) - sign;
}

static const UINT32 xy_bits = 11;
static const UINT32 color_bits = 8;
static const UINT32 text_bits[] = { 10, 7, 4, 1 };
static const INT32 depth_bits[] = { 0, -3, -6, -9 };
static const UINT32 xy_rshift = 12;
static const UINT32 color_rshift = 4;
static const UINT32 xy_one = 1;

#define TEMASTER_MODE_RESET             0x00000001
#define TEMASTER_MODE_DTEXT             0x00000002
#define TEMASTER_MODE_DSHADE            0x00000004
#define TEMASTER_MODE_DBLEND            0x00000008
#define TEMASTER_MODE_DZBUF             0x00000010
#define TEMASTER_MODE_DDITH             0x00000020

#define TEICNTL_INT                     0x00000001
#define TEICNTL_STEP                    0x00000002
#define TEICNTL_STPL                    0x00000004
#define TEICNTL_STPI                    0x00000008
#define TEICNTL_RSTRT                   0x00000010
#define TEICNTL_STRT                    0x00000020

#define TEDCNTL_TLD                     0x00000001
#define TEDCNTL_JA                      0x00000002
#define TEDCNTL_JR                      0x00000004
#define TEDCNTL_INT                     0x00000008
#define TEDCNTL_PSE                     0x00000010
#define TEDCNTL_SYNC                    0x00000020

#define INTSTAT_DEFERRED_INSTR          0x00000100
#define INTSTAT_IMMEDIATE_INSTR         0x00000200
#define INTSTAT_LIST_END                0x00000400
#define INTSTAT_WINDOW_CLIP             0x00000800
#define INTSTAT_SPECIAL_INSTR           0x00001000
#define INTSTAT_UNIMPLEMENTED_INSTR     0x00002000
#define INTSTAT_SUPERVISOR              0x00004000
#define INTSTAT_ANY_RENDER              0x00008000
#define INTSTAT_Z_FUNC                  0x00010000
#define INTSTAT_ALU_STATUS              0x00020000
#define INTSTAT_FB_CLIP                 0x00040000
#define INTSTAT_IMMEDIATE               0x00080000

#define VERTEXSTATE_TSORT_MASK              0x00000007
#define VERTEXSTATE_TSORT_OMN               0x00000001
#define VERTEXSTATE_TSORT_MNO               0x00000002
#define VERTEXSTATE_TSORT_ONM               0x00000003
#define VERTEXSTATE_TSORT_NOM               0x00000004
#define VERTEXSTATE_TSORT_MON               0x00000005
#define VERTEXSTATE_TSORT_NMO               0x00000006

#define VERTEXSTATE_VCNT_SHIFT              3
#define VERTEXSTATE_VCNT_MASK               0x00000018

#define ESCNTL_DSPOFF                       0x00000001
#define ESCNTL_DUSCAN                       0x00000002
#define ESCNTL_PERSPECTIVEOFF               0x00000004

#define TXTCNTL_MMDMA_TRAM_ON               0x00000004
#define TXTCNTL_MMDMA_PIP_ON                0x00000008
#define TXTCNTL_SNOOP_ON                    0x00000020

#define TXTLDCNTL_SRCBITOFFS                0x00000007
#define TXTLDCNTL_LDMODE_MASK               0x00000300
#define TXTLDCNTL_LDMODE_TEXLOAD            0x00000000
#define TXTLDCNTL_LDMODE_MMDMA              0x00000100
#define TXTLDCNTL_LDMODE_PIPLOAD            0x00000200
#define TXTLDCNTL_LDMODE_RESERVED           0x00000300
#define TXTLDCNTL_COMPRESSED                0x00000400

#define TXTADDRCNTL_LODMAX_MASK             0x0000000f
#define TXTADDRCNTL_FILTERSEL_MASK          0x00000003
#define TXTADDRCNTL_FILTERSEL_POINT         0x00000000
#define TXTADDRCNTL_FILTERSEL_LINEAR        0x00000001
#define TXTADDRCNTL_FILTERSEL_BILINEAR      0x00000002
#define TXTADDRCNTL_FILTERSEL_QUASITRI      0x00000003
#define TXTADDRCNTL_R12FILTERSEL_SHIFT      4
#define TXTADDRCNTL_R3FILTERSEL_SHIFT       7
#define TXTADDRCNTL_R45FILTERSEL_SHIFT      10
#define TXTADDRCNTL_LOOKUP_EN               0x00002000

#define TXTPIPCNTL_INDEX_OFFSET             0x000000ff
#define TXTPIPCNTL_COLORSEL_MASK            0x00000700
#define TXTPIPCNTL_COLORSEL_SHIFT           8
#define TXTPIPCNTL_ALPHASEL_MASK            0x00003800
#define TXTPIPCNTL_ALPHASEL_SHIFT           11
#define TXTPIPCNTL_SSBSEL_MASK              0x0001c000
#define TXTPIPCNTL_SSBSEL_SHIFT             14

#define TXTPIPCNTL_SEL_CONSTANT             0
#define TXTPIPCNTL_SEL_TRAM                 1
#define TXTPIPCNTL_SEL_PIP                  2

#define TXTTABCNTL_C_ASEL_MASK              0x00000007
#define TXTTABCNTL_C_ASEL_SHIFT             0
#define TXTTABCNTL_C_BSEL_MASK              0x00000038
#define TXTTABCNTL_C_BSEL_SHIFT             3
#define TXTTABCNTL_C_TSEL_MASK              0x000001c0
#define TXTTABCNTL_C_TSEL_SHIFT             6

#define TXTTABCNTL_C_ABTSEL_AITER           0
#define TXTTABCNTL_C_ABTSEL_CITER           1
#define TXTTABCNTL_C_ABTSEL_AT              2
#define TXTTABCNTL_C_ABTSEL_CT              3
#define TXTTABCNTL_C_ABTSEL_ACONST          4
#define TXTTABCNTL_C_ABTSEL_CCONST          5

#define TXTTABCNTL_C_OSEL_MASK              0x00000600
#define TXTTABCNTL_C_OSEL_SHIFT             9

#define TXTTABCNTL_CO_SEL_CITER             0
#define TXTTABCNTL_CO_SEL_CT                1
#define TXTTABCNTL_CO_SEL_BLEND             2
#define TXTTABCNTL_CO_SEL_RESERVED          3

#define TXTTABCNTL_A_ASEL_MASK              0x00001800
#define TXTTABCNTL_A_ASEL_SHIFT             11
#define TXTTABCNTL_A_BSEL_MASK              0x00006000
#define TXTTABCNTL_A_BSEL_SHIFT             13

#define TXTTABCNTL_A_ABSEL_AITER            0
#define TXTTABCNTL_A_ABSEL_AT               1
#define TXTTABCNTL_A_ABSEL_ACONST           2

#define TXTTABCNTL_A_OSEL_MASK              0x00018000
#define TXTTABCNTL_A_OSEL_SHIFT             15

#define TXTTABCNTL_AO_SEL_AITER             0
#define TXTTABCNTL_AO_SEL_AT                1
#define TXTTABCNTL_AO_SEL_BLEND             2
#define TXTTABCNTL_AO_SEL_RESERVED          3

#define TXTTABCNTL_BLENDOP_MASK             0x00020000
#define TXTTABCNTL_BLENDOP_SHIFT            17
#define TXTTABCNTL_BLENDOP_LERP             0
#define TXTTABCNTL_BLENDOP_MULT             1

#define TXTTABCONST_BLUE                    0x000000ff
#define TXTTABCONST_BLUE_SHIFT              0
#define TXTTABCONST_GREEN                   0x0000ff00
#define TXTTABCONST_GREEN_SHIFT             8
#define TXTTABCONST_RED                     0x00ff0000
#define TXTTABCONST_RED_SHIFT               16
#define TXTTABCONST_ALPHA                   0x7f000000
#define TXTTABCONST_ALPHA_SHIFT             24
#define TXTTABCONST_SSB                     0x80000000

#define TXTLDDSTBASE_ADDR                   0x00003ffc

#define TXTLODBASE_MASK                     0x00003ffc

#define TXTLDSRCBASE_ADDR                   0x00003fff

#define TXTLDBYTECNT_COUNT                  0x0fffffff
#define TXTLDROWCNT_COUNT                   0x0fffffff
#define TXTLDTEXCNT_COUNT                   0x0fffffff

#define TxTLDWIDTH_SRCROW                   0x0000ffff
#define TxTLDWIDTH_DSTROW_SHIFT             16
#define TxTLDWIDTH_DSTROW                   0xffff0000

#define TXTUVMAX_VMAX_MASK                  0x000003ff
#define TXTUVMAX_VMAX_SHIFT                 0
#define TXTUVMAX_UMAX_MASK                  0x03ff0000
#define TXTUVMAX_UMAX_SHIFT                 16

#define TXTUVMASK_VMASK_MASK                0x000003ff
#define TXTUVMASK_VMASK_SHIFT               0
#define TXTUVMASK_UMASK_MASK                0x03ff0000
#define TXTUVMASK_UMASK_SHIFT               16

#define TXTEXPFORM_CDEPTH_MASK              0x0000000f
#define TXTEXPFORM_CDEPTH_SHIFT             0
#define TXTEXPFORM_IDEPTH_MASK              0x0000000f
#define TXTEXPFORM_IDEPTH_SHIFT             0
#define TXTEXPFORM_ADEPTH_MASK              0x000000f0
#define TXTEXPFORM_ADEPTH_SHIFT             4
#define TXTEXPFORM_TRANSPARENT              0x00000100
#define TXTEXPFORM_SSBON                    0x00000200
#define TXTEXPFORM_COLORON                  0x00000400
#define TXTEXPFORM_INDEXON                  0x00000400
#define TXTEXPFORM_ALPHAON                  0x00000800
#define TXTEXPFORM_LITERAL                  0x00001000

#define DBSNOOP_DESTWRSNOOP             0x00000001
#define DBSNOOP_SRCRDSNOOP              0x00000002
#define DBSNOOP_ZWRSNOOP                0x00000004
#define DBSNOOP_ZRDSNOOP                0x00000008

#define DBSUPERGENCTL_DESTOUTEN         0x00000001
#define DBSUPERGENCTL_DESTWR16BEN       0x00000002
#define DBSUPERGENCTL_ZWR16BEN          0x00000004

#define DBUSERGENCTL_DESTOUT_MASK       0x0000000f
#define DBUSERGENCTL_DITHEREN           0x00000010
#define DBUSERGENCTL_SRCINEN            0x00000020
#define DBUSERGENCTL_BLENDEN            0x00000040
#define DBUSERGENCTL_WCLIPOUTEN         0x00000080
#define DBUSERGENCTL_WCLIPINEN          0x00000100
#define DBUSERGENCTL_ZOUTEN             0x00000200
#define DBUSERGENCTL_ZBUFEN             0x00000400

#define DBDISCARDCTL_ADISEN             0x00000001
#define DBDISCARDCTL_RGBDISEN           0x00000002
#define DBDISCARDCTL_SSBDISEN           0x00000004
#define DBDISCARDCTL_ZCLIPDISEN         0x00000008

#define DBSTATUS_ANYREND                0x00000001
#define DBSTATUS_ZFUNC_GT               0x00000002
#define DBSTATUS_ZFUNC_EQ               0x00000004
#define DBSTATUS_ZFUNC_LT               0x00000008
#define DBSTATUS_ALUSTAT_BLUE_GT        0x00000010
#define DBSTATUS_ALUSTAT_BLUE_EQ        0x00000020
#define DBSTATUS_ALUSTAT_BLUE_LT        0x00000040
#define DBSTATUS_ALUSTAT_GREEN_GT       0x00000080
#define DBSTATUS_ALUSTAT_GREEN_EQ       0x00000100
#define DBSTATUS_ALUSTAT_GREEN_LT       0x00000200
#define DBSTATUS_ALUSTAT_RED_GT         0x00000400
#define DBSTATUS_ALUSTAT_RED_EQ         0x00000800
#define DBSTATUS_ALUSTAT_RED_LT         0x00001000
#define DBSTATUS_ZCLIP                  0x00002000
#define DBSTATUS_WINCLIP                0x00004000
#define DBSTATUS_FBCLIP                 0x00008000

#define DBINTCNTL_ZFUNCSTATINTEN_MASK   0x00000003
#define DBINTCNTL_ZFUNCSTATINTEN_MASK   0x00000003

#define DBFBCLIP_YFBCLIP_MASK           0x000007ff
#define DBFBCLIP_YFBCLIP_SHIFT          0
#define DBFBCLIP_XFBCLIP_MASK           0x07ff0000
#define DBFBCLIP_XFBCLIP_SHIFT          16

#define DBFBXWINCLIP_XMAX_MASK          0x000007ff
#define DBFBXWINCLIP_XMAX_SHIFT         0
#define DBFBXWINCLIP_XMIN_MASK          0x07ff0000
#define DBFBXWINCLIP_XMIN_SHIFT         16

#define DBFBYWINCLIP_YMAX_MASK          0x000007ff
#define DBFBYWINCLIP_YMAX_SHIFT         0
#define DBFBYWINCLIP_YMIN_MASK          0x07ff0000
#define DBFBYWINCLIP_YMIN_SHIFT         16

#define DBDESTCNTL_32BPP                0x00000001

#define DBDEST_XSTRIDE                  0x000007ff

#define DBSRCCNTL_32BPP                 0x00000001
#define DBSRCCNTL_MSBREP                0x00000002

#define DBSRCXSTRIDE                    0x000007ff

#define DBSRCOFFS_YOFFS_MASK            0x00000fff
#define DBSRCOFFS_YOFFS_SHIFT           0
#define DBSRCOFFS_XOFFS_MASK            0x0fff0000
#define DBSRCOFFS_XOFFS_SHIFT           16

#define DBZCNTL_ZFUNCCNTL_MASK          0x0000003f
#define DBZCNTL_ZPIXOUT_LT              0x00000001
#define DBZCNTL_ZBUFOUT_LT              0x00000002
#define DBZCNTL_ZPIXOUT_EQ              0x00000004
#define DBZCNTL_ZBUFOUT_EQ              0x00000008
#define DBZCNTL_ZPIXOUT_GT              0x00000010
#define DBZCNTL_ZBUFOUT_GT              0x00000020

#define DBZBASEADDR_MASK                0x00ffffff

#define DBZOFFS_YOFFS_MASK              0x00000fff
#define DBZOFFS_YOFFS_SHIFT             0
#define DBZOFFS_XOFFS_MASK              0x0fff0000
#define DBZOFFS_XOFFS_SHIFT             16

#define DBZCLIP_YCLIP_MASK              0x000007ff
#define DBZCLIP_YCLIP_SHIFT             0
#define DBZCLIP_XCLIP_MASK              0x07ff0000
#define DBZCLIP_XCLIP_SHIFT             16

#define DBSSBDSBCNTL_DSBSELECT_MASK     0x00000007
#define DBSSBDSBCNTL_DSBSELECT_SHIFT    0
#define DBSSBDSBCNTL_DSBSELECT_SSB      0
#define DBSSBDSBCNTL_DSBSELECT_CONSTANT 1
#define DBSSBDSBCNTL_DSBSELECT_SRC      2

#define DBSSBDSBCNTL_DSBCONST           0x00000004

#define DBCONSTIN_B_MASK                0x000000ff
#define DBCONSTIN_B_SHIFT               0
#define DBCONSTIN_G_MASK                0x0000ff00
#define DBCONSTIN_G_SHIFT               8
#define DBCONSTIN_R_MASK                0x00ff0000
#define DBCONSTIN_R_SHIFT               16

#define DBTXTMULTCNTL_TXTRJUST                  0x00000001
#define DBTXTMULTCNTL_TXTCOEFCMP                0x00000002

#define DBTXTMULTCNTL_TXTCONSTCNTL_MASK         0x0000000c
#define DBTXTMULTCNTL_TXTCONSTCNTL_SHIFT        2
#define DBTXTMULTCNTL_TXTCONSTCNTL_TEXSSB       0
#define DBTXTMULTCNTL_TXTCONSTCNTL_SRCDSB       1

#define DBTXTMULTCNTL_COEFSEL_MASK              0x00000030
#define DBTXTMULTCNTL_COEFSEL_SHIFT             4
#define DBTXTMULTCNTL_COEFSEL_ATI               0
#define DBTXTMULTCNTL_COEFSEL_ASRC              1
#define DBTXTMULTCNTL_COEFSEL_CONSTANT          2
#define DBTXTMULTCNTL_COEFSEL_CSRC              3

#define DBTXTMULTCNTL_INSEL_MASK                0x000000c0
#define DBTXTMULTCNTL_INSEL_SHIFT               6
#define DBTXTMULTCNTL_INSEL_CTI                 0
#define DBTXTMULTCNTL_INSEL_CONSTANT            1
#define DBTXTMULTCNTL_INSEL_COMPSRC             2
#define DBTXTMULTCNTL_INSEL_ATI                 3

#define DBSRCMULTCNTL_SRCRJUST                  0x00000001
#define DBSRCMULTCNTL_SRCCOEFCMP                0x00000002

#define DBSRCMULTCNTL_SRCCONSTCNTL_MASK         0x0000000c
#define DBSRCMULTCNTL_SRCCONSTCNTL_SHIFT        2
#define DBSRCMULTCNTL_SRCCONSTCNTL_TEXSSB       0
#define DBSRCMULTCNTL_SRCCONSTCNTL_SRCDSB       1

#define DBSRCMULTCNTL_COEFSEL_MASK              0x00000030
#define DBSRCMULTCNTL_COEFSEL_SHIFT             4
#define DBSRCMULTCNTL_COEFSEL_ATI               0
#define DBSRCMULTCNTL_COEFSEL_ASRC              1
#define DBSRCMULTCNTL_COEFSEL_CONSTANT          2
#define DBSRCMULTCNTL_COEFSEL_CTI               3

#define DBSRCMULTCNTL_INSEL_MASK                0x000000c0
#define DBSRCMULTCNTL_INSEL_SHIFT               6
#define DBSRCMULTCNTL_INSEL_SRC                 0
#define DBSRCMULTCNTL_INSEL_CONSTANT            1
#define DBSRCMULTCNTL_INSEL_COMPCTI             2
#define DBSRCMULTCNTL_INSEL_TEXALPHA            3

#define DBALUCNTL_FINALDIVIDE_MASK              0x00000007
#define DBALUCNTL_FINALDIVIDE_SHIFT             0

#define DBALUCNTL_ALUOP_MASK                    0x000000f8
#define DBALUCNTL_ALUOP_SHIFT                   5

#define DBDSTACNTL_ADESTSEL_MASK                0x00000003
#define DBDSTACNTL_ADESTSEL_SHIFT               0
#define DBDSTACNTL_ADESTCONSTCNTL_MASK          0x0000000c
#define DBDSTACNTL_ADESTCONSTCNTL_SHIFT         2

#define DBDSTALPHACONST_CONST1_MASK             0x000000ff
#define DBDSTALPHACONST_CONST1_SHIFT            0
#define DBDSTALPHACONST_CONST0_MASK             0x00ff0000
#define DBDSTALPHACONST_CONST0_SHIFT            16

#define DBSSBDSBCNTL_DSBSEL_MASK                0x00000003
#define DBSSBDSBCNTL_DSBSEL_SHIFT               0
#define DBSSBDSBCNTL_DSBCONST_MASK              0x00000004
#define DBSSBDSBCNTL_DSBCONST_SHIFT             2

static inline float int_trunc(float x)
{
	return (float)(int)x;
}

static inline UINT8 extract_exp(float x)
{
	UINT32 u32;
	memcpy(&u32, &x, sizeof(u32));
	return (u32 >> 23) & 0xff;
}

static INT32 ieee754_to_tefix(float f, INT32 bits)
{
	float format = ldexpf(1.0f, bits);
	float tmp1 = format * 2.0f;

	float tmp2 = fmodf(fabsf(f), tmp1);
	float tmp3 = (tmp2 >= format) ? tmp2 : tmp2 + format;

	UINT32 res;
	memcpy(&res, &tmp3, sizeof(res));
	res &= 0x007fffff;

	if (tmp2 >= format || tmp3 == tmp1)
		res |= 0x00800000;

	if (f < 0.0f)
		res = -res;

	return res;
}

static void write_te_reg(UINT32 *reg, UINT32 data, te_reg_wmode mode)
{
	switch (mode)
	{
		case REG_WRITE:
			*reg = data;
			break;
		case REG_SET:
			*reg |= data;
			break;
		case REG_CLEAR:
			*reg &= ~data;
			break;
		default:
			fatalerror("write_te_reg: Bad register write mode");
	}
}

void m2_te_reset(void)
{
	m_state = TE_STOPPED;
}

UINT32 m2_te_read(offs_t offset)
{
	UINT32 unit = (offset >> 11) & 7;
	UINT32 reg = offset & 0x1ff;

	switch (unit)
	{
		case 0:
		{
			if (reg < sizeof(m_gc) / 4)
				return m_gc.m_regs[reg];

			break;
		}
		case 1:
		{
			if (reg < sizeof(m_se) / 4)
				return m_se.m_regs[reg];

			break;
		}
		case 2:
		{
			if (reg < ARRAY_LENGTH(m_es.m_regs))
				return m_es.m_regs[reg];
			if (reg - ARRAY_LENGTH(m_es.m_regs) < ARRAY_LENGTH(m_es.m_buffer_regs))
				return m_es.m_buffer_regs[reg - ARRAY_LENGTH(m_es.m_regs)];

			break;
		}
		case 3:
		{
			if (reg < 0x400 / 4)
			{
				return m_pipram[reg];
			}
			else if ((reg - 0x400 / 4) < sizeof(m_tm) / 4)
			{
				return m_tm.m_regs[reg - 0x400 / 4];
			}

			break;
		}
		case 4:
		{
			if (reg < sizeof(m_db) / 4)
				return m_db.m_regs[reg];

			break;
		}
	}

	return 0;
}

void m2_te_write(offs_t offset, UINT32 data)
{
	UINT32 unit = (offset >> 11) & 7;
	UINT32 reg = offset & 0x1ff;
	te_reg_wmode wmode = (te_reg_wmode)((offset >> 9) & 3);

	switch (unit)
	{
		case 0:
		{
			switch (reg)
			{
				case 0:
				{
					te_master_mode_w(data, wmode);
					return;
				}
				case 0x0c/4:
				{
					te_teicntl_w(data, wmode);
					return;
				}
				case 0x14/4:
				{
					te_tedcntl_w(data, wmode);
					return;
				}
				case 0x24/4:
				{
					write_te_reg(&m_gc.int_status, data, wmode);
					te_update_interrupts();
					return;
				}
				default:
				{
					if (reg < sizeof(m_gc) / 4)
					{
						write_te_reg(&m_gc.m_regs[reg], data, wmode);
						return;
					}
				}
			}
			break;
		}
		case 1:
		{
			if (reg < sizeof(m_se) / 4)
			{
				write_te_reg(&m_se.m_regs[reg], data, wmode);
				return;
			}
			break;
		}
		case 2:
		{
			if (reg < ARRAY_LENGTH(m_es.m_regs))
				write_te_reg(&m_es.m_regs[reg], data, wmode);
			else if (reg - ARRAY_LENGTH(m_es.m_regs) < ARRAY_LENGTH(m_es.m_buffer_regs))
				write_te_reg(&m_es.m_buffer_regs[reg - ARRAY_LENGTH(m_es.m_regs)], data, wmode);
			break;
		}
		case 3:
		{
			if (reg < 0x400 / 4)
			{
				m_pipram[reg] = data;
				return;
			}
			else if ((reg - 0x400 / 4) < sizeof(m_tm) / 4)
			{
				write_te_reg(&m_tm.m_regs[reg - 0x400 / 4], data, wmode);
				return;
			}
			break;
		}
		case 4:
		{
			if (reg < sizeof(m_db) / 4)
			{
				write_te_reg(&m_db.m_regs[reg], data, wmode);
				return;
			}
			break;
		}
	}
}

static void te_set_interrupt(UINT32 mask)
{
	m_gc.int_status |= mask;
	te_update_interrupts();
}

static void te_update_interrupts(void)
{
	bool gen_int = ((m_gc.int_status & m_gc.int_enable) & 0x0000ff00) != 0;

	m2_irq(1U << 27, gen_int);

	m2_irq(1U << 28, (m_gc.int_status >> 8) & 1);
	m2_irq(1U << 29, (m_gc.int_status >> 9) & 1);
	m2_irq(1U << 30, (m_gc.int_status >> 10) & 1);
	m2_irq(1U << 31, (m_gc.int_status >> 11) & 1);
}

static void te_master_mode_w(UINT32 data, te_reg_wmode wmode)
{
	write_te_reg(&m_gc.te_master_mode, data, wmode);

	if (m_gc.te_master_mode & TEMASTER_MODE_RESET)
		m2_te_reset();
}

static void te_teicntl_w(UINT32 data, te_reg_wmode wmode)
{
	UINT32 newreg = 0;

	write_te_reg(&newreg, data, wmode);
	write_te_reg(&m_gc.te_master_mode, data, wmode);

	if (newreg & TEICNTL_STRT)
	{
		m_gc.irp = m_gc.teicntl_data;
		m_state = TE_RUNNING;
		te_execute();
	}
	else if (newreg & TEICNTL_RSTRT)
	{
		m_state = TE_RUNNING;
		te_execute();
	}
}

static void te_tedcntl_w(UINT32 data, te_reg_wmode wmode)
{
	write_te_reg(&m_gc.tedcntl, data, wmode);

	if (m_gc.tedcntl & TEDCNTL_SYNC)
	{
	}
	else if (m_gc.tedcntl & TEDCNTL_PSE)
	{
		m_state = TE_PAUSED;
	}
	else if (m_gc.tedcntl & TEDCNTL_INT)
	{
		te_set_interrupt(INTSTAT_IMMEDIATE);
	}
	else if (m_gc.tedcntl & TEDCNTL_JR)
	{
		m_gc.irp += m_gc.tedcntl_data;
	}
	else if (m_gc.tedcntl & TEDCNTL_JA)
	{
		m_gc.irp = m_gc.tedcntl_data;
	}
	else if (m_gc.tedcntl & TEDCNTL_TLD)
	{
		te_load_texture();
	}
}

static void te_add_vertex(const se_vtx *vtx, UINT32 flags)
{
	UINT32 tsort;
	UINT32 vcnt;

	if (flags & VTX_FLAG_NEW)
	{
		tsort = 0;
		vcnt = 0;
	}
	else
	{
		tsort = m_se.vertex_state & VERTEXSTATE_TSORT_MASK;
		vcnt = (m_se.vertex_state & VERTEXSTATE_VCNT_MASK) >> VERTEXSTATE_VCNT_SHIFT;
	}

	if (vcnt < 3)
	{
		m_se.vertices[vcnt++] = (*vtx);

		if (vcnt == 3)
			tsort = VERTEXSTATE_TSORT_OMN;
	}
	else
	{
		if (flags & VTX_FLAG_RM)
		{
			switch (tsort)
			{
				case VERTEXSTATE_TSORT_OMN: m_se.vertices[1] = (*vtx);  tsort = VERTEXSTATE_TSORT_ONM; break;
				case VERTEXSTATE_TSORT_MNO: m_se.vertices[0] = (*vtx);  tsort = VERTEXSTATE_TSORT_NMO; break;
				case VERTEXSTATE_TSORT_ONM: m_se.vertices[2] = (*vtx);  tsort = VERTEXSTATE_TSORT_OMN; break;
				case VERTEXSTATE_TSORT_NOM: m_se.vertices[2] = (*vtx);  tsort = VERTEXSTATE_TSORT_MON; break;
				case VERTEXSTATE_TSORT_MON: m_se.vertices[0] = (*vtx);  tsort = VERTEXSTATE_TSORT_NOM; break;
				case VERTEXSTATE_TSORT_NMO: m_se.vertices[1] = (*vtx);  tsort = VERTEXSTATE_TSORT_MNO; break;
			}
		}
		else
		{
			switch (tsort)
			{
				case VERTEXSTATE_TSORT_OMN: m_se.vertices[0] = (*vtx);  tsort = VERTEXSTATE_TSORT_NOM; break;
				case VERTEXSTATE_TSORT_MNO: m_se.vertices[2] = (*vtx);  tsort = VERTEXSTATE_TSORT_OMN; break;
				case VERTEXSTATE_TSORT_ONM: m_se.vertices[0] = (*vtx);  tsort = VERTEXSTATE_TSORT_NMO; break;
				case VERTEXSTATE_TSORT_NOM: m_se.vertices[1] = (*vtx);  tsort = VERTEXSTATE_TSORT_MNO; break;
				case VERTEXSTATE_TSORT_MON: m_se.vertices[1] = (*vtx);  tsort = VERTEXSTATE_TSORT_ONM; break;
				case VERTEXSTATE_TSORT_NMO: m_se.vertices[2] = (*vtx);  tsort = VERTEXSTATE_TSORT_MON; break;
			}
		}
	}

	m_se.vertex_state = (vcnt << VERTEXSTATE_VCNT_SHIFT) | tsort;

	if (vcnt == 3)
	{
		te_setup_triangle(flags);
	}
}

static void te_calculate_slope(const slope_params *sp, float q1, float q2, float q3, float *slope_out, float *ddx_out)
{
	float detx = q1 * sp->y23 + q2 * sp->y31 + q3 * sp->y12;
	float dety = q1 * sp->x23 + q2 * sp->x31 + q3 * sp->x12;
	float detxy = sp->xstep_long * detx - dety;

	(*slope_out) = detxy * sp->iAria;
	(*ddx_out) = detx * sp->iAria;
}

static void te_setup_triangle(UINT32 flags)
{
	se_vtx va = m_se.vertices[0];
	se_vtx vb = m_se.vertices[1];
	se_vtx vc = m_se.vertices[2];

	va.x = int_trunc(va.x);
	vb.x = int_trunc(vb.x);
	vc.x = int_trunc(vc.x);

	va.y = int_trunc(va.y);
	vb.y = int_trunc(vb.y);
	vc.y = int_trunc(vc.y);

	if (flags & VTX_FLAG_SHAD)
	{
		va.r = int_trunc(va.r * 255.0f);
		va.g = int_trunc(va.g * 255.0f);
		va.b = int_trunc(va.b * 255.0f);
		va.a = int_trunc(va.a * 255.0f);

		vb.r = int_trunc(vb.r * 255.0f);
		vb.g = int_trunc(vb.g * 255.0f);
		vb.b = int_trunc(vb.b * 255.0f);
		vb.a = int_trunc(vb.a * 255.0f);

		vc.r = int_trunc(vc.r * 255.0f);
		vc.g = int_trunc(vc.g * 255.0f);
		vc.b = int_trunc(vc.b * 255.0f);
		vc.a = int_trunc(vc.a * 255.0f);
	}

	UINT32 a = 0;

	if ((va.y - vb.y) < 0.0f) a |= 4;
	if ((vb.y - vc.y) < 0.0f) a |= 2;
	if ((vc.y - va.y) < 0.0f) a |= 1;

	const se_vtx *v1 = NULL;
	const se_vtx *v2 = NULL;
	const se_vtx *v3 = NULL;

	switch (a)
	{
		case 1: v1 = &vc; v2 = &vb; v3 = &va; break;
		case 2: v1 = &vb; v2 = &va; v3 = &vc; break;
		case 3: v1 = &vb; v2 = &vc; v3 = &va; break;
		case 4: v1 = &va; v2 = &vc; v3 = &vb; break;
		case 5: v1 = &vc; v2 = &va; v3 = &vb; break;
		case 6: v1 = &va; v2 = &vb; v3 = &vc; break;
		default:
			return;
	}

	UINT32 wrange = 0;

	if (flags & VTX_FLAG_PRSP)
	{
		const UINT8 exp_zero = 0x7f;
		const UINT8 wexp0 = exp_zero - 3;
		const UINT8 wexp1 = exp_zero - 6;
		const UINT8 wexp2 = exp_zero - 9;

		UINT8 w1exp = extract_exp(v1->w);
		UINT8 w2exp = extract_exp(v2->w);
		UINT8 w3exp = extract_exp(v3->w);

		if (w1exp >= wexp0 || w2exp >= wexp0 || w3exp >= wexp0)
			wrange = 0;
		else if (w1exp >= wexp1 || w2exp >= wexp1 || w3exp >= wexp1)
			wrange = 1;
		else if (w1exp >= wexp2 || w2exp >= wexp2 || w3exp >= wexp2)
			wrange = 2;
		else
			wrange = 3;
	}

	float x12 = v1->x - v2->x;
	float x23 = v2->x - v3->x;
	float x31 = v3->x - v1->x;
	float y12 = v1->y - v2->y;
	float y23 = v2->y - v3->y;
	float y31 = v3->y - v1->y;

	float aria = v1->x * y23 + v2->x * y31 + v3->x * y12;

	if (aria == 0.0f)
		return;

	float iaria = 1.0f / aria;

	bool scan_lr = aria > 0.0f;

	float abs_y12 = fabs(y12);
	float abs_y23 = fabs(y23);
	float abs_y31 = fabs(y31);

	float xslope_0 = (abs_y12 > 0.0f) ? (-x12 / abs_y12) : 0.0f;
	float xslope_1 = (abs_y23 > 0.0f) ? (-x23 / abs_y23) : 0.0f;
	float xslope_long = (abs_y31 > 0.0f) ? (x31 / abs_y31) : 0.0f;

	float xstep_long;
	float xstep_0;
	float xstep_1;

	if (scan_lr)
	{
		xstep_long = floor(xslope_long);
		xstep_0 = ceil(xslope_0);
		xstep_1 = ceil(xslope_1);
	}
	else
	{
		xstep_long = ceil(xslope_long);
		xstep_0 = floor(xslope_0);
		xstep_1 = floor(xslope_1);
	}

	float xystep_0 = x12 - xstep_0 * y12;
	float xystep_1 = x23 - xstep_1 * y23;
	float xystep_long = x31 - xstep_long * y31;

	float dy_0 = -y12;
	float dy_1 = -y23;
	float dy_long = -y31;

	slope_params sparams = { y23, y31, y12, x23, x31, x12, xstep_long, iaria };

	float r_slope = 0.0f, r_ddx = 0.0f;
	float g_slope = 0.0f, g_ddx = 0.0f;
	float b_slope = 0.0f, b_ddx = 0.0f;
	float a_slope = 0.0f, a_ddx = 0.0f;
	float w_slope = 0.0f, w_ddx = 0.0f;
	float uw_slope = 0.0f, uw_ddx = 0.0f;
	float vw_slope = 0.0f, vw_ddx = 0.0f;

	if (flags & VTX_FLAG_SHAD)
	{
		te_calculate_slope(&sparams, v1->r, v2->r, v3->r, &r_slope, &r_ddx);
		te_calculate_slope(&sparams, v1->g, v2->g, v3->g, &g_slope, &g_ddx);
		te_calculate_slope(&sparams, v1->b, v2->b, v3->b, &b_slope, &b_ddx);
		te_calculate_slope(&sparams, v1->a, v2->a, v3->a, &a_slope, &a_ddx);
	}

	if (flags & VTX_FLAG_TEXT)
	{
		te_calculate_slope(&sparams, v1->uw, v2->uw, v3->uw, &uw_slope, &uw_ddx);
		te_calculate_slope(&sparams, v1->vw, v2->vw, v3->vw, &vw_slope, &vw_ddx);
	}

	if (flags & VTX_FLAG_PRSP)
	{
		te_calculate_slope(&sparams, v1->w, v2->w, v3->w, &w_slope, &w_ddx);
	}

	const UINT32 textbits = text_bits[wrange];
	const UINT32 depthbits = depth_bits[wrange];

	m_es.x1 = ieee754_to_tefix(v1->x, xy_bits) >> xy_rshift;
	m_es.y1 = ieee754_to_tefix(v1->y, xy_bits) >> xy_rshift;
	m_es.x2 = ieee754_to_tefix(v2->x, xy_bits) >> xy_rshift;
	m_es.y2 = ieee754_to_tefix(v2->y, xy_bits) >> xy_rshift;
	m_es.y3 = ieee754_to_tefix(v3->y, xy_bits) >> xy_rshift;

	m_es.xstep_0 = ieee754_to_tefix(xstep_0, xy_bits) >> xy_rshift;
	m_es.xstep_1 = ieee754_to_tefix(xstep_1, xy_bits) >> xy_rshift;
	m_es.xstep_long = ieee754_to_tefix(xstep_long, xy_bits) >> xy_rshift;
	m_es.xystep_0 = ieee754_to_tefix(xystep_0, xy_bits) >> xy_rshift;
	m_es.xystep_1 = ieee754_to_tefix(xystep_1, xy_bits) >> xy_rshift;
	m_es.xystep_long = ieee754_to_tefix(xystep_long, xy_bits) >> xy_rshift;
	m_es.dy_0 = ieee754_to_tefix(dy_0, xy_bits) >> xy_rshift;
	m_es.dy_1 = ieee754_to_tefix(dy_1, xy_bits) >> xy_rshift;
	m_es.dy_long = ieee754_to_tefix(dy_long, xy_bits) >> xy_rshift;

	m_es.r1 = ieee754_to_tefix(v1->r, color_bits) >> color_rshift;
	m_es.g1 = ieee754_to_tefix(v1->g, color_bits) >> color_rshift;
	m_es.b1 = ieee754_to_tefix(v1->b, color_bits) >> color_rshift;
	m_es.a1 = ieee754_to_tefix(v1->a, color_bits) >> color_rshift;
	m_es.ddx_r = ieee754_to_tefix(r_ddx, color_bits) >> color_rshift;
	m_es.ddx_g = ieee754_to_tefix(g_ddx, color_bits) >> color_rshift;
	m_es.ddx_b = ieee754_to_tefix(b_ddx, color_bits) >> color_rshift;
	m_es.ddx_a = ieee754_to_tefix(a_ddx, color_bits) >> color_rshift;
	m_es.slope_r = ieee754_to_tefix(r_slope, color_bits) >> color_rshift;
	m_es.slope_g = ieee754_to_tefix(g_slope, color_bits) >> color_rshift;
	m_es.slope_b = ieee754_to_tefix(b_slope, color_bits) >> color_rshift;
	m_es.slope_a = ieee754_to_tefix(a_slope, color_bits) >> color_rshift;

	m_es.uw1 = ieee754_to_tefix(v1->uw, textbits);
	m_es.vw1 = ieee754_to_tefix(v1->vw, textbits);
	m_es.w1 = ieee754_to_tefix(v1->w, depthbits);
	m_es.ddx_uw = ieee754_to_tefix(uw_ddx, textbits);
	m_es.ddx_vw = ieee754_to_tefix(vw_ddx, textbits);
	m_es.ddx_w = ieee754_to_tefix(w_ddx, depthbits);
	m_es.slope_uw = ieee754_to_tefix(uw_slope, textbits);
	m_es.slope_vw = ieee754_to_tefix(vw_slope, textbits);
	m_es.slope_w = ieee754_to_tefix(w_slope, depthbits);

	m_es.r2l = !scan_lr;

	te_walk_edges(wrange);
}

static inline bool ins(bool lr, INT32 v)
{
	return lr ? v <= 0 : v >= 0;
}

static inline bool ein(bool lr, INT32 v)
{
	return lr ? v < 0 : v > 0;
}

static const UINT8 m_nr_rom[128] =
{
	127, 125, 123, 121, 119, 118,
	116, 114, 112, 110, 109, 107,
	105, 104, 102, 100, 99, 97,
	96, 94, 93, 91, 90, 88,
	87, 86, 84, 83, 81, 80,
	79, 78, 76, 75, 74, 73,
	71, 70, 69, 68, 67, 65,
	64, 63, 62, 61, 60, 59,
	58, 57, 56, 55, 54, 53,
	52, 51, 50, 49, 48, 47,
	46, 45, 44, 43, 42, 41,
	40, 40, 39, 38, 37, 36,
	35, 35, 34, 33, 32, 31,
	31, 30, 29, 28, 28, 27,
	26, 25, 25, 24, 23, 23,
	22, 21, 21,20, 19, 19,
	18, 17, 17, 16, 15, 15,
	14, 13, 13, 12, 12, 11,
	11, 10, 9, 9, 8, 8,
	7, 7, 6, 5, 5, 4,
	4, 3, 3, 2, 2, 1,
	1, 0
};

static UINT32 nr_invert(UINT32 num, UINT32 *shift_amount)
{
	UINT32 original;
	UINT32 first_guess;
	UINT32 m;
	UINT32 shfnum;

	if (num == 0)
	{
		*shift_amount = 0;
		return 0;
	}

	*shift_amount = -1;
	while ((num << ++*shift_amount) < 0x800000);

	shfnum = num << *shift_amount;
	original = shfnum >> 8;
	first_guess = m_nr_rom[(shfnum >> 16) & ((1 << 7) - 1)];
	first_guess = first_guess +  128;
	m = first_guess * original;
	m = (1 << 24) - m;
	m = m >> 3;
	m = first_guess * m;
	m = m >> 8;

	return m;
}

static void te_walk_edges(UINT32 wrange)
{
	bool dsp_off = m_es.es_cntl & ESCNTL_DSPOFF;
	bool du_scan = m_es.es_cntl & ESCNTL_DUSCAN;
	bool omit_right = true;
	bool scan_lr = m_es.r2l == false;

	UINT32 cf_long = 0;
	UINT32 cf_short = 0;
	UINT32 xystep;
	UINT32 xstep;
	UINT32 dy;
	UINT32 xs;
	UINT32 xe;

	INT32 r, g, b, a;
	INT32 uw, vw;
	INT32 w;

	UINT32 y = m_es.y1;

	if (m_es.y1 == m_es.y2)
	{
		xystep = m_es.xystep_1;
		xstep = m_es.xstep_1;
		dy = m_es.dy_1;

		if (scan_lr ^ (m_es.x1 < m_es.x2))
		{
			fatalerror("m2_te_device::walk_edges: SPECIAL CASE: WHAT DO?");
			r = m_es.r1;
			g = m_es.g1;
			b = m_es.b1;
			a = m_es.a1;
			uw = m_es.uw1;
			vw = m_es.vw1;
			w = m_es.w1;

			xs = m_es.x2;
			xe = m_es.x1;
		}
		else
		{
			r = m_es.r1;
			g = m_es.g1;
			b = m_es.b1;
			a = m_es.a1;
			uw = m_es.uw1;
			vw = m_es.vw1;
			w = m_es.w1;

			xs = m_es.x1;
			xe = m_es.x2;
		}
	}
	else
	{
		xystep = m_es.xystep_0;
		xstep = m_es.xstep_0;
		dy = m_es.dy_0;
		r = m_es.r1;
		g = m_es.g1;
		b = m_es.b1;
		a = m_es.a1;
		uw = m_es.uw1;
		vw = m_es.vw1;
		w = m_es.w1;

		xs = m_es.x1;
		xe = xs;
	}

	do
	{
		te_walk_span(wrange, omit_right, y, xs, xe, r, g, b, a, uw, vw, w);

		omit_right = false;
		cf_short += xystep;
		cf_long += m_es.xystep_long;

		bool step_back = false;

		if (scan_lr)
		{
			xs += m_es.xstep_long;
			xe += xstep;

			if (!ins(scan_lr, cf_long))
			{
				step_back = true;
				cf_long += m_es.dy_long;
				xs += xy_one;
			}

			if (dsp_off ? !ins(scan_lr, cf_short) : !ein(scan_lr, cf_short))
			{
				cf_short -= dy;
				xe -= xy_one;
			}
		}
		else
		{
			xe += xstep;
			xs += m_es.xstep_long;

			if (!ins(scan_lr, cf_short))
			{
				cf_short += dy;
				xe += xy_one;
			}

			if (dsp_off ? !ins(scan_lr, cf_long) : !ein(scan_lr, cf_long))
			{
				step_back = true;
				cf_long -= m_es.dy_long;
				xs -= xy_one;
			}
		}

		if (!(m_gc.te_master_mode & TEMASTER_MODE_DSHADE))
		{
			if (scan_lr)
			{
				r += step_back ? m_es.slope_r + m_es.ddx_r : m_es.slope_r;
				g += step_back ? m_es.slope_g + m_es.ddx_g : m_es.slope_g;
				b += step_back ? m_es.slope_b + m_es.ddx_b : m_es.slope_b;
				a += step_back ? m_es.slope_a + m_es.ddx_a : m_es.slope_a;
			}
			else
			{
				r += step_back ? m_es.slope_r - m_es.ddx_r : m_es.slope_r;
				g += step_back ? m_es.slope_g - m_es.ddx_g : m_es.slope_g;
				b += step_back ? m_es.slope_b - m_es.ddx_b : m_es.slope_b;
				a += step_back ? m_es.slope_a - m_es.ddx_a : m_es.slope_a;
			}

			r = te_clamp(r, 0, 0x0007ffff);
			g = te_clamp(g, 0, 0x0007ffff);
			b = te_clamp(b, 0, 0x0007ffff);
			a = te_clamp(a, 0, 0x0007ffff);
		}

		if (!(m_es.es_cntl & TEMASTER_MODE_DTEXT))
		{
			if (scan_lr)
			{
				uw += step_back ? m_es.slope_uw + m_es.ddx_uw : m_es.slope_uw;
				vw += step_back ? m_es.slope_vw + m_es.ddx_vw : m_es.slope_vw;
			}
			else
			{
				uw += step_back ? m_es.slope_uw - m_es.ddx_uw : m_es.slope_uw;
				vw += step_back ? m_es.slope_vw - m_es.ddx_vw : m_es.slope_vw;
			}

			uw = te_clamp(uw, 0, 0x007fffff);
			vw = te_clamp(vw, 0, 0x007fffff);
		}

		if (!(m_es.es_cntl & ESCNTL_PERSPECTIVEOFF))
		{
			if (scan_lr)
				w += step_back ? m_es.slope_w + m_es.ddx_w : m_es.slope_w;
			else
				w += step_back ? m_es.slope_w - m_es.ddx_w : m_es.slope_w;

			w = te_clamp(w, 0, 0x007fffff);
		}

		UINT32 next_y = y + (du_scan ? -xy_one : xy_one);

		if (next_y == m_es.y2)
		{
			cf_short = 0;
			xystep = m_es.xystep_1;
			xstep = m_es.xstep_1;
			dy = m_es.dy_1;
			xe = m_es.x2;

			if (scan_lr)
				omit_right = true;
		}

		y = next_y;
	} while (y != m_es.y3);
}

static void te_texcoord_gen(UINT32 wrange, UINT32 uw, UINT32 vw, UINT32 w,
								UINT32 * uo, UINT32 * vo, UINT32 * wo)
{
	if (!(m_es.es_cntl & ESCNTL_PERSPECTIVEOFF))
	{
		UINT32 wshift;

		UINT32 winv = nr_invert(w, &wshift);

		UINT32 uwshift = 0;
		UINT32 vwshift = 0;

		while ((uw < 0x00400000) && uwshift < 7)
		{
			uw <<= 1;
			++uwshift;
		}

		while ((vw < 0x00400000) && vwshift < 7)
		{
			vw <<= 1;
			++vwshift;
		}

		UINT32 normalized_uw = uw >> 7;
		UINT32 normalized_vw = vw >> 7;

		UINT64 ur = (UINT64)normalized_uw * winv;
		UINT64 vr = (UINT64)normalized_vw * winv;

		if (uwshift > wshift)
			ur >>= (uwshift - wshift);
		else
			ur <<= (wshift - uwshift);

		if (vwshift > wshift)
			vr >>= (vwshift - wshift);
		else
			vr <<= (wshift - vwshift);

		(*uo) = ur >> 20;
		(*vo) = vr >> 20;
	}
	else
	{
		(*uo) = uw >> (7 + (10 - text_bits[wrange]));
		(*vo) = vw >> (7 + (10 - text_bits[wrange]));
	}

	(*uo) = (*uo) >> 2;
	(*vo) = (*vo) >> 2;
}

static UINT32 te_lod_calc(UINT32 u0, UINT32 v0, UINT32 u1, UINT32 v1)
{
	return 0;
}

static UINT32 te_get_tram_bitdepth(void)
{
	const UINT32 tex_type = m_tm.tex_exptype;

	UINT32 bitdepth = 0;

	if (tex_type & TXTEXPFORM_COLORON)
		bitdepth += (tex_type & TXTEXPFORM_CDEPTH_MASK) >> TXTEXPFORM_CDEPTH_SHIFT;
	if (tex_type & TXTEXPFORM_ALPHAON)
		bitdepth += (tex_type & TXTEXPFORM_ADEPTH_MASK) >> TXTEXPFORM_ADEPTH_SHIFT;
	if (tex_type & TXTEXPFORM_SSBON)
		++bitdepth;

	return bitdepth;
}

static void te_get_texture_color(UINT32 u, UINT32 v, UINT32 lod,
									UINT32 * r, UINT32 * g, UINT32 * b, UINT32 * a, UINT32 * s)
{
	UINT32 texaddr;
	UINT32 texbit;
	UINT32 texdepth = te_get_tram_bitdepth();

	UINT32 filter = (m_tm.tex_addr_cntl >> TXTADDRCNTL_R12FILTERSEL_SHIFT) & TXTADDRCNTL_FILTERSEL_MASK;

	switch (filter)
	{
		case TXTADDRCNTL_FILTERSEL_POINT:
		case TXTADDRCNTL_FILTERSEL_LINEAR:
		{
			te_addr_calc(u, v, lod, &texaddr, &texbit, &texdepth);
			te_get_texel(texaddr, texbit, texdepth, r, g, b, a, s);
			break;
		}

		case TXTADDRCNTL_FILTERSEL_BILINEAR:
		case TXTADDRCNTL_FILTERSEL_QUASITRI:
		{
			UINT32 r0, g0, b0, a0, s0;
			UINT32 r1, g1, b1, a1, s1;
			UINT32 r2, g2, b2, a2, s2;
			UINT32 r3, g3, b3, a3, s3;

			te_addr_calc(u, v, lod, &texaddr, &texbit, &texdepth);
			te_get_texel(texaddr, texbit, texdepth, &r0, &g0, &b0, &a0, &s0);
			te_addr_calc(u + 0x10, v, lod, &texaddr, &texbit, &texdepth);
			te_get_texel(texaddr, texbit, texdepth, &r1, &g1, &b1, &a1, &s1);
			te_addr_calc(u, v + 0x10, lod, &texaddr, &texbit, &texdepth);
			te_get_texel(texaddr, texbit, texdepth, &r2, &g2, &b2, &a2, &s2);
			te_addr_calc(u + 0x10, v + 0x10, lod, &texaddr, &texbit, &texdepth);
			te_get_texel(texaddr, texbit, texdepth, &r3, &g3, &b3, &a3, &s3);

			UINT32 ufrac = u & 0xf;
			UINT32 vfrac = v & 0xf;
			UINT32 om_ufrac = 0x10 - ufrac;
			UINT32 om_vfrac = 0x10 - vfrac;

			s0 = s0 * 0xff;
			s1 = s1 * 0xff;
			s2 = s2 * 0xff;
			s3 = s3 * 0xff;

			UINT32 rl = r0 * (om_ufrac) + r1 * ufrac;
			UINT32 gl = g0 * (om_ufrac) + g1 * ufrac;
			UINT32 bl = b0 * (om_ufrac) + b1 * ufrac;
			UINT32 al = a0 * (om_ufrac) + a1 * ufrac;
			UINT32 sl = s0 * (om_ufrac) + s1 * ufrac;

			UINT32 ru = r2 * (om_ufrac) + r3 * ufrac;
			UINT32 gu = g2 * (om_ufrac) + g3 * ufrac;
			UINT32 bu = b2 * (om_ufrac) + b3 * ufrac;
			UINT32 au = a2 * (om_ufrac) + a3 * ufrac;
			UINT32 su = s2 * (om_ufrac) + s3 * ufrac;

			(*r) = (rl * om_vfrac + ru * vfrac) >> 8;
			(*g) = (gl * om_vfrac + gu * vfrac) >> 8;
			(*b) = (bl * om_vfrac + bu * vfrac) >> 8;
			(*a) = (al * om_vfrac + au * vfrac) >> 8;

			(*s) = ((sl * om_vfrac + su * vfrac) >> 10) ? 1 : 0;
			break;
		}
	}
}

static void te_addr_calc(UINT32 u, UINT32 v, UINT32 lod,
							UINT32 * texaddr, UINT32 * texbit, UINT32 * tdepth)
{
	UINT32 u_mask = (m_tm.uv_mask & TXTUVMASK_UMASK_MASK) >> TXTUVMASK_UMASK_SHIFT;
	UINT32 v_mask = (m_tm.uv_mask & TXTUVMASK_VMASK_MASK) >> TXTUVMASK_VMASK_SHIFT;
	UINT32 u_max = (m_tm.uv_max & TXTUVMAX_UMAX_MASK) >> TXTUVMAX_UMAX_SHIFT;
	UINT32 v_max = (m_tm.uv_max & TXTUVMAX_VMAX_MASK) >> TXTUVMAX_VMAX_SHIFT;

	UINT32 u0 = u >> 4;
	UINT32 v0 = v >> 4;

	u0 = u0 & u_mask;
	v0 = v0 & v_mask;

	u_max &= u_mask;
	v_max &= v_mask;

	u0 = MIN(u0, u_max);
	v0 = MIN(v0, v_max);

	UINT32 lodmax = m_tm.tex_addr_cntl & TXTADDRCNTL_LODMAX_MASK;
	UINT32 lod_shift = (lodmax - lod) & 3;

	UINT32 voffset = (v0 * (u_max + 1)) << lod_shift;
	UINT32 offset = (voffset + u0) >> lod_shift;

	offset = offset * (*tdepth);

	UINT32 base_addr = 0;

	switch (lod)
	{
		case 0: base_addr = m_tm.tex_lod_base0; break;
		case 1: base_addr = m_tm.tex_lod_base1; break;
		case 2: base_addr = m_tm.tex_lod_base2; break;
		case 3: base_addr = m_tm.tex_lod_base3; break;
	}

	UINT32 adsum = (base_addr << 3) + offset;

	(*texaddr) = adsum / 32;
	(*texbit) = adsum & 31;
}

static void te_get_texel(UINT32 tex_addr, UINT32 tex_bit, UINT32 tdepth,
							UINT32 * r, UINT32 * g, UINT32 * b, UINT32 * a, UINT32 * ssb)
{
	const UINT32 tex_type = m_tm.tex_exptype;

	UINT8 rtex, gtex, btex, atex;
	UINT8 ssbtex;

	UINT8 rpip, gpip, bpip, apip;
	UINT8 ssbpip;

	UINT32 tram_data = m_tram[tex_addr];

	UINT32 texel = tram_data >> (tex_bit & ~7);

	texel >>= (tex_bit ^ (8 - tdepth)) & 7;

	UINT32 c_depth = (tex_type & TXTEXPFORM_CDEPTH_MASK) >> TXTEXPFORM_CDEPTH_SHIFT;
	UINT32 a_depth = (tex_type & TXTEXPFORM_ADEPTH_MASK) >> TXTEXPFORM_ADEPTH_SHIFT;
	UINT32 i_depth = c_depth;

	if (tex_type & TXTEXPFORM_LITERAL)
	{
		if (tex_type & TXTEXPFORM_COLORON)
		{
			UINT32 mask = (1 << c_depth) - 1;

			btex = texel & mask;
			texel >>= c_depth;
			gtex = texel & mask;
			texel >>= c_depth;
			rtex = texel & mask;
			texel >>= c_depth;
		}
		else
		{
			rtex = gtex = btex = 0;
		}
	}
	else
	{
		rtex = gtex = btex = 0;
	}

	if (tex_type & TXTEXPFORM_INDEXON)
	{
		UINT32 pipidx = texel & ((1 << i_depth) - 1);
		UINT32 pipaddr = (m_tm.tex_pip_cntl & TXTPIPCNTL_INDEX_OFFSET) + pipidx;

		UINT32 pipval = m_pipram[pipaddr & 0xff];

		bpip = pipval & 0xff;
		gpip = (pipval >> 8) & 0xff;
		rpip = (pipval >> 16) & 0xff;
		apip = (pipval >> 24) & 0x7f;
		ssbpip = pipval >> 31;

		texel >>= i_depth;
	}
	else
	{
		rpip = gpip = bpip = 0;
		apip = ssbpip = 0;
	}

	if (tex_type & TXTEXPFORM_ALPHAON)
	{
		UINT32 mask = (1 << a_depth) - 1;

		atex = texel & mask;

		if (a_depth == 4)
			atex = (atex << 3) | (atex >> 1);

		texel >>= a_depth;
	}
	else
	{
		atex = 0;
	}

	if (tex_type & TXTEXPFORM_SSBON)
		ssbtex = texel & 1;
	else
		ssbtex = 0;

	switch ((m_tm.tex_pip_cntl & TXTPIPCNTL_SSBSEL_MASK) >> TXTPIPCNTL_SSBSEL_SHIFT)
	{
		case TXTPIPCNTL_SEL_CONSTANT:
		{
			UINT32 cnst = ssbtex ? m_tm.tex_srcconst1 : m_tm.tex_srcconst0;
			(*ssb) = cnst >> 31;
			break;
		}
		case TXTPIPCNTL_SEL_TRAM:
		{
			(*ssb) = ssbtex;
			break;
		}
		case TXTPIPCNTL_SEL_PIP:
		{
			(*ssb) = ssbpip;
			break;
		}
	}

	switch ((m_tm.tex_pip_cntl & TXTPIPCNTL_COLORSEL_MASK) >> TXTPIPCNTL_COLORSEL_SHIFT)
	{
		case TXTPIPCNTL_SEL_CONSTANT:
		{
			UINT32 cnst = (*ssb) ? m_tm.tex_srcconst1 : m_tm.tex_srcconst0;
			(*r) = (cnst >> 16) & 0xff;
			(*g) = (cnst >>  8) & 0xff;
			(*b) = (cnst >>  0) & 0xff;
			break;
		}
		case TXTPIPCNTL_SEL_TRAM:
		{
			(*r) = rtex;
			(*g) = gtex;
			(*b) = btex;
			break;
		}
		case TXTPIPCNTL_SEL_PIP:
		{
			(*r) = rpip;
			(*g) = gpip;
			(*b) = bpip;
			break;
		}
	}

	switch ((m_tm.tex_pip_cntl & TXTPIPCNTL_ALPHASEL_MASK) >> TXTPIPCNTL_ALPHASEL_SHIFT)
	{
		case TXTPIPCNTL_SEL_CONSTANT:
		{
			UINT32 cnst = (*ssb) ? m_tm.tex_srcconst1 : m_tm.tex_srcconst0;
			(*a) = (cnst >> 24) & 0x7f;
			break;
		}
		case TXTPIPCNTL_SEL_TRAM:
		{
			(*a) = atex;
			break;
		}
		case TXTPIPCNTL_SEL_PIP:
		{
			(*a) = apip;
			break;
		}
	}

	(*a) = ((*a) << 1) | ((*a) >> 6);
}

static inline UINT8 lerp(UINT8 a, UINT8 b, UINT8 t)
{
	if (t == 255)
		return b;
	if (t == 0)
		return a;

	return (((255 - t) * a) >> 8) + ((t * b) >> 8);
}

static inline UINT8 multiply(UINT8 a, UINT8 b)
{
	if (b == 255)
		return a;
	if (a == 255)
		return b;
	else
		return (a * b) >> 8;
}

static void te_texture_blend(
	UINT32 ri, UINT32 gi, UINT32 bi, UINT32 ai,
	UINT32 rt, UINT32 gt, UINT32 bt, UINT32 at, UINT32 ssbt,
	UINT32 *ro, UINT32 *go, UINT32 *bo, UINT32 *ao, UINT32 *ssbo)
{
	UINT32 rbl = 0, gbl = 0, bbl = 0, abl = 0;

	{
		UINT32 ar, ab, ag, aa;
		UINT32 br, bb, bg, ba;
		UINT32 tr, tb, tg;

		te_select_lerp( (m_tm.tex_tab_cntl & TXTTABCNTL_C_ASEL_MASK) >> TXTTABCNTL_C_ASEL_SHIFT,
					ri, gi, bi, ai,
					rt, gt, bt, at, ssbt,
					&ar, &ag, &ab );

		te_select_lerp( (m_tm.tex_tab_cntl & TXTTABCNTL_C_BSEL_MASK) >> TXTTABCNTL_C_BSEL_SHIFT,
					ri, gi, bi, ai,
					rt, gt, bt, at, ssbt,
					&br, &bg, &bb );

		te_select_lerp( (m_tm.tex_tab_cntl & TXTTABCNTL_C_TSEL_MASK) >> TXTTABCNTL_C_TSEL_SHIFT,
					ri, gi, bi, ai,
					rt, gt, bt, at, ssbt,
					&tr, &tg, &tb );

		if (((m_tm.tex_tab_cntl & TXTTABCNTL_BLENDOP_MASK) >> TXTTABCNTL_BLENDOP_SHIFT) == TXTTABCNTL_BLENDOP_LERP)
		{
			rbl = lerp(ar, br, tr);
			gbl = lerp(ag, bg, tg);
			bbl = lerp(ab, bb, tb);
		}
		else
		{
			te_select_mul( (m_tm.tex_tab_cntl & TXTTABCNTL_A_ASEL_MASK) >> TXTTABCNTL_A_ASEL_SHIFT,
						ai, at, ssbt,
						&aa);

			te_select_mul( (m_tm.tex_tab_cntl & TXTTABCNTL_A_BSEL_MASK) >> TXTTABCNTL_A_BSEL_SHIFT,
						ai, at, ssbt,
						&ba);

			rbl = multiply(ar, br);
			gbl = multiply(ag, bg);
			bbl = multiply(ab, bb);
			abl = multiply(aa, ba);
		}
	}

	switch ((m_tm.tex_tab_cntl & TXTTABCNTL_C_OSEL_MASK) >> TXTTABCNTL_C_OSEL_SHIFT)
	{
		case TXTTABCNTL_CO_SEL_CITER:
		{
			(*ro) = ri;
			(*go) = gi;
			(*bo) = bi;
			break;
		}
		case TXTTABCNTL_CO_SEL_CT:
		{
			(*ro) = rt;
			(*go) = gt;
			(*bo) = bt;
			break;
		}
		case TXTTABCNTL_CO_SEL_BLEND:
		{
			(*ro) = rbl;
			(*go) = gbl;
			(*bo) = bbl;
			break;
		}
		case TXTTABCNTL_CO_SEL_RESERVED:
		{
			(*ro) = (*go) = (*bo) = 0;
			break;
		}
	}

	switch ((m_tm.tex_tab_cntl & TXTTABCNTL_A_OSEL_MASK) >> TXTTABCNTL_A_OSEL_SHIFT)
	{
		case TXTTABCNTL_AO_SEL_AITER:
		{
			(*ao) = ai;
			break;
		}
		case TXTTABCNTL_AO_SEL_AT:
		{
			(*ao) = at;
			break;
		}
		case TXTTABCNTL_AO_SEL_BLEND:
		{
			(*ao) = abl;
			break;
		}
		case TXTTABCNTL_AO_SEL_RESERVED:
		{
			(*ao) = 0;
			break;
		}
	}
}

static void te_select_lerp( UINT32 sel,
								UINT32 ri, UINT32 gi, UINT32 bi, UINT32 ai,
								UINT32 rt, UINT32 gt, UINT32 bt, UINT32 at, UINT32 ssbt,
								UINT32 * ar, UINT32 * ag, UINT32 * ab )
{
	switch (sel)
	{
		case TXTTABCNTL_C_ABTSEL_AITER:
		{
			(*ar) = ai;
			(*ag) = ai;
			(*ab) = ai;
			break;
		}
		case TXTTABCNTL_C_ABTSEL_CITER:
		{
			(*ar) = ri;
			(*ag) = gi;
			(*ab) = bi;
			break;
		}
		case TXTTABCNTL_C_ABTSEL_AT:
		{
			(*ar) = at;
			(*ag) = at;
			(*ab) = at;
			break;
		}
		case TXTTABCNTL_C_ABTSEL_CT:
		{
			(*ar) = rt;
			(*ag) = gt;
			(*ab) = bt;
			break;
		}
		case TXTTABCNTL_C_ABTSEL_ACONST:
		{
			UINT32 cnst = ssbt ? m_tm.tex_srcconst3 : m_tm.tex_srcconst2;
			UINT8 aval = (cnst >> 24) & 0x7f;

			aval = (aval << 1) | (aval >> 6);

			(*ar) = aval;
			(*ag) = aval;
			(*ab) = aval;
			break;
		}
		case TXTTABCNTL_C_ABTSEL_CCONST:
		{
			UINT32 cnst = ssbt ? m_tm.tex_srcconst3 : m_tm.tex_srcconst2;

			(*ar) = (cnst >> 16) & 0xff;
			(*ag) = (cnst >>  8) & 0xff;
			(*ab) = (cnst >>  0) & 0xff;
			break;
		}
		default:
		{
			(*ar) = 0;
			(*ag) = 0;
			(*ab) = 0;
			break;
		}
	}
}

static void te_select_mul(UINT32 sel, UINT32 ai, UINT32 at, UINT32 ssbt,
							   UINT32 * a )
{
	switch (sel)
	{
		case TXTTABCNTL_A_ABSEL_AITER:
		{
			(*a) = ai;
			break;
		}
		case TXTTABCNTL_A_ABSEL_AT:
		{
			(*a) = at;
			break;
		}
		case TXTTABCNTL_A_ABSEL_ACONST:
		{
			UINT32 cnst = ssbt ? m_tm.tex_srcconst3 : m_tm.tex_srcconst2;
			UINT8 aval = (cnst >> 24) & 0x7f;

			(*a) = (aval << 1) | (aval >> 6);
			break;
		}
		default:
		{
			(*a) = 0;
			break;
		}
	}
}

static void te_write_dst_pixel(void)
{
	UINT32 mask = m_db.usergen_ctrl & DBUSERGENCTL_DESTOUT_MASK;

	if (m_db.dst_ctrl & DBDESTCNTL_32BPP)
	{
		UINT32 dstaddr = m_db.dst_baseaddr + (m_dbstate.y * m_db.dst_xstride + m_dbstate.x) * sizeof(UINT32);

		UINT32 old = m2_ram_dword(dstaddr);
		UINT32 out = 0;

		UINT8 sdsb_a = (old >> 24) & 0xff;
		UINT8 sr = (old >> 16) & 0xff;
		UINT8 sg = (old >> 8) & 0xff;
		UINT8 sb = old & 0xff;

		out |= (mask & 8 ? (m_dbstate.dsb << 7) | (m_dbstate.dst.a >> 1) : sdsb_a) << 24;
		out |= (mask & 4 ? m_dbstate.dst.r : sr) << 16;
		out |= (mask & 2 ? m_dbstate.dst.g : sg) << 8;
		out |= mask & 1 ? m_dbstate.dst.b : sb;

		m2_ram_write_dword(dstaddr, out);
	}
	else
	{
		UINT32 dstaddr = m_db.dst_baseaddr + (m_dbstate.y * m_db.dst_xstride + m_dbstate.x) * sizeof(UINT16);

		UINT16 old = m2_ram_word(dstaddr);
		UINT16 out = 0;

		UINT8 sdsb_a = (old >> 15) & 0x1;
		UINT8 sr = (old >> 10) & 0x1f;
		UINT8 sg = (old >> 5) & 0x1f;
		UINT8 sb = old & 0x1f;

		out |= (mask & 8 ? m_dbstate.dsb : sdsb_a) << 15;
		out |= (mask & 4 ? m_dbstate.dst.r >> 3 : sr) << 10;
		out |= (mask & 2 ? m_dbstate.dst.g >> 3 : sg) << 5;
		out |= mask & 1 ? m_dbstate.dst.b >> 3 : sb;

		m2_ram_write_word(dstaddr, out);
	}
}

static void te_destination_blend(UINT32 x, UINT32 y, UINT32 w, const rgba * ti_color, UINT8 ssb)
{
	m_dbstate.x = x;
	m_dbstate.y = y;
	m_dbstate.w = w;
	m_dbstate.ti = (*ti_color);
	m_dbstate.ssb = ssb;

	bool dis = false;
	bool zpixout = true;
	bool zbufout = true;
	bool zclip = false;
	bool zclipdis = false;
	bool fbclipdis = false;
	bool winclipdis = false;

	[[maybe_unused]] UINT32 zgel = 0;

	UINT32 zaddr;

	{
		UINT32 xclip = (m_db.fbclip & DBFBCLIP_XFBCLIP_MASK) >> DBFBCLIP_XFBCLIP_SHIFT;
		UINT32 yclip = (m_db.fbclip & DBFBCLIP_YFBCLIP_MASK) >> DBFBCLIP_YFBCLIP_SHIFT;

		fbclipdis = (x >= xclip) || (y >= yclip);

		if (fbclipdis)
			m_db.status |= DBSTATUS_FBCLIP;
	}

	{
		UINT32 xmin = (m_db.x_winclip & DBFBXWINCLIP_XMIN_MASK) >> DBFBXWINCLIP_XMIN_SHIFT;
		UINT32 xmax = (m_db.x_winclip & DBFBXWINCLIP_XMAX_MASK) >> DBFBXWINCLIP_XMAX_SHIFT;
		UINT32 ymin = (m_db.y_winclip & DBFBYWINCLIP_YMIN_MASK) >> DBFBYWINCLIP_YMIN_SHIFT;
		UINT32 ymax = (m_db.y_winclip & DBFBYWINCLIP_YMAX_MASK) >> DBFBYWINCLIP_YMAX_SHIFT;

		bool inside = (x >= xmin) && (x < xmax) && (y >= ymin) && (y < ymax);

		winclipdis = ((m_db.usergen_ctrl & DBUSERGENCTL_WCLIPINEN) && inside) ||
					((m_db.usergen_ctrl & DBUSERGENCTL_WCLIPOUTEN) && !inside);

		if (winclipdis)
			m_db.status |= DBSTATUS_WINCLIP;
	}

	te_select_src_pixel();

	te_select_tex_pixel();

	te_select_alpha_dsb();

	{
		UINT8 dm10, dm11, dm20, dm21;
		UINT32 txtcnst0 = m_db.txt_coef_const0;
		UINT32 txtcnst1 = m_db.txt_coef_const1;
		UINT32 srccnst0 = m_db.src_coef_const0;
		UINT32 srccnst1 = m_db.src_coef_const1;

		if ((m_db.usergen_ctrl & DBUSERGENCTL_BLENDEN) && !(m_gc.te_master_mode & TEMASTER_MODE_DBLEND))
		{
			dm10 = txtcnst0 & 0xff; txtcnst0 >>= 8;
			dm11 = txtcnst1 & 0xff; txtcnst1 >>= 8;
			dm20 = srccnst0 & 0xff; srccnst0 >>= 8;
			dm21 = srccnst1 & 0xff; srccnst1 >>= 8;
			m_dbstate.blend.b = te_color_blend(m_dbstate.texpath.b, m_dbstate.ti.b, m_dbstate.srcpath.b, m_dbstate.src.b, dm10, dm11, dm20, dm21);

			dm10 = txtcnst0 & 0xff; txtcnst0 >>= 8;
			dm11 = txtcnst1 & 0xff; txtcnst1 >>= 8;
			dm20 = srccnst0 & 0xff; srccnst0 >>= 8;
			dm21 = srccnst1 & 0xff; srccnst1 >>= 8;
			m_dbstate.blend.g = te_color_blend(m_dbstate.texpath.g, m_dbstate.ti.g, m_dbstate.srcpath.g, m_dbstate.src.g, dm10, dm11, dm20, dm21);

			dm10 = txtcnst0 & 0xff;
			dm11 = txtcnst1 & 0xff;
			dm20 = srccnst0 & 0xff;
			dm21 = srccnst1 & 0xff;
			m_dbstate.blend.r = te_color_blend(m_dbstate.texpath.r, m_dbstate.ti.r, m_dbstate.srcpath.r, m_dbstate.src.r, dm10, dm11, dm20, dm21);
		}
		else
		{
			m_dbstate.blend.r = m_dbstate.ti.r;
			m_dbstate.blend.g = m_dbstate.ti.g;
			m_dbstate.blend.b = m_dbstate.ti.b;
		}

		if (m_db.usergen_ctrl & DBUSERGENCTL_DITHEREN)
		{
			UINT32 dith_x = x & 3;
			UINT32 dith_y = y & 3;

			UINT32 dith_mtx = dith_y & 2 ? m_db.dither_mat_b : m_db.dither_mat_a;
			UINT32 idx = 7 ^ (((dith_y & 1) << 2) | dith_x);
			UINT8 val = (dith_mtx >> (idx * 4)) & 0xf;

			m_dbstate.dst.r = te_dither(m_dbstate.blend.r, val);
			m_dbstate.dst.g = te_dither(m_dbstate.blend.g, val);
			m_dbstate.dst.b = te_dither(m_dbstate.blend.b, val);
		}
		else
		{
			m_dbstate.dst.r = m_dbstate.blend.r;
			m_dbstate.dst.g = m_dbstate.blend.g;
			m_dbstate.dst.b = m_dbstate.blend.b;
		}
	}

	if (!(m_gc.te_master_mode & TEMASTER_MODE_DZBUF) &&
		(m_db.usergen_ctrl & DBUSERGENCTL_ZBUFEN))
	{
		INT32 x_offs = te_sign_extend((m_db.z_offset & DBZOFFS_XOFFS_MASK) >> DBZOFFS_XOFFS_SHIFT, 12);
		INT32 y_offs = te_sign_extend((m_db.z_offset & DBZOFFS_YOFFS_MASK) >> DBZOFFS_YOFFS_SHIFT, 12);

		x_offs += m_dbstate.x;
		y_offs += m_dbstate.y;

		UINT32 x_clip = (m_db.z_clip & DBZCLIP_XCLIP_MASK) >> DBZCLIP_XCLIP_SHIFT;
		UINT32 y_clip = (m_db.z_clip & DBZCLIP_YCLIP_MASK) >> DBZCLIP_YCLIP_SHIFT;

		zclip = (x_offs < 0) || (y_offs < 0) || (x_offs >= x_clip) || (y_offs >= y_clip);

		zaddr = m_db.z_baseaddr + (x_offs + y_offs * m_db.dst_xstride) * 2;
	}
	else
	{
		zaddr = 0;
		zclip = false;
	}

	zclipdis = zclip && (m_db.discard_ctrl & DBDISCARDCTL_ZCLIPDISEN);

	if (zclipdis)
		m_db.status |= DBSTATUS_ZCLIP;

	if (!(m_gc.te_master_mode & TEMASTER_MODE_DZBUF) &&
		(m_db.usergen_ctrl & DBUSERGENCTL_ZBUFEN) &&
		!zclip)
	{
		INT32 zdiff = w - m2_ram_word(zaddr);

		if (zdiff > 0)
		{
			zpixout = m_db.z_ctrl & 1;
			zbufout = m_db.z_ctrl & 2;
			zgel |= 4;
			m_db.status |= DBSTATUS_ZFUNC_GT;
		}
		else if (zdiff == 0)
		{
			zpixout = m_db.z_ctrl & DBZCNTL_ZPIXOUT_EQ;
			zbufout = m_db.z_ctrl & DBZCNTL_ZBUFOUT_EQ;
			zgel |= 2;
			m_db.status |= DBSTATUS_ZFUNC_EQ;
		}
		else
		{
			zpixout = m_db.z_ctrl & 0x10;
			zbufout = m_db.z_ctrl & 0x20;
			zgel |= 1;
			m_db.status |= DBSTATUS_ZFUNC_LT;
		}
	}
	else
	{
		zpixout = true;
		zbufout = false;
	}

	{
		bool ssbdis = (m_db.discard_ctrl & DBDISCARDCTL_SSBDISEN) && (m_dbstate.ssb == 0);

		bool adis = (m_db.discard_ctrl & DBDISCARDCTL_ADISEN) && (m_dbstate.dst.a == 0);

		bool rgbdis = (m_db.discard_ctrl & DBDISCARDCTL_RGBDISEN) &&
					(m_dbstate.dst.r == 0) && (m_dbstate.dst.g == 0) && (m_dbstate.dst.b == 0);

		dis = fbclipdis || winclipdis || zclipdis || ssbdis || adis || rgbdis;
	}

	if (!dis)
	{
		if (zbufout && (m_db.usergen_ctrl & DBUSERGENCTL_ZOUTEN))
		{
			m2_ram_write_word(zaddr, w & 0xffff);
		}

		if (zpixout && (m_db.supergen_ctrl & DBSUPERGENCTL_DESTOUTEN))
		{
			te_write_dst_pixel();
		}
	}
}

static void te_select_tex_pixel(void)
{
	UINT32 cntl;

	if (m_dbstate.ti.a == 0)
		cntl = (m_db.src_alpha_ctrl >> 4) & 3;
	else if (m_dbstate.ti.a == 255)
		cntl = m_db.src_alpha_ctrl & 3;
	else
		cntl = (m_db.src_alpha_ctrl >> 2) & 3;

	switch (cntl)
	{
		case 0: m_dbstate.texpath.a = m_dbstate.ti.a;   break;
		case 1: m_dbstate.texpath.a = 255;              break;
		case 2: m_dbstate.texpath.a = 0;                break;
	}

	switch ((m_db.txt_mult_cntl & DBTXTMULTCNTL_INSEL_MASK) >> DBTXTMULTCNTL_INSEL_SHIFT)
	{
		case DBTXTMULTCNTL_INSEL_CTI:
		{
			m_dbstate.texpath.r = m_dbstate.ti.r;
			m_dbstate.texpath.g = m_dbstate.ti.g;
			m_dbstate.texpath.b = m_dbstate.ti.b;
			break;
		}
		case DBTXTMULTCNTL_INSEL_CONSTANT:
		{
			UINT32 cnst = m_db.const_in;
			m_dbstate.texpath.r = (cnst >> 16) & 0xff;
			m_dbstate.texpath.g = (cnst >> 8) & 0xff;
			m_dbstate.texpath.b = cnst & 0xff;
			break;
		}
		case DBTXTMULTCNTL_INSEL_COMPSRC:
		{
			m_dbstate.texpath.r = ~m_dbstate.src.r;
			m_dbstate.texpath.g = ~m_dbstate.src.g;
			m_dbstate.texpath.b = ~m_dbstate.src.b;
			break;
		}
		case DBTXTMULTCNTL_INSEL_ATI:
		{
			m_dbstate.texpath.r = m_dbstate.ti.a;
			m_dbstate.texpath.g = m_dbstate.ti.a;
			m_dbstate.texpath.b = m_dbstate.ti.a;
			break;
		}
	}

	if (m_db.txt_mult_cntl & DBTXTMULTCNTL_TXTRJUST)
	{
		m_dbstate.texpath.r >>= 3;
		m_dbstate.texpath.g >>= 3;
		m_dbstate.texpath.b >>= 3;
	}
}

static void te_select_src_pixel(void)
{
	if ((m_db.usergen_ctrl & DBUSERGENCTL_SRCINEN)
		&& (m_db.usergen_ctrl & DBUSERGENCTL_BLENDEN)
		&& !(m_gc.te_master_mode & TEMASTER_MODE_DBLEND))
	{
		INT32 x_offs = te_sign_extend((m_db.src_offset & DBSRCOFFS_XOFFS_MASK) >> DBSRCOFFS_XOFFS_SHIFT, 12);
		INT32 y_offs = te_sign_extend((m_db.src_offset & DBSRCOFFS_YOFFS_MASK) >> DBSRCOFFS_YOFFS_SHIFT, 12);

		x_offs += m_dbstate.x;
		y_offs += m_dbstate.y;

		UINT32 addr = y_offs * m_db.src_xstride + x_offs;

		if (m_db.src_ctrl & DBSRCCNTL_32BPP)
		{
			UINT32 srcaddr = m_db.src_baseaddr + addr * sizeof(UINT32);
			UINT32 srcval = m2_ram_dword(srcaddr);

			m_dbstate.src.a = ((srcval >> 24) & 0x7f) << 1;
			m_dbstate.src.r = (srcval >> 16) & 0xff;
			m_dbstate.src.g = (srcval >>  8) & 0xff;
			m_dbstate.src.b = (srcval >>  0) & 0xff;

			if (m_db.src_ctrl & DBSRCCNTL_MSBREP)
				m_dbstate.src.a |= (m_dbstate.src.a >> 6) & 1;

			m_dbstate.dsb = (srcval >> 31) & 1;
		}
		else
		{
			UINT32 srcaddr = m_db.src_baseaddr + addr * sizeof(UINT16);
			UINT32 srcval = m2_ram_word(srcaddr);

			m_dbstate.src.r = (srcval >> 10) & 0x1f;
			m_dbstate.src.g = (srcval >> 5) & 0x1f;
			m_dbstate.src.b = srcval & 0x1f;
			m_dbstate.src.a = 0;

			m_dbstate.dsb = srcval & 0x8000;
			m_dbstate.src.r <<= 3;
			m_dbstate.src.g <<= 3;
			m_dbstate.src.b <<= 3;

			if (m_db.src_ctrl & DBSRCCNTL_MSBREP)
			{
				m_dbstate.src.r |= (m_dbstate.src.r >> 5);
				m_dbstate.src.g |= (m_dbstate.src.g >> 5);
				m_dbstate.src.b |= (m_dbstate.src.b >> 5);
			}
		}
	}
	else
	{
		m_dbstate.dsb = 0;
		m_dbstate.src.r = 0;
		m_dbstate.src.g = 0;
		m_dbstate.src.b = 0;
		m_dbstate.src.a = 0;
	}

	switch ((m_db.src_mult_cntl & DBSRCMULTCNTL_INSEL_MASK) >> DBSRCMULTCNTL_INSEL_SHIFT)
	{
		case DBSRCMULTCNTL_INSEL_SRC:
		{
			m_dbstate.srcpath.r = m_dbstate.src.r;
			m_dbstate.srcpath.g = m_dbstate.src.g;
			m_dbstate.srcpath.b = m_dbstate.src.b;
			break;
		}
		case DBSRCMULTCNTL_INSEL_CONSTANT:
		{
			UINT32 cnst = m_db.const_in;
			m_dbstate.srcpath.r = (cnst >> 16) & 0xff;
			m_dbstate.srcpath.g = (cnst >> 8) & 0xff;
			m_dbstate.srcpath.b = (cnst >> 0) & 0xff;
			break;
		}
		case DBSRCMULTCNTL_INSEL_COMPCTI:
		{
			m_dbstate.srcpath.r = ~m_dbstate.ti.r;
			m_dbstate.srcpath.g = ~m_dbstate.ti.g;
			m_dbstate.srcpath.b = ~m_dbstate.ti.b;
			break;
		}
		case DBSRCMULTCNTL_INSEL_TEXALPHA:
		{
			m_dbstate.srcpath.r = m_dbstate.src.a;
			m_dbstate.srcpath.g = m_dbstate.src.a;
			m_dbstate.srcpath.b = m_dbstate.src.a;
			break;
		}
	}

	if (m_db.src_mult_cntl & DBSRCMULTCNTL_SRCRJUST)
	{
		m_dbstate.srcpath.r >>= 3;
		m_dbstate.srcpath.g >>= 3;
		m_dbstate.srcpath.b >>= 3;
	}

	m_dbstate.srcpath.a = m_dbstate.src.a;
}

static UINT8 te_dither(UINT8 in, UINT8 dithval)
{
	INT32 res;
	INT32 sgn_val;

	if (dithval & 8)
		sgn_val = -8 + (dithval & 7);
	else
		sgn_val = dithval;

	res = (INT32)in + sgn_val;

	if (res > 255)
		res = 255;
	else if (res < 0)
		res = 0;

	return (UINT8)res;
}

static UINT8 te_get_src_coef(UINT8 cti, UINT8 dm2const0, UINT8 dm2const1)
{
	UINT32 sel=0;
	UINT8 cnst, coef=0;

	switch ((m_db.src_mult_cntl & DBSRCMULTCNTL_SRCCONSTCNTL_MASK) >> DBSRCMULTCNTL_SRCCONSTCNTL_SHIFT)
	{
		case DBSRCMULTCNTL_SRCCONSTCNTL_TEXSSB: sel = m_dbstate.ssb;    break;
		case DBSRCMULTCNTL_SRCCONSTCNTL_SRCDSB: sel = m_dbstate.dsb;    break;
	}

	cnst = sel ? dm2const1 : dm2const0;

	switch ((m_db.src_mult_cntl & DBSRCMULTCNTL_COEFSEL_MASK) >> DBSRCMULTCNTL_COEFSEL_SHIFT)
	{
		case DBSRCMULTCNTL_COEFSEL_ATI:         coef = m_dbstate.texpath.a;     break;
		case DBSRCMULTCNTL_COEFSEL_ASRC:        coef = m_dbstate.srcpath.a;     break;
		case DBSRCMULTCNTL_COEFSEL_CONSTANT:    coef = cnst;                    break;
		case DBSRCMULTCNTL_COEFSEL_CTI:         coef = cti;                     break;
	}

	if (m_db.src_mult_cntl & DBSRCMULTCNTL_SRCCOEFCMP)
		return ~coef;
	else
		return coef;
}

static UINT8 te_get_tex_coef(UINT8 cs, UINT8 dm1const0, UINT8 dm1const1)
{
	UINT32 sel=0;
	UINT8 cnst, coef=0;

	UINT32 cntl = ((m_db.txt_mult_cntl & DBTXTMULTCNTL_TXTCONSTCNTL_MASK) >> DBTXTMULTCNTL_TXTCONSTCNTL_SHIFT);

	if (cntl == DBTXTMULTCNTL_TXTCONSTCNTL_TEXSSB)
		sel = m_dbstate.ssb;
	else if (cntl == DBTXTMULTCNTL_TXTCONSTCNTL_SRCDSB)
		sel = m_dbstate.dsb;

	cnst = sel ? dm1const1 : dm1const0;

	switch ((m_db.txt_mult_cntl & DBTXTMULTCNTL_COEFSEL_MASK) >> DBTXTMULTCNTL_COEFSEL_SHIFT)
	{
		case DBTXTMULTCNTL_COEFSEL_ATI:         coef = m_dbstate.texpath.a;     break;
		case DBTXTMULTCNTL_COEFSEL_ASRC:        coef = m_dbstate.srcpath.a;     break;
		case DBTXTMULTCNTL_COEFSEL_CONSTANT:    coef = cnst;                    break;
		case DBTXTMULTCNTL_COEFSEL_CSRC:        coef = cs;                      break;
	}

	if (m_db.txt_mult_cntl & DBTXTMULTCNTL_TXTCOEFCMP)
		return ~coef;
	else
		return coef;
}

static void te_select_alpha_dsb(void)
{
	if ((m_db.usergen_ctrl & DBUSERGENCTL_BLENDEN) && !(m_gc.te_master_mode & TEMASTER_MODE_DBLEND))
	{
		UINT32 sel = 0;
		UINT8 aconst;

		switch ((m_db.dst_alpha_ctrl & DBDSTACNTL_ADESTCONSTCNTL_MASK) >> DBDSTACNTL_ADESTCONSTCNTL_SHIFT)
		{
			case 0: sel = m_dbstate.ssb;    break;
			case 1: sel = m_dbstate.dsb;    break;
		}

		if (sel)
			aconst = (m_db.dst_alpha_const & DBDSTALPHACONST_CONST1_MASK) >> DBDSTALPHACONST_CONST1_SHIFT;
		else
			aconst = (m_db.dst_alpha_const & DBDSTALPHACONST_CONST0_MASK) >> DBDSTALPHACONST_CONST0_SHIFT;

		switch ((m_db.dst_alpha_ctrl & DBDSTACNTL_ADESTSEL_MASK) >> DBDSTACNTL_ADESTSEL_SHIFT)
		{
			case 0: m_dbstate.dst.a = m_dbstate.texpath.a;  break;
			case 1: m_dbstate.dst.a = aconst;               break;
			case 2: m_dbstate.dst.a = m_dbstate.srcpath.a;  break;
			case 3: m_dbstate.dst.a = m_dbstate.blend.r;    break;
		}

		switch ((m_db.ssbdsb_ctrl & DBSSBDSBCNTL_DSBSEL_MASK) >> DBSSBDSBCNTL_DSBSEL_SHIFT)
		{
			case 0: m_dbstate.dsb = m_dbstate.ssb;      break;
			case 1: m_dbstate.dsb = (m_db.ssbdsb_ctrl & DBSSBDSBCNTL_DSBCONST_MASK) >> DBSSBDSBCNTL_DSBCONST_SHIFT; break;
			case 2: m_dbstate.dsb = m_dbstate.dsb;      break;
		}
	}
	else
	{
		m_dbstate.dst.a = m_dbstate.texpath.a;
		m_dbstate.dsb = m_dbstate.ssb;
	}
}

static UINT8 te_color_blend(UINT8 ct, UINT8 cti, UINT8 cs, UINT8 csrc,
								UINT8 dm10, UINT8 dm11,
								UINT8 dm20, UINT8 dm21)
{
	UINT8 tcoef, scoef;
	UINT16 tm, sm;

	tcoef = te_get_tex_coef(csrc, dm10, dm11);
	scoef = te_get_src_coef(cti, dm20, dm21);

	tm = (tcoef == 255) ? ct : ((ct == 255) ? tcoef : ((tcoef * ct) >> 8));
	sm = (scoef == 255) ? cs : ((cs == 255) ? scoef : ((scoef * cs) >> 8));

	return te_alu_calc(tm, sm);
}

static UINT8 te_alu_calc(UINT16 a, UINT16 b)
{
	INT32 result = 0;
	UINT32 blendout;
	UINT32 carry = 0;
	UINT32 borrow = 0;
	UINT32 cntl = (m_db.alu_ctrl & DBALUCNTL_ALUOP_MASK) >> DBALUCNTL_ALUOP_SHIFT;

	if ((cntl & 8) == 0)
	{
		if (!(cntl & 4))
		{
			result = a + b;
		}
		else if (cntl & 2)
		{
			result = b - a;
			if (result < 0)
				borrow = 1;
		}
		else
		{
			result = a - b;
			if (result < 0)
				borrow = 1;
		}
	}

	else
	{
		int i, j;

		for (i = 0; i < 8; ++i)
		{
			j = (a & 1) *2 + (b & 1);

			result >>= 1;
			result |= (cntl >> j & 1) ? 0x80 : 0;

			a >>= 1;
			b >>= 1;
		}
	}

	result &= 0x1ff;

	switch ((m_db.alu_ctrl & DBALUCNTL_FINALDIVIDE_MASK) >> DBALUCNTL_FINALDIVIDE_SHIFT)
	{
		case 1:  blendout = result << 1;    break;
		case 2:  blendout = result << 2;    break;
		case 3:  blendout = result << 3;    break;
		case 7:  blendout = result >> 1;    break;
		case 6:  blendout = result >> 2;    break;
		case 5:  blendout = result >> 3;    break;
		default: blendout = result;         break;
	}

	if (blendout > 255)
		carry = 1;

	if ((cntl & 8) == 0)
	{
		if (!(cntl & 1))
		{
			blendout = carry ? 255 : blendout;
			blendout = borrow ? 0 : blendout;
		}
		else
		{
			if (cntl & 2)
			{
				blendout = carry ? 255 : blendout;
				blendout = borrow ? 0 : blendout;
			}
		}
	}

	return blendout & 0xff;
}

static void te_walk_span(UINT32 wrange, bool omit_right,
							 UINT32 y, UINT32 xs, UINT32 xe,
							 INT32 r, INT32 g, INT32 b, INT32 a,
							 UINT32 uw, UINT32 vw,
							 UINT32 w)
{
	bool scan_lr = !m_es.r2l;

	xe = scan_lr ? xe + 1 : xe - 1;

	if (omit_right)
	{
		if (scan_lr)
		{
			xe -= 1;
		}
		else
		{
			xs -= 1;
			r -= m_es.ddx_r;
			g -= m_es.ddx_g;
			b -= m_es.ddx_b;
			a -= m_es.ddx_a;
			uw -= m_es.ddx_uw;
			vw -= m_es.ddx_vw;
			w -= m_es.ddx_w;
		}
	}

	while (xs != xe)
	{
		UINT32 sx = xs;
		UINT32 sy = y;

		if (sx == 320/2 && sy == 200)
		{
			;
		}

		UINT32 rt, gt, bt, at, ssbt;

		UINT32 w16 = 0;

		{
			UINT32 normalized_w = w >> -depth_bits[wrange];
			w16 = (normalized_w >> 7) & 0xffff;
		}

		if (!(m_gc.te_master_mode & TEMASTER_MODE_DTEXT) && (m_tm.tex_addr_cntl & TXTADDRCNTL_LOOKUP_EN))
		{
			UINT32 u, v;

			te_texcoord_gen(wrange, uw, vw, w, &u, &v, &w16);

			UINT32 lod = te_lod_calc(u, v, u, v);

			te_get_texture_color(u, v, lod, &rt, &gt, &bt, &at, &ssbt);
		}
		else
		{
			rt = gt = bt = at = ssbt = 0;
		}

		UINT32 ri, gi, bi, ai;

		if (!(m_gc.te_master_mode & TEMASTER_MODE_DSHADE))
		{
			ri = r >> 11;
			gi = g >> 11;
			bi = b >> 11;
			ai = a >> 11;
		}
		else
		{
			ri = 0xff;
			gi = 0xff;
			bi = 0xff;
			ai = 0xff;
		}

		UINT32 ro, go, bo, ao;

		UINT32 ssbo = ssbt;

		te_texture_blend(ri, gi, bi, ai,
					  rt, gt, bt, at, ssbt,
					  &ro, &go, &bo, &ao, &ssbo);

		rgba texout;
		texout.r = ro;
		texout.g = go;
		texout.b = bo;
		texout.a = ao;

		te_destination_blend(sx, sy, w16, &texout, ssbo);

		if (scan_lr)
		{
			xs += 1;
			r += m_es.ddx_r;
			g += m_es.ddx_g;
			b += m_es.ddx_b;
			a += m_es.ddx_a;
			uw += m_es.ddx_uw;
			vw += m_es.ddx_vw;
			w += m_es.ddx_w;
		}
		else
		{
			xs -= 1;
			r -= m_es.ddx_r;
			g -= m_es.ddx_g;
			b -= m_es.ddx_b;
			a -= m_es.ddx_a;
			uw -= m_es.ddx_uw;
			vw -= m_es.ddx_vw;
			w -= m_es.ddx_w;
		}

		r = te_clamp(r, 0, 0x0007ffff);
		g = te_clamp(g, 0, 0x0007ffff);
		b = te_clamp(b, 0, 0x0007ffff);
		a = te_clamp(a, 0, 0x0007ffff);

		uw = te_clamp(uw, 0, 0x007fffff);
		vw = te_clamp(vw, 0, 0x007fffff);

		w = te_clamp(w, 0, 0x007fffff);
	}
}

static UINT32 te_irp_fetch(void)
{
	UINT32 data = m2_ram_dword(m_gc.irp);
	m_gc.irp += 4;

	return data;
}

static float te_irp_fetch_float(void)
{
	UINT32 data = m2_ram_dword(m_gc.irp);
	m_gc.irp += 4;

	float result;
	memcpy(&result, &data, sizeof(result));
	return result;
}

static void te_illegal_inst(void)
{
	te_set_interrupt(INTSTAT_UNIMPLEMENTED_INSTR);
	m_state = TE_STOPPED;
}

static void te_execute(void)
{
	while (m_state == TE_RUNNING)
	{
		UINT32 inst = te_irp_fetch();

		switch (inst & INST_MASK)
		{
			case INST_WRITE_REG:
			{
				UINT32 offs = inst & 0xffff;
				INT32 cnt = (inst >> 16) & 0xff;

				while (cnt-- >= 0)
				{
					m2_te_write(offs >> 2, te_irp_fetch());
					offs += 4;

					if (m_state != TE_RUNNING)
						break;
				}
				break;
			}

			case INST_VTX_SHORT:
			{
				INT32 cnt = inst & 0xffff;
				UINT32 flags = inst & 0x001f0000;
				UINT32 ver = (inst >> 24) & 0xf;

				if (ver != 0)
				{
					te_illegal_inst();
					break;
				}

				while (cnt-- >= 0)
				{
					se_vtx vtx = { 0 };

					vtx.x = te_irp_fetch_float();
					vtx.y = te_irp_fetch_float();

					if (flags & VTX_FLAG_SHAD)
					{
						vtx.r = te_irp_fetch_float();
						vtx.g = te_irp_fetch_float();
						vtx.b = te_irp_fetch_float();
						vtx.a = te_irp_fetch_float();
					}
					if (flags & VTX_FLAG_PRSP)
					{
						vtx.w = te_irp_fetch_float();
					}
					if (flags & VTX_FLAG_TEXT)
					{
						vtx.uw = te_irp_fetch_float();
						vtx.vw = te_irp_fetch_float();
					}

					te_add_vertex(&vtx, flags);

					flags &= ~VTX_FLAG_NEW;
				}

				break;
			}
			case INST_VTX_LONG:
			{
				const UINT32 ver = (inst >> 24) & 0xf;

				if (ver != 0)
				{
					te_illegal_inst();
					break;
				}

				fatalerror("Long format unimplemented");

				break;
			}
			case INST_VTX_POINT:
			{
				const UINT32 ver = (inst >> 24) & 0xf;

				if (ver != 0)
				{
					te_illegal_inst();
					break;
				}

				fatalerror("Point format unimplemented");

				break;
			}
			default:
			{
				te_illegal_inst();
				break;
			}
		}

		if (m_gc.irp == m_gc.iwp)
		{
			m_state = m_gc.te_master_mode & TEICNTL_STPL ? TE_STOPPED : TE_PAUSED;
			te_set_interrupt(INTSTAT_LIST_END);
		}
		else if (m_gc.te_master_mode & (TEICNTL_STPI | TEICNTL_STEP))
		{
			m_state = TE_STOPPED;
		}
	};

	if (m_gc.te_master_mode & TEICNTL_INT)
	{
		te_set_interrupt(INTSTAT_IMMEDIATE_INSTR);
	}
}

static UINT8 te_read_tram8(offs_t address)
{
	address &= TEXTURE_RAM_BYTEMASK;

	UINT8 *ptr = (UINT8 *)(&m_tram[0]) + BYTE4_XOR_BE(address);
	return *ptr;
}

static UINT16 te_read_tram16(offs_t address)
{
	address &= TEXTURE_RAM_BYTEMASK;

	UINT8 *ptr = (UINT8 *)(&m_tram[0]) + address;
	return *(UINT16 *)(ptr);
}

static UINT32 te_read_tram32(offs_t address)
{
	address &= TEXTURE_RAM_BYTEMASK;

	UINT8 *ptr = (UINT8 *)(&m_tram[0]) + address;
	return *(UINT32 *)(ptr);
}

static void te_write_tram8(offs_t address, UINT8 data)
{
	assert(address <= TEXTURE_RAM_WORDS * 4);

	UINT8 *ptr = (UINT8 *)(&m_tram[0]) + address;
	*ptr = data;
}

static void te_write_tram16(offs_t address, UINT16 data)
{
	assert(address < TEXTURE_RAM_WORDS * 4);

	UINT8 *ptr = (UINT8 *)(&m_tram[0]) + WORD2_XOR_BE(address);
	*(UINT16 *)(ptr) = data;
}

static void te_write_tram32(offs_t address, UINT32 data)
{
	address &= TEXTURE_RAM_BYTEMASK;

	UINT8 *ptr = (UINT8 *)(&m_tram[0]) + address;
	*(UINT32 *)(ptr) = data;
}

static UINT8 te_read_pipram8(offs_t address)
{
	address &= PIP_RAM_BYTEMASK;

	UINT8 *ptr = (UINT8 *)(&m_pipram[0]) + address;
	return *ptr;
}

static UINT16 te_read_pipram16(offs_t address)
{
	address &= PIP_RAM_BYTEMASK;

	UINT8 *ptr = (UINT8 *)(&m_pipram[0]) + address;
	return *(UINT16 *)(ptr);
}

static UINT32 te_read_pipram32(offs_t address)
{
	address &= PIP_RAM_BYTEMASK;

	UINT8 *ptr = (UINT8 *)(&m_pipram[0]) + address;
	return *(UINT32 *)(ptr);
}

static void te_write_pipram32(offs_t address, UINT32 data)
{
	address &= PIP_RAM_BYTEMASK;

	UINT8 *ptr = (UINT8 *)(&m_pipram[0]) + address;
	*(UINT32 *)(ptr) = data;
}

static UINT32 te_readbits_from_ram(UINT32 * src_addr, UINT32 * bit_offs, UINT32 bits)
{
	UINT32 src_val = m2_ram_dword((*src_addr));

	if ((((*bit_offs) & 31) + bits) > 32)
	{
		UINT32 src_val2 = m2_ram_dword((*src_addr) + 4);

		UINT32 r_bits = (((*bit_offs) & 31) + bits) - 32;
		UINT32 l_bits = bits - r_bits;

		src_val &= (1 << l_bits) - 1;
		src_val <<= r_bits;

		src_val2 >>= (32 - r_bits);
		src_val2 &= (1 << r_bits) - 1;

		src_val |= src_val2;
	}
	else
	{
		src_val = src_val >> (32 - (bits + ((*bit_offs) & 31)));
	}

	return src_val &= (1 << bits) - 1;
}

static void te_load_texture(void)
{
	switch (m_tm.texld_cntl & TXTLDCNTL_LDMODE_MASK)
	{
		case TXTLDCNTL_LDMODE_TEXLOAD:
		{
			if (m_tm.texld_cntl & TXTLDCNTL_COMPRESSED)
			{
				UINT32 src_addr = m_tm.texld_srcbase;
				UINT32 bit_offs = m_tm.texld_cntl & TXTLDCNTL_SRCBITOFFS;

				UINT32 dst_addr = m_tm.texld_dstbase << 3;
				INT32 texels = m_tm.tex_bytecnt;

				UINT32 dst_type = m_tm.tex_exptype;
				UINT32 dst_cdepth = (dst_type & TXTEXPFORM_CDEPTH_MASK) >> TXTEXPFORM_CDEPTH_SHIFT;
				UINT32 dst_adepth = (dst_type & TXTEXPFORM_ADEPTH_MASK) >> TXTEXPFORM_ADEPTH_SHIFT;
				UINT32 dst_bits = 0;

				if (dst_type & TXTEXPFORM_COLORON)
				{
					if (dst_type & TXTEXPFORM_LITERAL)
						dst_bits += dst_cdepth * 3;
					else
						dst_bits += dst_cdepth;
				}

				if (dst_type & TXTEXPFORM_SSBON)
					++dst_bits;

				if (dst_bits & TXTEXPFORM_ALPHAON)
					dst_bits += dst_adepth;

				while (texels > 0)
				{
					UINT8 cntl = te_readbits_from_ram(&src_addr, &bit_offs, 8);

					UINT32 type = (cntl >> 6) & 3;
					UINT32 cnst;

					if (type == 0)
						cnst = m_tm.tex_srcconst0;
					else if (type == 1)
						cnst = m_tm.tex_srcconst1;
					else if (type == 2)
						cnst = m_tm.tex_srcconst2;
					else
						cnst = m_tm.tex_srcconst3;

					UINT32 src_type = type & 2 ? m_tm.tex_srctype23 : m_tm.tex_srctype01;

					if (type & 1)
						src_type = src_type >> 16;

					const bool has_color = src_type & TXTEXPFORM_COLORON;
					const bool has_alpha = src_type & TXTEXPFORM_ALPHAON;
					const bool has_ssb = src_type & TXTEXPFORM_SSBON;
					const bool is_transparent = src_type & TXTEXPFORM_TRANSPARENT;
					const bool is_literal = src_type & TXTEXPFORM_LITERAL;

					UINT32 cnt = (cntl & (is_transparent ? 0x3f : 0x1f)) + 1;

					UINT32 src_cdepth = (src_type & TXTEXPFORM_CDEPTH_MASK) >> TXTEXPFORM_CDEPTH_SHIFT;
					UINT32 src_adepth = (src_type & TXTEXPFORM_ADEPTH_MASK) >> TXTEXPFORM_ADEPTH_SHIFT;
					UINT32 src_bits = 0;

					if (is_literal)
						src_cdepth *= 3;

					UINT32 src_val = 0;

					if (is_transparent)
					{
						src_val = cnst;
					}
					else
					{
						src_bits += src_cdepth;

						if (has_ssb)
							++src_bits;

						if (has_alpha)
							src_bits += src_adepth;
					}

					if (!is_transparent && (cntl & 0x20))
					{
						for (UINT32 i = 0; i < cnt; ++i)
						{
							UINT32 dst_val = 0;
							UINT32 src_ssb = 0;
							UINT32 src_color = 0;
							UINT32 src_alpha = 0;
							UINT32 src_val = te_readbits_from_ram(&src_addr, &bit_offs, src_bits);

							if (has_color)
							{
								src_color = src_val & ((1 << src_cdepth) - 1);
								src_val >>= src_cdepth;

								if (!(src_type & TXTEXPFORM_INDEXON))
									src_color += cnst & 0xff;
							}

							if (has_alpha)
							{
								if (is_transparent)
								{
									src_alpha = (cnst >> 24) & ((1 << src_adepth) - 1);
								}
								else
								{
									src_alpha = src_val & ((1 << src_adepth) - 1);
									src_val >>= src_adepth;
								}
							}

							if (has_ssb)
							{
								if (is_transparent)
									src_ssb = (cnst >> 31) & 1;
								else
									src_ssb = src_val & 1;
							}

							dst_val = src_color
									| (src_alpha << src_cdepth)
									| (src_ssb << (src_cdepth + src_adepth));

							UINT32 oldval = m_tram[(dst_addr >> 3) >> 2];

							UINT32 shift = (32 - (dst_bits + (dst_addr & 31)));
							oldval &= ~(((1 << dst_bits) - 1) << shift);
							oldval |= dst_val << shift;

							m_tram[(dst_addr >> 3) >> 2] = oldval;

							dst_addr += dst_bits;
						}
					}
					else
					{
						UINT32 dst_val;

						if (!is_transparent)
						{
							src_val = te_readbits_from_ram(&src_addr, &bit_offs, src_bits);

							if (!is_literal)
								src_val += cnst & 0xff;
						}

						dst_val = src_val;

						for (UINT32 i = 0; i < cnt; ++i)
						{
							UINT32 oldval = m_tram[(dst_addr >> 3) & ~3];

							UINT32 shift = (32 - (dst_bits + (dst_addr & 31)));
							oldval &= ~(((1 << dst_bits) - 1) << shift);
							oldval |= dst_val << shift;

							m_tram[(dst_addr >> 3) >> 2] = oldval;

							dst_addr += dst_bits;
						}
					}

					texels -= cnt;
				}
			}
			else
			{
				fatalerror("REGULAR TEXTURE NOT SUPPORTED\n");
			}

			break;
		}

		case TXTLDCNTL_LDMODE_MMDMA:
		{
			UINT32 srcaddr = m_tm.tex_mm_srcbase;
			UINT32 dstaddr = m_tm.tex_mm_dstbase;
			UINT32 bytes = m_tm.tex_bytecnt;

			assert((bytes & 3) == 0);

			if (m_tm.tex_cntl & TXTCNTL_MMDMA_TRAM_ON)
			{
				assert((dstaddr & 3) == 0);

				while (bytes > 0)
				{
					UINT32 data = m2_ram_byte(srcaddr);
					te_write_tram8(dstaddr, data);
					dstaddr ++;
					srcaddr ++;
					bytes--;
				}
			}
			else if (m_tm.tex_cntl & TXTCNTL_MMDMA_PIP_ON)
			{
				assert((dstaddr & 3) == 0);

				while (bytes > 0)
				{
					UINT32 data = m2_ram_dword(srcaddr);
					te_write_pipram32(dstaddr, data);
					dstaddr += 4;
					srcaddr += 4;
					bytes -= 4;
				}
			}

			break;
		}

		case TXTLDCNTL_LDMODE_PIPLOAD:
		{
			UINT32 srcaddr = m_tm.texld_srcbase;
			UINT32 dstaddr = m_tm.texld_dstbase;
			UINT32 bytes = m_tm.tex_bytecnt;

			assert((m_tm.tex_bytecnt & 3) == 0);

			while (bytes > 0)
			{
				UINT32 data = m2_ram_dword(srcaddr);
				te_write_pipram32(dstaddr, data);
				dstaddr += 4;
				srcaddr += 4;
				bytes -= 4;
			}

			break;
		}

		default:
		{
			break;
		}
	}
}

void m2_te_init(void)
{
	memset(&m_gc, 0, sizeof(m_gc));
	memset(&m_se, 0, sizeof(m_se));
	memset(&m_es, 0, sizeof(m_es));
	memset(&m_tm, 0, sizeof(m_tm));
	memset(&m_db, 0, sizeof(m_db));
	memset(m_tram, 0, sizeof(m_tram));
	memset(m_pipram, 0, sizeof(m_pipram));
	m2_te_reset();
}

READ64_HANDLER( m2_te_tram_r )
{
	UINT64 data;
	memcpy(&data, &m_tram[offset * 2], sizeof(data));
	return data;
}

WRITE64_HANDLER( m2_te_tram_w )
{
	UINT64 word;
	memcpy(&word, &m_tram[offset * 2], sizeof(word));
	COMBINE_DATA(&word);
	memcpy(&m_tram[offset * 2], &word, sizeof(word));
}

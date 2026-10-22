#include "driver.h"
#include <math.h>
#undef COMBINE_DATA
#define COMBINE_DATA(p) (*(p) = (*(p) & ~mem_mask) | (data & mem_mask))
#include "debugger.h"
#include "sh3.h"
#include "sh4regs.h"
#include "sh4dmac.h"
#include "sh3comn.h"
#ifdef LSB_FIRST
#define NATIVE_ENDIAN_VALUE_LE_BE(le,be) le
#else
#define NATIVE_ENDIAN_VALUE_LE_BE(le,be) be
#endif
#define CPU_TYPE_SH3 2
#define CPU_TYPE_SH4 3
#define BUSY_LOOP_HACKS 0
#define EXPPRI(pl,po,p,n) (((4-(pl))<<24)|((15-(po))<<16)|((p)<<8)|(255-(n)))
#define NMIPRI() EXPPRI(3,0,16,SH4_INTC_NMI)
#define INTPRI(p,n) EXPPRI(4,2,p,n)
#define LOG(x) do {} while(0)
#define FP_RS(r) sh3.m_fr[(r)]
#define FP_RFS(r) *( (float  *)(sh3.m_fr+(r)) )
#define FP_RFD(r) *( (double *)(sh3.m_fr+(r)) )
#define FP_XS(r) sh3.m_xf[(r)]
#define FP_XFS(r) *( (float  *)(sh3.m_xf+(r)) )
#define FP_XFD(r) *( (double *)(sh3.m_xf+(r)) )
#define FP_RS2(r) sh3.m_fr[(r) ^ sh3.m_fpu_pr]
#define FP_RFS2(r) *( (float  *)(sh3.m_fr+((r) ^ sh3.m_fpu_pr)) )
#define FP_XS2(r) sh3.m_xf[(r) ^ sh3.m_fpu_pr]
#define FP_XFS2(r) *( (float  *)(sh3.m_xf+((r) ^ sh3.m_fpu_pr)) )
#define T   0x00000001
#define S   0x00000002
#define I   0x000000f0
#define Q   0x00000100
#define M   0x00000200
#define FD  0x00008000
#define BL  0x10000000
#define sRB 0x20000000
#define MD  0x40000000
#define AM  0x1fffffff
#define FLAGS   (MD|sRB|BL|FD|M|Q|I|S|T)
#define RM  0x00000003
#define DN  0x00040000
#define PR  0x00080000
#define SZ  0x00100000
#define FR  0x00200000
#define Rn  ((opcode>>8)&15)
#define Rm  ((opcode>>4)&15)
#define REGFLAG_R(n)                    (1 << (n))
#define REGFLAG_FR(n)                   (1 << (n))
#define REGFLAG_XR(n)                   (1 << (n))
#define REGFLAG_PR                      (1 << 0)
#define REGFLAG_MACL                    (1 << 1)
#define REGFLAG_MACH                    (1 << 2)
#define REGFLAG_GBR                     (1 << 3)
#define REGFLAG_VBR                     (1 << 4)
#define REGFLAG_SR                      (1 << 5)
#define REGFLAG_SGR                     (1 << 6)
#define REGFLAG_FPUL                    (1 << 7)
#define REGFLAG_FPSCR                   (1 << 8)
#define REGFLAG_DBR                     (1 << 9)
#define REGFLAG_SSR                     (1 << 10)
#define REGFLAG_SPC                     (1 << 11)

struct sh3_state {
	int c_md2;
	int c_md1;
	int c_md0;
	int c_md6;
	int c_md4;
	int c_md3;
	int c_md5;
	int c_md7;
	int c_md8;
	int c_clock;

	UINT32  m_ppc;
	UINT32  m_pc;
	UINT32  m_spc;
	UINT32  m_pr;
	UINT32  m_sr;
	UINT32  m_ssr;
	UINT32  m_gbr;
	UINT32  m_vbr;
	UINT32  m_mach;
	UINT32  m_macl;
	UINT32  m_r[16];
	UINT32  m_rbnk[2][8];
	UINT32  m_sgr;
	UINT32  m_fr[16];
	UINT32  m_xf[16];
	UINT32  m_ea;
	UINT32  m_delay;
	UINT32  m_cpu_off;
	UINT32  m_pending_irq;
	UINT32  m_test_irq;
	UINT32  m_fpscr;
	UINT32  m_fpul;
	UINT32  m_dbr;

	UINT32  m_exception_priority[128];
	int     m_exception_requesting[128];

	INT8    m_irq_line_state[17];

	UINT32 m_m[16384];

	UINT32 m_SH4_TSTR;
	UINT32 m_SH4_TCNT0;
	UINT32 m_SH4_TCNT1;
	UINT32 m_SH4_TCNT2;
	UINT32 m_SH4_TCR0;
	UINT32 m_SH4_TCR1;
	UINT32 m_SH4_TCR2;
	UINT32 m_SH4_TCOR0;
	UINT32 m_SH4_TCOR1;
	UINT32 m_SH4_TCOR2;
	UINT32 m_SH4_TOCR;
	UINT32 m_SH4_TCPR2;

	UINT32 m_SH4_IPRA;

	UINT32 m_SH4_IPRC;

	UINT32 m_SH4_SAR0;
	UINT32 m_SH4_SAR1;
	UINT32 m_SH4_SAR2;
	UINT32 m_SH4_SAR3;

	UINT32 m_SH4_DAR0;
	UINT32 m_SH4_DAR1;
	UINT32 m_SH4_DAR2;
	UINT32 m_SH4_DAR3;

	UINT32 m_SH4_CHCR0;
	UINT32 m_SH4_CHCR1;
	UINT32 m_SH4_CHCR2;
	UINT32 m_SH4_CHCR3;

	UINT32 m_SH4_DMATCR0;
	UINT32 m_SH4_DMATCR1;
	UINT32 m_SH4_DMATCR2;
	UINT32 m_SH4_DMATCR3;

	UINT32 m_SH4_DMAOR;

	INT8    m_nmi_line_state;

	UINT8 m_sleep_mode;

	int     m_frt_input;
	int     m_irln;
	int     m_internal_irq_level;
	int     m_internal_irq_vector;

	mame_timer *m_dma_timer[4];
	mame_timer *m_refresh_timer;
	mame_timer *m_rtc_timer;
	mame_timer *m_timer[3];
	UINT32  m_refresh_timer_base;
	int     m_dma_timer_active[4];
	UINT32  m_dma_source[4];
	UINT32  m_dma_destination[4];
	UINT32  m_dma_count[4];
	int     m_dma_wordsize[4];
	int     m_dma_source_increment[4];
	int     m_dma_destination_increment[4];
	int     m_dma_mode[4];

	int     m_sh4_icount;
	int     m_is_slave;
	int     m_cpu_clock;
	int     m_bus_clock;
	int     m_pm_clock;
	int     m_fpu_sz;
	int     m_fpu_pr;
	int     m_ioport16_pullup;
	int     m_ioport16_direction;
	int     m_ioport4_pullup;
	int     m_ioport4_direction;

	void    (*m_ftcsr_read_callback)(UINT32 data);

	UINT32 m_sh4_tlb_address[64];
	UINT32 m_sh4_tlb_data[64];
	UINT8 m_sh4_mmu_enabled;

	int m_cpu_type;

	UINT32  m_sh3internal_upper[0x3000/4];
	UINT32  m_sh3internal_lower[0x1000];

	UINT64 m_debugger_temp;


};
static struct sh3_state sh3;
static int (*sh3_irq_callback)(int);
static void sh3_set_input_line(int line,int state);
static void sh4_handle_sar0_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_sar1_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_sar2_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_sar3_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_dar0_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_dar1_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_dar2_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_dar3_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_dmatcr0_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_dmatcr1_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_dmatcr2_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_dmatcr3_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_chcr0_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_chcr1_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_chcr2_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_chcr3_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_dmaor_addr_w(UINT32 data, UINT32 mem_mask);
static void TODO(const UINT16 opcode);
static int sign_of(int n);
static void zero(int n,int sign);
static int data_type_of(int n);
static UINT8 RB(offs_t A);
static UINT16 RW(offs_t A);
static UINT32 RL(offs_t A);
static void WB(offs_t A, UINT8 V);
static void WW(offs_t A, UINT16 V);
static void WL(offs_t A, UINT32 V);
static void ADD(const UINT16 opcode);
static void ADDI(const UINT16 opcode);
static void ADDC(const UINT16 opcode);
static void ADDV(const UINT16 opcode);
static void AND(const UINT16 opcode);
static void ANDI(const UINT16 opcode);
static void ANDM(const UINT16 opcode);
static void BF(const UINT16 opcode);
static void BFS(const UINT16 opcode);
static void BRA(const UINT16 opcode);
static void BRAF(const UINT16 opcode);
static void BSR(const UINT16 opcode);
static void BSRF(const UINT16 opcode);
static void BT(const UINT16 opcode);
static void BTS(const UINT16 opcode);
static void CLRMAC(const UINT16 opcode);
static void CLRT(const UINT16 opcode);
static void CMPEQ(const UINT16 opcode);
static void CMPGE(const UINT16 opcode);
static void CMPGT(const UINT16 opcode);
static void CMPHI(const UINT16 opcode);
static void CMPHS(const UINT16 opcode);
static void CMPPL(const UINT16 opcode);
static void CMPPZ(const UINT16 opcode);
static void CMPSTR(const UINT16 opcode);
static void CMPIM(const UINT16 opcode);
static void DIV0S(const UINT16 opcode);
static void DIV0U(const UINT16 opcode);
static void DIV1(const UINT16 opcode);
static void DMULS(const UINT16 opcode);
static void DMULU(const UINT16 opcode);
static void DT(const UINT16 opcode);
static void EXTSB(const UINT16 opcode);
static void EXTSW(const UINT16 opcode);
static void EXTUB(const UINT16 opcode);
static void EXTUW(const UINT16 opcode);
static void JMP(const UINT16 opcode);
static void JSR(const UINT16 opcode);
static void LDCSR(const UINT16 opcode);
static void LDCGBR(const UINT16 opcode);
static void LDCVBR(const UINT16 opcode);
static void LDCMSR(const UINT16 opcode);
static void LDCMGBR(const UINT16 opcode);
static void LDCMVBR(const UINT16 opcode);
static void LDSMACH(const UINT16 opcode);
static void LDSMACL(const UINT16 opcode);
static void LDSPR(const UINT16 opcode);
static void LDSMMACH(const UINT16 opcode);
static void LDSMMACL(const UINT16 opcode);
static void LDSMPR(const UINT16 opcode);
static void MAC_L(const UINT16 opcode);
static void MAC_W(const UINT16 opcode);
static void MOV(const UINT16 opcode);
static void MOVBS(const UINT16 opcode);
static void MOVWS(const UINT16 opcode);
static void MOVLS(const UINT16 opcode);
static void MOVBL(const UINT16 opcode);
static void MOVWL(const UINT16 opcode);
static void MOVLL(const UINT16 opcode);
static void MOVBM(const UINT16 opcode);
static void MOVWM(const UINT16 opcode);
static void MOVLM(const UINT16 opcode);
static void MOVBP(const UINT16 opcode);
static void MOVWP(const UINT16 opcode);
static void MOVLP(const UINT16 opcode);
static void MOVBS0(const UINT16 opcode);
static void MOVWS0(const UINT16 opcode);
static void MOVLS0(const UINT16 opcode);
static void MOVBL0(const UINT16 opcode);
static void MOVWL0(const UINT16 opcode);
static void MOVLL0(const UINT16 opcode);
static void MOVI(const UINT16 opcode);
static void MOVWI(const UINT16 opcode);
static void MOVLI(const UINT16 opcode);
static void MOVBLG(const UINT16 opcode);
static void MOVWLG(const UINT16 opcode);
static void MOVLLG(const UINT16 opcode);
static void MOVBSG(const UINT16 opcode);
static void MOVWSG(const UINT16 opcode);
static void MOVLSG(const UINT16 opcode);
static void MOVBS4(const UINT16 opcode);
static void MOVWS4(const UINT16 opcode);
static void MOVLS4(const UINT16 opcode);
static void MOVBL4(const UINT16 opcode);
static void MOVWL4(const UINT16 opcode);
static void MOVLL4(const UINT16 opcode);
static void MOVA(const UINT16 opcode);
static void MOVT(const UINT16 opcode);
static void MULL(const UINT16 opcode);
static void MULS(const UINT16 opcode);
static void MULU(const UINT16 opcode);
static void NEG(const UINT16 opcode);
static void NEGC(const UINT16 opcode);
static void NOP(const UINT16 opcode);
static void NOT(const UINT16 opcode);
static void OR(const UINT16 opcode);
static void ORI(const UINT16 opcode);
static void ORM(const UINT16 opcode);
static void ROTCL(const UINT16 opcode);
static void ROTCR(const UINT16 opcode);
static void ROTL(const UINT16 opcode);
static void ROTR(const UINT16 opcode);
static void RTE(const UINT16 opcode);
static void RTS(const UINT16 opcode);
static void SETT(const UINT16 opcode);
static void SHAL(const UINT16 opcode);
static void SHAR(const UINT16 opcode);
static void SHLL(const UINT16 opcode);
static void SHLL2(const UINT16 opcode);
static void SHLL8(const UINT16 opcode);
static void SHLL16(const UINT16 opcode);
static void SHLR(const UINT16 opcode);
static void SHLR2(const UINT16 opcode);
static void SHLR8(const UINT16 opcode);
static void SHLR16(const UINT16 opcode);
static void SLEEP(const UINT16 opcode);
static void STCSR(const UINT16 opcode);
static void STCGBR(const UINT16 opcode);
static void STCVBR(const UINT16 opcode);
static void STCMSR(const UINT16 opcode);
static void STCMGBR(const UINT16 opcode);
static void STCMVBR(const UINT16 opcode);
static void STSMACH(const UINT16 opcode);
static void STSMACL(const UINT16 opcode);
static void STSPR(const UINT16 opcode);
static void STSMMACH(const UINT16 opcode);
static void STSMMACL(const UINT16 opcode);
static void STSMPR(const UINT16 opcode);
static void SUB(const UINT16 opcode);
static void SUBC(const UINT16 opcode);
static void SUBV(const UINT16 opcode);
static void SWAPB(const UINT16 opcode);
static void SWAPW(const UINT16 opcode);
static void TAS(const UINT16 opcode);
static void TRAPA(const UINT16 opcode);
static void TST(const UINT16 opcode);
static void TSTI(const UINT16 opcode);
static void TSTM(const UINT16 opcode);
static void XOR(const UINT16 opcode);
static void XORI(const UINT16 opcode);
static void XORM(const UINT16 opcode);
static void XTRCT(const UINT16 opcode);
static void STCSSR(const UINT16 opcode);
static void STCSPC(const UINT16 opcode);
static void STCSGR(const UINT16 opcode);
static void STSFPUL(const UINT16 opcode);
static void STSFPSCR(const UINT16 opcode);
static void STCDBR(const UINT16 opcode);
static void STCRBANK(const UINT16 opcode);
static void STCMRBANK(const UINT16 opcode);
static void MOVCAL(const UINT16 opcode);
static void CLRS(const UINT16 opcode);
static void SETS(const UINT16 opcode);
static void STCMSGR(const UINT16 opcode);
static void STSMFPUL(const UINT16 opcode);
static void STSMFPSCR(const UINT16 opcode);
static void STCMDBR(const UINT16 opcode);
static void STCMSSR(const UINT16 opcode);
static void STCMSPC(const UINT16 opcode);
static void LDSMFPUL(const UINT16 opcode);
static void LDSMFPSCR(const UINT16 opcode);
static void LDCMDBR(const UINT16 opcode);
static void LDCMRBANK(const UINT16 opcode);
static void LDCMSSR(const UINT16 opcode);
static void LDCMSPC(const UINT16 opcode);
static void LDSFPUL(const UINT16 opcode);
static void LDSFPSCR(const UINT16 opcode);
static void LDCDBR(const UINT16 opcode);
static void SHAD(const UINT16 opcode);
static void SHLD(const UINT16 opcode);
static void LDCRBANK(const UINT16 opcode);
static void LDCSSR(const UINT16 opcode);
static void LDCSPC(const UINT16 opcode);
static void PREFM(const UINT16 opcode);
static void FMOVMRIFR(const UINT16 opcode);
static void FMOVFRMR(const UINT16 opcode);
static void FMOVFRMDR(const UINT16 opcode);
static void FMOVFRS0(const UINT16 opcode);
static void FMOVS0FR(const UINT16 opcode);
static void FMOVMRFR(const UINT16 opcode);
static void FMOVFR(const UINT16 opcode);
static void FLDI1(const UINT16 opcode);
static void FLDI0(const UINT16 opcode);
static void  FLDS(const UINT16 opcode);
static void  FSTS(const UINT16 opcode);
static void FRCHG();
static void FSCHG();
static void FTRC(const UINT16 opcode);
static void FLOAT(const UINT16 opcode);
static void FNEG(const UINT16 opcode);
static void FABS(const UINT16 opcode);
static void FCMP_EQ(const UINT16 opcode);
static void FCMP_GT(const UINT16 opcode);
static void FCNVDS(const UINT16 opcode);
static void FCNVSD(const UINT16 opcode);
static void FADD(const UINT16 opcode);
static void FSUB(const UINT16 opcode);
static void FMUL(const UINT16 opcode);
static void FDIV(const UINT16 opcode);
static void FMAC(const UINT16 opcode);
static void FSQRT(const UINT16 opcode);
static void FSRRA(const UINT16 opcode);
static void FSSCA(const UINT16 opcode);
static void FIPR(const UINT16 opcode);
static void FTRV(const UINT16 opcode);
static void op1111_0xf13(const UINT16 opcode);
static void dbreak(const UINT16 opcode);
static void op1111_0x13(UINT16 opcode);
static void device_reset();
static void execute_one_0000(const UINT16 opcode);
static void execute_one_4000(const UINT16 opcode);
static void execute_one(const UINT16 opcode);
static void execute_run();
static void sh4_change_register_bank(int to);
static void sh4_swap_fp_registers();
static void sh4_swap_fp_couples();
static void sh4_syncronize_register_bank(int to);
static void sh4_default_exception_priorities();
static void sh4_exception_recompute();
static void sh4_exception_request(int exception);
static void sh4_exception_unrequest(int exception);
static void sh4_exception_checkunrequest(int exception);
static void sh4_exception(const char *message, int exception);
static UINT32 compute_ticks_refresh_timer(mame_timer *timer, int hertz, int base, int divisor);
static void sh4_refresh_timer_recompute();
static void sh4_refresh_timer_callback(int param);
static void increment_rtc_time(int mode);
static void sh4_rtc_timer_callback(int param);
static void sh4_dmac_nmi();
static void sh4_handler_ipra_w(UINT32 data, UINT32 mem_mask);
static void sh4_internal_w(offs_t offset, UINT32 data, UINT32 mem_mask);
static UINT32 sh4_internal_r(offs_t offset, UINT32 mem_mask);
static void sh4_set_frt_input(int state);
static void sh4_set_irln_input(int value);
static void execute_set_input(int irqline, int state);
static void sh4_parse_configuration();
static UINT32 sh4_getsqremap(UINT32 address);
static UINT64 sh4_tlb_r(offs_t offset, UINT64 mem_mask);
static void sh4_tlb_w(offs_t offset, UINT64 data, UINT64 mem_mask);
static void sh3_internal_high_w(offs_t offset, UINT32 data, UINT32 mem_mask);
static UINT32 sh3_internal_high_r(offs_t offset, UINT32 mem_mask);
static UINT32 sh3_internal_r(offs_t offset, UINT32 mem_mask);
static void sh3_internal_w(offs_t offset, UINT32 data, UINT32 mem_mask);
static void sh4_dmac_callback(int param);
static int sh4_dma_transfer(int channel, int timermode, UINT32 chcr, UINT32 *sar, UINT32 *dar, UINT32 *dmatcr);
static int sh4_dma_transfer_device(int channel, UINT32 chcr, UINT32 *sar, UINT32 *dar, UINT32 *dmatcr);
static void sh4_dmac_check(int channel);
static int sh4_dma_data(struct sh4_device_dma *s);
static void sh4_dma_ddt(struct sh4_ddt_dma *s);
static double sh4_scale_up_mame_time(double _time1, UINT32 factor1);
static UINT32 compute_ticks_timer(mame_timer *timer, int hertz, int divisor);
static void sh4_timer_recompute(int which);
static void sh4_timer_callback(int param);
static UINT32 sh4_handle_tcnt0_addr_r(UINT32 mem_mask);
static UINT32 sh4_handle_tcnt1_addr_r(UINT32 mem_mask);
static UINT32 sh4_handle_tcnt2_addr_r(UINT32 mem_mask);
static UINT32 sh4_handle_tcor0_addr_r(UINT32 mem_mask);
static UINT32 sh4_handle_tcor1_addr_r(UINT32 mem_mask);
static UINT32 sh4_handle_tcor2_addr_r(UINT32 mem_mask);
static UINT32 sh4_handle_tcr0_addr_r(UINT32 mem_mask);
static UINT32 sh4_handle_tcr1_addr_r(UINT32 mem_mask);
static UINT32 sh4_handle_tcr2_addr_r(UINT32 mem_mask);
static UINT32 sh4_handle_tstr_addr_r(UINT32 mem_mask);
static UINT32 sh4_handle_tocr_addr_r(UINT32 mem_mask);
static UINT32 sh4_handle_tcpr2_addr_r(UINT32 mem_mask);
static void sh4_handle_tstr_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_tcr0_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_tcr1_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_tcr2_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_tcor0_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_tcor1_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_tcor2_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_tcnt0_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_tcnt1_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_tcnt2_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_tocr_addr_w(UINT32 data, UINT32 mem_mask);
static void sh4_handle_tcpr2_addr_w(UINT32 data, UINT32 mem_mask);
	void sh4_check_pending_irq(const char *message)
	{
		int a,irq,z;

		irq = 0;
		z = -1;
		for (a=0;a <= SH4_INTC_ROVI;a++)
		{
			if (sh3.m_exception_requesting[a])
			{
				if ((int)sh3.m_exception_priority[a] > z)
				{
					z = sh3.m_exception_priority[a];
					irq = a;
				}
			}
		}
		if (z >= 0)
		{
			sh4_exception(message, irq);
		}
	}
	UINT32 sh4_handle_sar0_addr_r(UINT32 mem_mask) { return sh3.m_SH4_SAR0; }
	UINT32 sh4_handle_sar1_addr_r(UINT32 mem_mask) { return sh3.m_SH4_SAR1; }
	UINT32 sh4_handle_sar2_addr_r(UINT32 mem_mask) { return sh3.m_SH4_SAR2; }
	UINT32 sh4_handle_sar3_addr_r(UINT32 mem_mask) { return sh3.m_SH4_SAR3; }
	UINT32 sh4_handle_dar0_addr_r(UINT32 mem_mask) { return sh3.m_SH4_DAR0; }
	UINT32 sh4_handle_dar1_addr_r(UINT32 mem_mask) { return sh3.m_SH4_DAR1; }
	UINT32 sh4_handle_dar2_addr_r(UINT32 mem_mask) { return sh3.m_SH4_DAR2; }
	UINT32 sh4_handle_dar3_addr_r(UINT32 mem_mask) { return sh3.m_SH4_DAR3; }
	UINT32 sh4_handle_dmatcr0_addr_r(UINT32 mem_mask) { return sh3.m_SH4_DMATCR0; }
	UINT32 sh4_handle_dmatcr1_addr_r(UINT32 mem_mask) { return sh3.m_SH4_DMATCR1; }
	UINT32 sh4_handle_dmatcr2_addr_r(UINT32 mem_mask) { return sh3.m_SH4_DMATCR2; }
	UINT32 sh4_handle_dmatcr3_addr_r(UINT32 mem_mask) { return sh3.m_SH4_DMATCR3; }
	UINT32 sh4_handle_chcr0_addr_r(UINT32 mem_mask) { return sh3.m_SH4_CHCR0; }
	UINT32 sh4_handle_chcr1_addr_r(UINT32 mem_mask) { return sh3.m_SH4_CHCR1; }
	UINT32 sh4_handle_chcr2_addr_r(UINT32 mem_mask) { return sh3.m_SH4_CHCR2; }
	UINT32 sh4_handle_chcr3_addr_r(UINT32 mem_mask) { return sh3.m_SH4_CHCR3; }
	UINT32 sh4_handle_dmaor_addr_r(UINT32 mem_mask) { return sh3.m_SH4_DMAOR; }
static void TODO(const UINT16 opcode)
{
}

#if 0
static int sign_of(int n)
{
	return(sh3.m_fr[n]>>31);
}

static void zero(int n,int sign)
{
if (sign == 0)
	sh3.m_fr[n] = 0x00000000;
else
	sh3.m_fr[n] = 0x80000000;
if ((sh3.m_fpscr & PR) == 1)
	sh3.m_fr[n+1] = 0x00000000;
}

static int data_type_of(int n)
{
UINT32 abs;

	abs = sh3.m_fr[n] & 0x7fffffff;
	if ((sh3.m_fpscr & PR) == 0) {
		if (abs < 0x00800000) {
			if (((sh3.m_fpscr & DN) == 1) || (abs == 0x00000000)) {
				if (sign_of(n) == 0) {
					zero(n, 0);
					return(SH4_FPU_PZERO);
				} else {
					zero(n, 1);
					return(SH4_FPU_NZERO);
				}
			} else
				return(SH4_FPU_DENORM);
		} else
			if (abs < 0x7f800000)
				return(SH4_FPU_NORM);
			else
				if (abs == 0x7f800000) {
					if (sign_of(n) == 0)
						return(SH4_FPU_PINF);
					else
						return(SH4_FPU_NINF);
				} else
					if (abs < 0x7fc00000)
						return(SH4_FPU_qNaN);
					else
						return(SH4_FPU_sNaN);
	} else {
		if (abs < 0x00100000) {
			if (((sh3.m_fpscr & DN) == 1) || ((abs == 0x00000000) && (sh3.m_fr[n+1] == 0x00000000))) {
				if(sign_of(n) == 0) {
					zero(n, 0);
					return(SH4_FPU_PZERO);
				} else {
					zero(n, 1);
					return(SH4_FPU_NZERO);
				}
			} else
				return(SH4_FPU_DENORM);
		} else
			if (abs < 0x7ff00000)
				return(SH4_FPU_NORM);
			else
				if ((abs == 0x7ff00000) && (sh3.m_fr[n+1] == 0x00000000)) {
					if (sign_of(n) == 0)
						return(SH4_FPU_PINF);
					else
						return(SH4_FPU_NINF);
				} else
					if (abs < 0x7ff80000)
						return(SH4_FPU_qNaN);
					else
						return(SH4_FPU_sNaN);
	}
	return(SH4_FPU_NORM);
}
#endif

static UINT8 RB(offs_t A)
{
	if (A >= 0xe0000000)
		return program_read_byte(A);

	return program_read_byte(A & AM);
}

static UINT16 RW(offs_t A)
{
	if (A >= 0xe0000000)
		return program_read_word(A);

	return program_read_word(A & AM);
}

static UINT32 RL(offs_t A)
{
	if (A >= 0xe0000000)
		return program_read_dword(A);

	return program_read_dword(A & AM);
}

static void WB(offs_t A, UINT8 V)
{
	if (A >= 0xe0000000)
	{
		program_write_byte(A,V);
		return;
	}

	program_write_byte(A & AM,V);
}

static void WW(offs_t A, UINT16 V)
{
	if (A >= 0xe0000000)
	{
		program_write_word(A,V);
		return;
	}

	program_write_word(A & AM,V);
}

static void WL(offs_t A, UINT32 V)
{
	if (A >= 0xe0000000)
	{
		program_write_dword(A,V);
		return;
	}

	program_write_dword(A & AM,V);
}

static void ADD(const UINT16 opcode)
{
	sh3.m_r[Rn] += sh3.m_r[Rm];
}

static void ADDI(const UINT16 opcode)
{
	sh3.m_r[Rn] += (INT32)(INT16)(INT8)(opcode&0xff);
}

static void ADDC(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;
	UINT32 tmp0, tmp1;

	tmp1 = sh3.m_r[n] + sh3.m_r[m];
	tmp0 = sh3.m_r[n];
	sh3.m_r[n] = tmp1 + (sh3.m_sr & T);
	if (tmp0 > tmp1)
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
	if (tmp1 > sh3.m_r[n])
		sh3.m_sr |= T;
}

static void ADDV(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;
	INT32 dest, src, ans;

	if ((INT32) sh3.m_r[n] >= 0)
		dest = 0;
	else
		dest = 1;
	if ((INT32) sh3.m_r[m] >= 0)
		src = 0;
	else
		src = 1;
	src += dest;
	sh3.m_r[n] += sh3.m_r[m];
	if ((INT32) sh3.m_r[n] >= 0)
		ans = 0;
	else
		ans = 1;
	ans += dest;
	if (src == 0 || src == 2)
	{
		if (ans == 1)
			sh3.m_sr |= T;
		else
			sh3.m_sr &= ~T;
	}
	else
		sh3.m_sr &= ~T;
}

static void AND(const UINT16 opcode)
{
	sh3.m_r[Rn] &= sh3.m_r[Rm];
}

static void ANDI(const UINT16 opcode)
{
	sh3.m_r[0] &= (opcode&0xff);
}

static void ANDM(const UINT16 opcode)
{
	UINT32 temp;

	sh3.m_ea = sh3.m_gbr + sh3.m_r[0];
	temp = (opcode&0xff) & RB( sh3.m_ea );
	WB(sh3.m_ea, temp );
	sh3.m_sh4_icount -= 2;
}

static void BF(const UINT16 opcode)
{
	if ((sh3.m_sr & T) == 0)
	{
		INT32 disp = ((INT32)(opcode&0xff) << 24) >> 24;
		sh3.m_pc = sh3.m_ea = sh3.m_pc + disp * 2 + 2;
		sh3.m_sh4_icount -= 2;
	}
}

static void BFS(const UINT16 opcode)
{
	if ((sh3.m_sr & T) == 0)
	{
		INT32 disp = ((INT32)(opcode&0xff) << 24) >> 24;
		sh3.m_delay = sh3.m_pc;
		sh3.m_pc = sh3.m_ea = sh3.m_pc + disp * 2 + 2;
		sh3.m_sh4_icount--;
	}
}

static void BRA(const UINT16 opcode)
{
	INT32 disp = ((INT32)(opcode&0xfff) << 20) >> 20;

#if BUSY_LOOP_HACKS
	if (disp == -2)
	{
		UINT32 next_opcode = RW(sh3.m_ppc & AM);

		if (next_opcode == 0x0009)
			sh3.m_sh4_icount %= 3;
	}
#endif
	sh3.m_delay = sh3.m_pc;
	sh3.m_pc = sh3.m_ea = sh3.m_pc + disp * 2 + 2;
	sh3.m_sh4_icount--;
}

static void BRAF(const UINT16 opcode)
{
	sh3.m_delay = sh3.m_pc;
	sh3.m_pc += sh3.m_r[Rn] + 2;
	sh3.m_sh4_icount--;
}

static void BSR(const UINT16 opcode)
{
	INT32 disp = ((INT32)(opcode&0xfff) << 20) >> 20;

	sh3.m_pr = sh3.m_pc + 2;
	sh3.m_delay = sh3.m_pc;
	sh3.m_pc = sh3.m_ea = sh3.m_pc + disp * 2 + 2;
	sh3.m_sh4_icount--;
}

static void BSRF(const UINT16 opcode)
{
	sh3.m_pr = sh3.m_pc + 2;
	sh3.m_delay = sh3.m_pc;
	sh3.m_pc += sh3.m_r[Rn] + 2;
	sh3.m_sh4_icount--;
}

static void BT(const UINT16 opcode)
{
	if ((sh3.m_sr & T) != 0)
	{
		INT32 disp = ((INT32)(opcode&0xff) << 24) >> 24;
		sh3.m_pc = sh3.m_ea = sh3.m_pc + disp * 2 + 2;
		sh3.m_sh4_icount -= 2;
	}
}

static void BTS(const UINT16 opcode)
{
	if ((sh3.m_sr & T) != 0)
	{
		INT32 disp = ((INT32)(opcode&0xff) << 24) >> 24;
		sh3.m_delay = sh3.m_pc;
		sh3.m_pc = sh3.m_ea = sh3.m_pc + disp * 2 + 2;
		sh3.m_sh4_icount--;
	}
}

static void CLRMAC(const UINT16 opcode)
{
	sh3.m_mach = 0;
	sh3.m_macl = 0;
}

static void CLRT(const UINT16 opcode)
{
	sh3.m_sr &= ~T;
}

static void CMPEQ(const UINT16 opcode)
{
	if (sh3.m_r[Rn] == sh3.m_r[Rm])
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
}

static void CMPGE(const UINT16 opcode)
{
	if ((INT32) sh3.m_r[Rn] >= (INT32) sh3.m_r[Rm])
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
}

static void CMPGT(const UINT16 opcode)
{
	if ((INT32) sh3.m_r[Rn] > (INT32) sh3.m_r[Rm])
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
}

static void CMPHI(const UINT16 opcode)
{
	if ((UINT32) sh3.m_r[Rn] > (UINT32) sh3.m_r[Rm])
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
}

static void CMPHS(const UINT16 opcode)
{
	if ((UINT32) sh3.m_r[Rn] >= (UINT32) sh3.m_r[Rm])
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
}

static void CMPPL(const UINT16 opcode)
{
	if ((INT32) sh3.m_r[Rn] > 0)
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
}

static void CMPPZ(const UINT16 opcode)
{
	if ((INT32) sh3.m_r[Rn] >= 0)
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
}

static void CMPSTR(const UINT16 opcode)
{
	UINT32 temp;
	INT32 HH, HL, LH, LL;
	temp = sh3.m_r[Rn] ^ sh3.m_r[Rm];
	HH = (temp >> 24) & 0xff;
	HL = (temp >> 16) & 0xff;
	LH = (temp >> 8) & 0xff;
	LL = temp & 0xff;
	if (HH && HL && LH && LL)
	sh3.m_sr &= ~T;
	else
	sh3.m_sr |= T;
	}

static void CMPIM(const UINT16 opcode)
{
	UINT32 imm = (UINT32)(INT32)(INT16)(INT8)(opcode&0xff);

	if (sh3.m_r[0] == imm)
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
}

static void DIV0S(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if ((sh3.m_r[n] & 0x80000000) == 0)
		sh3.m_sr &= ~Q;
	else
		sh3.m_sr |= Q;
	if ((sh3.m_r[m] & 0x80000000) == 0)
		sh3.m_sr &= ~M;
	else
		sh3.m_sr |= M;
	if ((sh3.m_r[m] ^ sh3.m_r[n]) & 0x80000000)
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
}

static void DIV0U(const UINT16 opcode)
{
	sh3.m_sr &= ~(M | Q | T);
}

static void DIV1(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	UINT32 tmp0;
	UINT32 old_q;

	old_q = sh3.m_sr & Q;
	if (0x80000000 & sh3.m_r[n])
		sh3.m_sr |= Q;
	else
		sh3.m_sr &= ~Q;

	sh3.m_r[n] = (sh3.m_r[n] << 1) | (sh3.m_sr & T);

	if (!old_q)
	{
		if (!(sh3.m_sr & M))
		{
			tmp0 = sh3.m_r[n];
			sh3.m_r[n] -= sh3.m_r[m];
			if(!(sh3.m_sr & Q))
				if(sh3.m_r[n] > tmp0)
					sh3.m_sr |= Q;
				else
					sh3.m_sr &= ~Q;
			else
				if(sh3.m_r[n] > tmp0)
					sh3.m_sr &= ~Q;
				else
					sh3.m_sr |= Q;
		}
		else
		{
			tmp0 = sh3.m_r[n];
			sh3.m_r[n] += sh3.m_r[m];
			if(!(sh3.m_sr & Q))
			{
				if(sh3.m_r[n] < tmp0)
					sh3.m_sr &= ~Q;
				else
					sh3.m_sr |= Q;
			}
			else
			{
				if(sh3.m_r[n] < tmp0)
					sh3.m_sr |= Q;
				else
					sh3.m_sr &= ~Q;
			}
		}
	}
	else
	{
		if (!(sh3.m_sr & M))
		{
			tmp0 = sh3.m_r[n];
			sh3.m_r[n] += sh3.m_r[m];
			if(!(sh3.m_sr & Q))
				if(sh3.m_r[n] < tmp0)
					sh3.m_sr |= Q;
				else
					sh3.m_sr &= ~Q;
			else
				if(sh3.m_r[n] < tmp0)
					sh3.m_sr &= ~Q;
				else
					sh3.m_sr |= Q;
		}
		else
		{
			tmp0 = sh3.m_r[n];
			sh3.m_r[n] -= sh3.m_r[m];
			if(!(sh3.m_sr & Q))
				if(sh3.m_r[n] > tmp0)
					sh3.m_sr &= ~Q;
				else
					sh3.m_sr |= Q;
			else
				if(sh3.m_r[n] > tmp0)
					sh3.m_sr |= Q;
				else
					sh3.m_sr &= ~Q;
		}
	}

	tmp0 = (sh3.m_sr & (Q | M));
	if((!tmp0) || (tmp0 == 0x300))
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
}

static void DMULS(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	UINT32 RnL, RnH, RmL, RmH, Res0, Res1, Res2;
	UINT32 temp0, temp1, temp2, temp3;
	INT32 tempm, tempn, fnLmL;

	tempn = (INT32) sh3.m_r[n];
	tempm = (INT32) sh3.m_r[m];
	if (tempn < 0)
		tempn = 0 - tempn;
	if (tempm < 0)
		tempm = 0 - tempm;
	if ((INT32) (sh3.m_r[n] ^ sh3.m_r[m]) < 0)
		fnLmL = -1;
	else
		fnLmL = 0;
	temp1 = (UINT32) tempn;
	temp2 = (UINT32) tempm;
	RnL = temp1 & 0x0000ffff;
	RnH = (temp1 >> 16) & 0x0000ffff;
	RmL = temp2 & 0x0000ffff;
	RmH = (temp2 >> 16) & 0x0000ffff;
	temp0 = RmL * RnL;
	temp1 = RmH * RnL;
	temp2 = RmL * RnH;
	temp3 = RmH * RnH;
	Res2 = 0;
	Res1 = temp1 + temp2;
	if (Res1 < temp1)
		Res2 += 0x00010000;
	temp1 = (Res1 << 16) & 0xffff0000;
	Res0 = temp0 + temp1;
	if (Res0 < temp0)
		Res2++;
	Res2 = Res2 + ((Res1 >> 16) & 0x0000ffff) + temp3;
	if (fnLmL < 0)
	{
		Res2 = ~Res2;
		if (Res0 == 0)
			Res2++;
		else
			Res0 = (~Res0) + 1;
	}
	sh3.m_mach = Res2;
	sh3.m_macl = Res0;
	sh3.m_sh4_icount--;
}

static void DMULU(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	UINT32 RnL, RnH, RmL, RmH, Res0, Res1, Res2;
	UINT32 temp0, temp1, temp2, temp3;

	RnL = sh3.m_r[n] & 0x0000ffff;
	RnH = (sh3.m_r[n] >> 16) & 0x0000ffff;
	RmL = sh3.m_r[m] & 0x0000ffff;
	RmH = (sh3.m_r[m] >> 16) & 0x0000ffff;
	temp0 = RmL * RnL;
	temp1 = RmH * RnL;
	temp2 = RmL * RnH;
	temp3 = RmH * RnH;
	Res2 = 0;
	Res1 = temp1 + temp2;
	if (Res1 < temp1)
		Res2 += 0x00010000;
	temp1 = (Res1 << 16) & 0xffff0000;
	Res0 = temp0 + temp1;
	if (Res0 < temp0)
		Res2++;
	Res2 = Res2 + ((Res1 >> 16) & 0x0000ffff) + temp3;
	sh3.m_mach = Res2;
	sh3.m_macl = Res0;
	sh3.m_sh4_icount--;
}

static void DT(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_r[n]--;
	if (sh3.m_r[n] == 0)
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
#if BUSY_LOOP_HACKS
	{
		UINT32 next_opcode = RW(sh3.m_ppc & AM);

		if (next_opcode == 0x8bfd)
		{
			while (sh3.m_r[n] > 1 && sh3.m_sh4_icount > 4)
			{
				sh3.m_r[n]--;
				sh3.m_sh4_icount -= 4;
			}
		}
	}
#endif
}

static void EXTSB(const UINT16 opcode)
{
	sh3.m_r[Rn] = ((INT32)sh3.m_r[Rm] << 24) >> 24;
}

static void EXTSW(const UINT16 opcode)
{
	sh3.m_r[Rn] = ((INT32)sh3.m_r[Rm] << 16) >> 16;
}

static void EXTUB(const UINT16 opcode)
{
	sh3.m_r[Rn] = sh3.m_r[Rm] & 0x000000ff;
}

static void EXTUW(const UINT16 opcode)
{
	sh3.m_r[Rn] = sh3.m_r[Rm] & 0x0000ffff;
}

static void JMP(const UINT16 opcode)
{
	sh3.m_delay = sh3.m_pc;
	sh3.m_pc = sh3.m_ea = sh3.m_r[Rn];
}

static void JSR(const UINT16 opcode)
{
	sh3.m_delay = sh3.m_pc;
	sh3.m_pr = sh3.m_pc + 2;
	sh3.m_pc = sh3.m_ea = sh3.m_r[Rn];
	sh3.m_sh4_icount--;
}

static void LDCSR(const UINT16 opcode)
{
	UINT32 reg;

	reg = sh3.m_r[Rn];
	if ((0) != 0)
		sh4_syncronize_register_bank((sh3.m_sr & sRB) >> 29);
	if ((sh3.m_r[Rn] & sRB) != (sh3.m_sr & sRB))
		sh4_change_register_bank(sh3.m_r[Rn] & sRB ? 1 : 0);
	sh3.m_sr = reg & FLAGS;
	sh4_exception_recompute();
}

static void LDCGBR(const UINT16 opcode)
{
	sh3.m_gbr = sh3.m_r[Rn];
}

static void LDCVBR(const UINT16 opcode)
{
	sh3.m_vbr = sh3.m_r[Rn];
}

static void LDCMSR(const UINT16 opcode)
{
	UINT32 old;

	old = sh3.m_sr;
	sh3.m_ea = sh3.m_r[Rn];
	sh3.m_sr = RL(sh3.m_ea ) & FLAGS;
	if ((0) != 0)
		sh4_syncronize_register_bank((old & sRB) >> 29);
	if ((old & sRB) != (sh3.m_sr & sRB))
		sh4_change_register_bank(sh3.m_sr & sRB ? 1 : 0);
	sh3.m_r[Rn] += 4;
	sh3.m_sh4_icount -= 2;
	sh4_exception_recompute();
}

static void LDCMGBR(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rn];
	sh3.m_gbr = RL(sh3.m_ea );
	sh3.m_r[Rn] += 4;
	sh3.m_sh4_icount -= 2;
}

static void LDCMVBR(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rn];
	sh3.m_vbr = RL(sh3.m_ea );
	sh3.m_r[Rn] += 4;
	sh3.m_sh4_icount -= 2;
}

static void LDSMACH(const UINT16 opcode)
{
	sh3.m_mach = sh3.m_r[Rn];
}

static void LDSMACL(const UINT16 opcode)
{
	sh3.m_macl = sh3.m_r[Rn];
}

static void LDSPR(const UINT16 opcode)
{
	sh3.m_pr = sh3.m_r[Rn];
}

static void LDSMMACH(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rn];
	sh3.m_mach = RL(sh3.m_ea );
	sh3.m_r[Rn] += 4;
}

static void LDSMMACL(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rn];
	sh3.m_macl = RL(sh3.m_ea );
	sh3.m_r[Rn] += 4;
}

static void LDSMPR(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rn];
	sh3.m_pr = RL(sh3.m_ea );
	sh3.m_r[Rn] += 4;
}

static void MAC_L(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	UINT32 RnL, RnH, RmL, RmH, Res0, Res1, Res2;
	UINT32 temp0, temp1, temp2, temp3;
	INT32 tempm, tempn, fnLmL;

	tempn = (INT32) RL(sh3.m_r[n] );
	sh3.m_r[n] += 4;
	tempm = (INT32) RL(sh3.m_r[m] );
	sh3.m_r[m] += 4;
	if ((INT32) (tempn ^ tempm) < 0)
		fnLmL = -1;
	else
		fnLmL = 0;
	if (tempn < 0)
		tempn = 0 - tempn;
	if (tempm < 0)
		tempm = 0 - tempm;
	temp1 = (UINT32) tempn;
	temp2 = (UINT32) tempm;
	RnL = temp1 & 0x0000ffff;
	RnH = (temp1 >> 16) & 0x0000ffff;
	RmL = temp2 & 0x0000ffff;
	RmH = (temp2 >> 16) & 0x0000ffff;
	temp0 = RmL * RnL;
	temp1 = RmH * RnL;
	temp2 = RmL * RnH;
	temp3 = RmH * RnH;
	Res2 = 0;
	Res1 = temp1 + temp2;
	if (Res1 < temp1)
		Res2 += 0x00010000;
	temp1 = (Res1 << 16) & 0xffff0000;
	Res0 = temp0 + temp1;
	if (Res0 < temp0)
		Res2++;
	Res2 = Res2 + ((Res1 >> 16) & 0x0000ffff) + temp3;
	if (fnLmL < 0)
	{
		Res2 = ~Res2;
		if (Res0 == 0)
			Res2++;
		else
			Res0 = (~Res0) + 1;
	}
	if (sh3.m_sr & S)
	{
		Res0 = sh3.m_macl + Res0;
		if (sh3.m_macl > Res0)
			Res2++;
		Res2 += (sh3.m_mach & 0x0000ffff);
		if (((INT32) Res2 < 0) && (Res2 < 0xffff8000))
		{
			Res2 = 0x00008000;
			Res0 = 0x00000000;
		}
		else if (((INT32) Res2 > 0) && (Res2 > 0x00007fff))
		{
			Res2 = 0x00007fff;
			Res0 = 0xffffffff;
		}
		sh3.m_mach = Res2;
		sh3.m_macl = Res0;
	}
	else
	{
		Res0 = sh3.m_macl + Res0;
		if (sh3.m_macl > Res0)
			Res2++;
		Res2 += sh3.m_mach;
		sh3.m_mach = Res2;
		sh3.m_macl = Res0;
	}
	sh3.m_sh4_icount -= 2;
}

static void MAC_W(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	INT32 tempm, tempn, dest, src, ans;
	UINT32 templ;

	tempn = (INT32) RW(sh3.m_r[n] );
	sh3.m_r[n] += 2;
	tempm = (INT32) RW(sh3.m_r[m] );
	sh3.m_r[m] += 2;
	templ = sh3.m_macl;
	tempm = ((INT32) (short) tempn * (INT32) (short) tempm);
	if ((INT32) sh3.m_macl >= 0)
		dest = 0;
	else
		dest = 1;
	if ((INT32) tempm >= 0)
	{
		src = 0;
		tempn = 0;
	}
	else
	{
		src = 1;
		tempn = 0xffffffff;
	}
	src += dest;
	sh3.m_macl += tempm;
	if ((INT32) sh3.m_macl >= 0)
		ans = 0;
	else
		ans = 1;
	ans += dest;
	if (sh3.m_sr & S)
	{
		if (ans == 1)
			{
				if (src == 0)
					sh3.m_macl = 0x7fffffff;
				if (src == 2)
					sh3.m_macl = 0x80000000;
			}
	}
	else
	{
		sh3.m_mach += tempn;
		if (templ > sh3.m_macl)
			sh3.m_mach += 1;
	}
	sh3.m_sh4_icount -= 2;
}

static void MOV(const UINT16 opcode)
{
	sh3.m_r[Rn] = sh3.m_r[Rm];
}

static void MOVBS(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rn];
	WB(sh3.m_ea, sh3.m_r[Rm] & 0x000000ff);
}

static void MOVWS(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rn];
	WW(sh3.m_ea, sh3.m_r[Rm] & 0x0000ffff);
}

static void MOVLS(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rn];
	WL(sh3.m_ea, sh3.m_r[Rm] );
}

static void MOVBL(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rm];
	sh3.m_r[Rn] = (UINT32)(INT32)(INT16)(INT8) RB( sh3.m_ea );
}

static void MOVWL(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rm];
	sh3.m_r[Rn] = (UINT32)(INT32)(INT16) RW(sh3.m_ea );
}

static void MOVLL(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rm];
	sh3.m_r[Rn] = RL(sh3.m_ea );
}

static void MOVBM(const UINT16 opcode)
{
	UINT32 data = sh3.m_r[Rm] & 0x000000ff;

	sh3.m_r[Rn] -= 1;
	WB(sh3.m_r[Rn], data );
}

static void MOVWM(const UINT16 opcode)
{
	UINT32 data = sh3.m_r[Rm] & 0x0000ffff;

	sh3.m_r[Rn] -= 2;
	WW(sh3.m_r[Rn], data );
}

static void MOVLM(const UINT16 opcode)
{
	UINT32 data = sh3.m_r[Rm];

	sh3.m_r[Rn] -= 4;
	WL(sh3.m_r[Rn], data );
}

static void MOVBP(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	sh3.m_r[n] = (UINT32)(INT32)(INT16)(INT8) RB( sh3.m_r[m] );
	if (n != m)
		sh3.m_r[m] += 1;
}

static void MOVWP(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	sh3.m_r[n] = (UINT32)(INT32)(INT16) RW(sh3.m_r[m] );
	if (n != m)
		sh3.m_r[m] += 2;
}

static void MOVLP(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	sh3.m_r[n] = RL(sh3.m_r[m] );
	if (n != m)
		sh3.m_r[m] += 4;
}

static void MOVBS0(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rn] + sh3.m_r[0];
	WB(sh3.m_ea, sh3.m_r[Rm] & 0x000000ff );
}

static void MOVWS0(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rn] + sh3.m_r[0];
	WW(sh3.m_ea, sh3.m_r[Rm] & 0x0000ffff );
}

static void MOVLS0(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rn] + sh3.m_r[0];
	WL(sh3.m_ea, sh3.m_r[Rm] );
}

static void MOVBL0(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rm] + sh3.m_r[0];
	sh3.m_r[Rn] = (UINT32)(INT32)(INT16)(INT8) RB( sh3.m_ea );
}

static void MOVWL0(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rm] + sh3.m_r[0];
	sh3.m_r[Rn] = (UINT32)(INT32)(INT16) RW(sh3.m_ea );
}

static void MOVLL0(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rm] + sh3.m_r[0];
	sh3.m_r[Rn] = RL(sh3.m_ea );
}

static void MOVI(const UINT16 opcode)
{
	sh3.m_r[Rn] = (UINT32)(INT32)(INT16)(INT8)(opcode&0xff);
}

static void MOVWI(const UINT16 opcode)
{
	UINT32 disp = opcode & 0xff;
	sh3.m_ea = sh3.m_pc + disp * 2 + 2;
	sh3.m_r[Rn] = (UINT32)(INT32)(INT16) RW(sh3.m_ea );
}

static void MOVLI(const UINT16 opcode)
{
	UINT32 disp = opcode & 0xff;
	sh3.m_ea = ((sh3.m_pc + 2) & ~3) + disp * 4;
	sh3.m_r[Rn] = RL(sh3.m_ea );
}

static void MOVBLG(const UINT16 opcode)
{
	UINT32 disp = opcode & 0xff;
	sh3.m_ea = sh3.m_gbr + disp;
	sh3.m_r[0] = (UINT32)(INT32)(INT16)(INT8) RB( sh3.m_ea );
}

static void MOVWLG(const UINT16 opcode)
{
	UINT32 disp = opcode & 0xff;
	sh3.m_ea = sh3.m_gbr + disp * 2;
	sh3.m_r[0] = (INT32)(INT16) RW(sh3.m_ea );
}

static void MOVLLG(const UINT16 opcode)
{
	UINT32 disp = opcode & 0xff;
	sh3.m_ea = sh3.m_gbr + disp * 4;
	sh3.m_r[0] = RL(sh3.m_ea );
}

static void MOVBSG(const UINT16 opcode)
{
	UINT32 disp = opcode & 0xff;
	sh3.m_ea = sh3.m_gbr + disp;
	WB(sh3.m_ea, sh3.m_r[0] & 0x000000ff );
}

static void MOVWSG(const UINT16 opcode)
{
	UINT32 disp = opcode & 0xff;
	sh3.m_ea = sh3.m_gbr + disp * 2;
	WW(sh3.m_ea, sh3.m_r[0] & 0x0000ffff );
}

static void MOVLSG(const UINT16 opcode)
{
	UINT32 disp = opcode & 0xff;
	sh3.m_ea = sh3.m_gbr + disp * 4;
	WL(sh3.m_ea, sh3.m_r[0] );
}

static void MOVBS4(const UINT16 opcode)
{
	UINT32 disp = opcode & 0x0f;
	sh3.m_ea = sh3.m_r[Rm] + disp;
	WB(sh3.m_ea, sh3.m_r[0] & 0x000000ff );
}

static void MOVWS4(const UINT16 opcode)
{
	UINT32 disp = opcode & 0x0f;
	sh3.m_ea = sh3.m_r[Rm] + disp * 2;
	WW(sh3.m_ea, sh3.m_r[0] & 0x0000ffff );
}

static void MOVLS4(const UINT16 opcode)
{
	UINT32 disp = opcode & 0x0f;
	sh3.m_ea = sh3.m_r[Rn] + disp * 4;
	WL(sh3.m_ea, sh3.m_r[Rm] );
}

static void MOVBL4(const UINT16 opcode)
{
	UINT32 disp = opcode & 0x0f;
	sh3.m_ea = sh3.m_r[Rm] + disp;
	sh3.m_r[0] = (UINT32)(INT32)(INT16)(INT8) RB( sh3.m_ea );
}

static void MOVWL4(const UINT16 opcode)
{
	UINT32 disp = opcode & 0x0f;
	sh3.m_ea = sh3.m_r[Rm] + disp * 2;
	sh3.m_r[0] = (UINT32)(INT32)(INT16) RW(sh3.m_ea );
}

static void MOVLL4(const UINT16 opcode)
{
	UINT32 disp = opcode & 0x0f;
	sh3.m_ea = sh3.m_r[Rm] + disp * 4;
	sh3.m_r[Rn] = RL(sh3.m_ea );
}

static void MOVA(const UINT16 opcode)
{
	UINT32 disp = opcode & 0xff;
	sh3.m_ea = ((sh3.m_pc + 2) & ~3) + disp * 4;
	sh3.m_r[0] = sh3.m_ea;
}

static void MOVT(const UINT16 opcode)
{
	sh3.m_r[Rn] = sh3.m_sr & T;
}

static void MULL(const UINT16 opcode)
{
	sh3.m_macl = sh3.m_r[Rn] * sh3.m_r[Rm];
	sh3.m_sh4_icount--;
}

static void MULS(const UINT16 opcode)
{
	sh3.m_macl = (INT16) sh3.m_r[Rn] * (INT16) sh3.m_r[Rm];
}

static void MULU(const UINT16 opcode)
{
	sh3.m_macl = (UINT16) sh3.m_r[Rn] * (UINT16) sh3.m_r[Rm];
}

static void NEG(const UINT16 opcode)
{
	sh3.m_r[Rn] = 0 - sh3.m_r[Rm];
}

static void NEGC(const UINT16 opcode)
{
	UINT32 temp;

	temp = sh3.m_r[Rm];
	sh3.m_r[Rn] = -temp - (sh3.m_sr & T);
	if (temp || (sh3.m_sr & T))
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
}

static void NOP(const UINT16 opcode)
{
}

static void NOT(const UINT16 opcode)
{
	sh3.m_r[Rn] = ~sh3.m_r[Rm];
}

static void OR(const UINT16 opcode)
{
	sh3.m_r[Rn] |= sh3.m_r[Rm];
}

static void ORI(const UINT16 opcode)
{
	sh3.m_r[0] |= (opcode&0xff);
	sh3.m_sh4_icount -= 2;
}

static void ORM(const UINT16 opcode)
{
	UINT32 temp;

	sh3.m_ea = sh3.m_gbr + sh3.m_r[0];
	temp = RB( sh3.m_ea );
	temp |= (opcode&0xff);
	WB(sh3.m_ea, temp );
}

static void ROTCL(const UINT16 opcode)
{
	UINT32 n = Rn;

	UINT32 temp;

	temp = (sh3.m_r[n] >> 31) & T;
	sh3.m_r[n] = (sh3.m_r[n] << 1) | (sh3.m_sr & T);
	sh3.m_sr = (sh3.m_sr & ~T) | temp;
}

static void ROTCR(const UINT16 opcode)
{
	UINT32 n = Rn;

	UINT32 temp;
	temp = (sh3.m_sr & T) << 31;
	if (sh3.m_r[n] & T)
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
	sh3.m_r[n] = (sh3.m_r[n] >> 1) | temp;
}

static void ROTL(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_sr = (sh3.m_sr & ~T) | ((sh3.m_r[n] >> 31) & T);
	sh3.m_r[n] = (sh3.m_r[n] << 1) | (sh3.m_r[n] >> 31);
}

static void ROTR(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_sr = (sh3.m_sr & ~T) | (sh3.m_r[n] & T);
	sh3.m_r[n] = (sh3.m_r[n] >> 1) | (sh3.m_r[n] << 31);
}

static void RTE(const UINT16 opcode)
{
	sh3.m_delay = sh3.m_pc;
	sh3.m_pc = sh3.m_ea = sh3.m_spc;
	if ((0) != 0)
		sh4_syncronize_register_bank((sh3.m_sr & sRB) >> 29);
	if ((sh3.m_ssr & sRB) != (sh3.m_sr & sRB))
		sh4_change_register_bank(sh3.m_ssr & sRB ? 1 : 0);
	sh3.m_sr = sh3.m_ssr;
	sh3.m_sh4_icount--;
	sh4_exception_recompute();
}

static void RTS(const UINT16 opcode)
{
	sh3.m_delay = sh3.m_pc;
	sh3.m_pc = sh3.m_ea = sh3.m_pr;
	sh3.m_sh4_icount--;
}

static void SETT(const UINT16 opcode)
{
	sh3.m_sr |= T;
}

static void SHAL(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_sr = (sh3.m_sr & ~T) | ((sh3.m_r[n] >> 31) & T);
	sh3.m_r[n] <<= 1;
}

static void SHAR(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_sr = (sh3.m_sr & ~T) | (sh3.m_r[n] & T);
	sh3.m_r[n] = (UINT32)((INT32)sh3.m_r[n] >> 1);
}

static void SHLL(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_sr = (sh3.m_sr & ~T) | ((sh3.m_r[n] >> 31) & T);
	sh3.m_r[n] <<= 1;
}

static void SHLL2(const UINT16 opcode)
{
	sh3.m_r[Rn] <<= 2;
}

static void SHLL8(const UINT16 opcode)
{
	sh3.m_r[Rn] <<= 8;
}

static void SHLL16(const UINT16 opcode)
{
	sh3.m_r[Rn] <<= 16;
}

static void SHLR(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_sr = (sh3.m_sr & ~T) | (sh3.m_r[n] & T);
	sh3.m_r[n] >>= 1;
}

static void SHLR2(const UINT16 opcode)
{
	sh3.m_r[Rn] >>= 2;
}

static void SHLR8(const UINT16 opcode)
{
	sh3.m_r[Rn] >>= 8;
}

static void SHLR16(const UINT16 opcode)
{
	sh3.m_r[Rn] >>= 16;
}

static void SLEEP(const UINT16 opcode)
{

	if(sh3.m_sleep_mode != 2)
		sh3.m_pc -= 2;
	sh3.m_sh4_icount -= 2;

	if(sh3.m_sleep_mode == 0)
		sh3.m_sleep_mode = 1;
	else if(sh3.m_sleep_mode == 2)
		sh3.m_sleep_mode = 0;
}

static void STCSR(const UINT16 opcode)
{
	sh3.m_r[Rn] = sh3.m_sr;
}

static void STCGBR(const UINT16 opcode)
{
	sh3.m_r[Rn] = sh3.m_gbr;
}

static void STCVBR(const UINT16 opcode)
{
	sh3.m_r[Rn] = sh3.m_vbr;
}

static void STCMSR(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_r[n] -= 4;
	sh3.m_ea = sh3.m_r[n];
	WL(sh3.m_ea, sh3.m_sr );
	sh3.m_sh4_icount--;
}

static void STCMGBR(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_r[n] -= 4;
	sh3.m_ea = sh3.m_r[n];
	WL(sh3.m_ea, sh3.m_gbr );
	sh3.m_sh4_icount--;
}

static void STCMVBR(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_r[n] -= 4;
	sh3.m_ea = sh3.m_r[n];
	WL(sh3.m_ea, sh3.m_vbr );
	sh3.m_sh4_icount--;
}

static void STSMACH(const UINT16 opcode)
{
	sh3.m_r[Rn] = sh3.m_mach;
}

static void STSMACL(const UINT16 opcode)
{
	sh3.m_r[Rn] = sh3.m_macl;
}

static void STSPR(const UINT16 opcode)
{
	sh3.m_r[Rn] = sh3.m_pr;
}

static void STSMMACH(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_r[n] -= 4;
	sh3.m_ea = sh3.m_r[n];
	WL(sh3.m_ea, sh3.m_mach );
}

static void STSMMACL(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_r[n] -= 4;
	sh3.m_ea = sh3.m_r[n];
	WL(sh3.m_ea, sh3.m_macl );
}

static void STSMPR(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_r[n] -= 4;
	sh3.m_ea = sh3.m_r[n];
	WL(sh3.m_ea, sh3.m_pr );
}

static void SUB(const UINT16 opcode)
{
	sh3.m_r[Rn] -= sh3.m_r[Rm];
}

static void SUBC(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	UINT32 tmp0, tmp1;

	tmp1 = sh3.m_r[n] - sh3.m_r[m];
	tmp0 = sh3.m_r[n];
	sh3.m_r[n] = tmp1 - (sh3.m_sr & T);
	if (tmp0 < tmp1)
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
	if (tmp1 < sh3.m_r[n])
		sh3.m_sr |= T;
}

static void SUBV(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	INT32 dest, src, ans;

	if ((INT32) sh3.m_r[n] >= 0)
		dest = 0;
	else
		dest = 1;
	if ((INT32) sh3.m_r[m] >= 0)
		src = 0;
	else
		src = 1;
	src += dest;
	sh3.m_r[n] -= sh3.m_r[m];
	if ((INT32) sh3.m_r[n] >= 0)
		ans = 0;
	else
		ans = 1;
	ans += dest;
	if (src == 1)
	{
		if (ans == 1)
			sh3.m_sr |= T;
		else
			sh3.m_sr &= ~T;
	}
	else
		sh3.m_sr &= ~T;
}

static void SWAPB(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	UINT32 temp0, temp1;

	temp0 = sh3.m_r[m] & 0xffff0000;
	temp1 = (sh3.m_r[m] & 0x000000ff) << 8;
	sh3.m_r[n] = (sh3.m_r[m] >> 8) & 0x000000ff;
	sh3.m_r[n] = sh3.m_r[n] | temp1 | temp0;
}

static void SWAPW(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	UINT32 temp;

	temp = (sh3.m_r[m] >> 16) & 0x0000ffff;
	sh3.m_r[n] = (sh3.m_r[m] << 16) | temp;
}

static void TAS(const UINT16 opcode)
{
	UINT32 n = Rn;

	UINT32 temp;
	sh3.m_ea = sh3.m_r[n];

	temp = RB( sh3.m_ea );
	if (temp == 0)
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
	temp |= 0x80;

	WB(sh3.m_ea, temp );
	sh3.m_sh4_icount -= 3;
}

static void TRAPA(const UINT16 opcode)
{
	UINT32 imm = opcode & 0xff;

	if (sh3.m_cpu_type == CPU_TYPE_SH4)
	{
		sh3.m_m[TRA] = imm << 2;
	}
	else
	{
		sh3.m_sh3internal_upper[SH3_TRA_ADDR] = imm << 2;
	}

	sh3.m_ssr = sh3.m_sr;
	sh3.m_spc = sh3.m_pc;
	sh3.m_sgr = sh3.m_r[15];

	sh3.m_sr |= MD;
	if ((0) != 0)
		sh4_syncronize_register_bank((sh3.m_sr & sRB) >> 29);
	if (!(sh3.m_sr & sRB))
		sh4_change_register_bank(1);
	sh3.m_sr |= sRB;
	sh3.m_sr |= BL;
	sh4_exception_recompute();

	if (sh3.m_cpu_type == CPU_TYPE_SH4)
	{
		sh3.m_m[EXPEVT] = 0x00000160;
	}
	else
	{
		sh3.m_sh3internal_upper[SH3_EXPEVT_ADDR] = 0x00000160;
	}

	sh3.m_pc = sh3.m_vbr + 0x00000100;

	sh3.m_sh4_icount -= 7;
}

static void TST(const UINT16 opcode)
{
	if ((sh3.m_r[Rn] & sh3.m_r[Rm]) == 0)
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
}

static void TSTI(const UINT16 opcode)
{
	UINT32 imm = opcode & 0xff;

	if ((imm & sh3.m_r[0]) == 0)
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
}

static void TSTM(const UINT16 opcode)
{
	UINT32 imm = opcode & 0xff;

	sh3.m_ea = sh3.m_gbr + sh3.m_r[0];
	if ((imm & RB( sh3.m_ea )) == 0)
		sh3.m_sr |= T;
	else
		sh3.m_sr &= ~T;
	sh3.m_sh4_icount -= 2;
}

static void XOR(const UINT16 opcode)
{
	sh3.m_r[Rn] ^= sh3.m_r[Rm];
}

static void XORI(const UINT16 opcode)
{
	UINT32 imm = opcode & 0xff;
	sh3.m_r[0] ^= imm;
}

static void XORM(const UINT16 opcode)
{
	UINT32 imm = opcode & 0xff;
	UINT32 temp;

	sh3.m_ea = sh3.m_gbr + sh3.m_r[0];
	temp = RB( sh3.m_ea );
	temp ^= imm;
	WB(sh3.m_ea, temp );
	sh3.m_sh4_icount -= 2;
}

static void XTRCT(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	UINT32 temp;

	temp = (sh3.m_r[m] << 16) & 0xffff0000;
	sh3.m_r[n] = (sh3.m_r[n] >> 16) & 0x0000ffff;
	sh3.m_r[n] |= temp;
}

static void STCSSR(const UINT16 opcode)
{
	sh3.m_r[Rn] = sh3.m_ssr;
}

static void STCSPC(const UINT16 opcode)
{
	sh3.m_r[Rn] = sh3.m_spc;
}

static void STCSGR(const UINT16 opcode)
{
	sh3.m_r[Rn] = sh3.m_sgr;
}

static void STSFPUL(const UINT16 opcode)
{
	sh3.m_r[Rn] = sh3.m_fpul;
}

static void STSFPSCR(const UINT16 opcode)
{
	sh3.m_r[Rn] = sh3.m_fpscr & 0x003FFFFF;
}

static void STCDBR(const UINT16 opcode)
{
	sh3.m_r[Rn] = sh3.m_dbr;
}

static void STCRBANK(const UINT16 opcode)
{
	UINT32 m = Rm;

	sh3.m_r[Rn] = sh3.m_rbnk[sh3.m_sr&sRB ? 0 : 1][m & 7];
}

static void STCMRBANK(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	sh3.m_r[n] -= 4;
	sh3.m_ea = sh3.m_r[n];
	WL(sh3.m_ea, sh3.m_rbnk[sh3.m_sr&sRB ? 0 : 1][m & 7]);
	sh3.m_sh4_icount--;
}

static void MOVCAL(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rn];
	WL(sh3.m_ea, sh3.m_r[0] );
}

static void CLRS(const UINT16 opcode)
{
	sh3.m_sr &= ~S;
}

static void SETS(const UINT16 opcode)
{
	sh3.m_sr |= S;
}

static void STCMSGR(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_r[n] -= 4;
	sh3.m_ea = sh3.m_r[n];
	WL(sh3.m_ea, sh3.m_sgr );
}

static void STSMFPUL(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_r[n] -= 4;
	sh3.m_ea = sh3.m_r[n];
	WL(sh3.m_ea, sh3.m_fpul );
}

static void STSMFPSCR(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_r[n] -= 4;
	sh3.m_ea = sh3.m_r[n];
	WL(sh3.m_ea, sh3.m_fpscr & 0x003FFFFF);
}

static void STCMDBR(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_r[n] -= 4;
	sh3.m_ea = sh3.m_r[n];
	WL(sh3.m_ea, sh3.m_dbr );
}

static void STCMSSR(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_r[n] -= 4;
	sh3.m_ea = sh3.m_r[n];
	WL(sh3.m_ea, sh3.m_ssr );
}

static void STCMSPC(const UINT16 opcode)
{
	UINT32 n = Rn;

	sh3.m_r[n] -= 4;
	sh3.m_ea = sh3.m_r[n];
	WL(sh3.m_ea, sh3.m_spc );
}

static void LDSMFPUL(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rn];
	sh3.m_fpul = RL(sh3.m_ea );
	sh3.m_r[Rn] += 4;
}

static void LDSMFPSCR(const UINT16 opcode)
{
	UINT32 s;

	s = sh3.m_fpscr;
	sh3.m_ea = sh3.m_r[Rn];
	sh3.m_fpscr = RL(sh3.m_ea );
	sh3.m_fpscr &= 0x003FFFFF;
	sh3.m_r[Rn] += 4;
	if ((s & FR) != (sh3.m_fpscr & FR))
		sh4_swap_fp_registers();
#ifdef LSB_FIRST
	if ((s & PR) != (sh3.m_fpscr & PR))
		sh4_swap_fp_couples();
#endif
	sh3.m_fpu_sz = (sh3.m_fpscr & SZ) ? 1 : 0;
	sh3.m_fpu_pr = (sh3.m_fpscr & PR) ? 1 : 0;
}

static void LDCMDBR(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rn];
	sh3.m_dbr = RL(sh3.m_ea );
	sh3.m_r[Rn] += 4;
}

static void LDCMRBANK(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	sh3.m_ea = sh3.m_r[n];
	sh3.m_rbnk[sh3.m_sr&sRB ? 0 : 1][m & 7] = RL(sh3.m_ea );
	sh3.m_r[n] += 4;
}

static void LDCMSSR(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rn];
	sh3.m_ssr = RL(sh3.m_ea );
	sh3.m_r[Rn] += 4;
}

static void LDCMSPC(const UINT16 opcode)
{
	sh3.m_ea = sh3.m_r[Rn];
	sh3.m_spc = RL(sh3.m_ea );
	sh3.m_r[Rn] += 4;
}

static void LDSFPUL(const UINT16 opcode)
{
	sh3.m_fpul = sh3.m_r[Rn];
}

static void LDSFPSCR(const UINT16 opcode)
{
	UINT32 s;

	s = sh3.m_fpscr;
	sh3.m_fpscr = sh3.m_r[Rn] & 0x003FFFFF;
	if ((s & FR) != (sh3.m_fpscr & FR))
		sh4_swap_fp_registers();
#ifdef LSB_FIRST
	if ((s & PR) != (sh3.m_fpscr & PR))
		sh4_swap_fp_couples();
#endif
	sh3.m_fpu_sz = (sh3.m_fpscr & SZ) ? 1 : 0;
	sh3.m_fpu_pr = (sh3.m_fpscr & PR) ? 1 : 0;
}

static void LDCDBR(const UINT16 opcode)
{
	sh3.m_dbr = sh3.m_r[Rn];
}

static void SHAD(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if ((sh3.m_r[m] & 0x80000000) == 0)
		sh3.m_r[n] = sh3.m_r[n] << (sh3.m_r[m] & 0x1F);
	else if ((sh3.m_r[m] & 0x1F) == 0) {
		if ((sh3.m_r[n] & 0x80000000) == 0)
			sh3.m_r[n] = 0;
		else
			sh3.m_r[n] = 0xFFFFFFFF;
	} else
		sh3.m_r[n]=(INT32)sh3.m_r[n] >> ((~sh3.m_r[m] & 0x1F)+1);
}

static void SHLD(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if ((sh3.m_r[m] & 0x80000000) == 0)
		sh3.m_r[n] = sh3.m_r[n] << (sh3.m_r[m] & 0x1F);
	else if ((sh3.m_r[m] & 0x1F) == 0)
		sh3.m_r[n] = 0;
	else
		sh3.m_r[n] = sh3.m_r[n] >> ((~sh3.m_r[m] & 0x1F)+1);
}

static void LDCRBANK(const UINT16 opcode)
{
	UINT32 m = Rm;

	sh3.m_rbnk[sh3.m_sr&sRB ? 0 : 1][m & 7] = sh3.m_r[Rn];
}

static void LDCSSR(const UINT16 opcode)
{
	sh3.m_ssr = sh3.m_r[Rn];
}

static void LDCSPC(const UINT16 opcode)
{
	sh3.m_spc = sh3.m_r[Rn];
}

static void PREFM(const UINT16 opcode)
{
	int a;
	UINT32 addr,dest,sq;

	addr = sh3.m_r[Rn];
	if ((addr >= 0xE0000000) && (addr <= 0xE3FFFFFF))
	{
		if (sh3.m_sh4_mmu_enabled)
		{
			addr = addr & 0xFFFFFFE0;
			dest = sh4_getsqremap(addr);

		}
		else
		{
			sq = (addr & 0x20) >> 5;
			dest = addr & 0x03FFFFE0;
			if (sq == 0)
			{
				if (sh3.m_cpu_type == CPU_TYPE_SH4)
				{
					dest |= (sh3.m_m[QACR0] & 0x1C) << 24;
				}
				else
				{
					fatalerror("sh3.m_cpu_type != CPU_TYPE_SH4 but access internal regs\n");
				}
			}
			else
			{
				if (sh3.m_cpu_type == CPU_TYPE_SH4)
				{
					dest |= (sh3.m_m[QACR1] & 0x1C) << 24;
				}
				else
				{
					fatalerror("sh3.m_cpu_type != CPU_TYPE_SH4 but access internal regs\n");
				}

			}
			addr = addr & 0xFFFFFFE0;
		}

		for (a = 0;a < 4;a++)
		{

			program_write_qword(dest, program_read_qword(addr));
			addr += 8;
			dest += 8;
		}
	}
}

static void FMOVMRIFR(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		n = n & 14;
		sh3.m_ea = sh3.m_r[m];
		sh3.m_r[m] += 8;
		sh3.m_xf[n+NATIVE_ENDIAN_VALUE_LE_BE(1,0)] = RL(sh3.m_ea );
		sh3.m_xf[n+NATIVE_ENDIAN_VALUE_LE_BE(0,1)] = RL(sh3.m_ea+4 );
	} else {
		if (sh3.m_fpu_sz) {
			if (n & 1) {
				n = n & 14;
				sh3.m_ea = sh3.m_r[m];
				sh3.m_xf[n] = RL(sh3.m_ea );
				sh3.m_r[m] += 4;
				sh3.m_xf[n+1] = RL(sh3.m_ea+4 );
				sh3.m_r[m] += 4;
			} else {
				sh3.m_ea = sh3.m_r[m];
				sh3.m_fr[n] = RL(sh3.m_ea );
				sh3.m_r[m] += 4;
				sh3.m_fr[n+1] = RL(sh3.m_ea+4 );
				sh3.m_r[m] += 4;
			}
		} else {
			sh3.m_ea = sh3.m_r[m];
			sh3.m_fr[n] = RL(sh3.m_ea );
			sh3.m_r[m] += 4;
		}
	}
}

static void FMOVFRMR(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		m= m & 14;
		sh3.m_ea = sh3.m_r[n];
		WL(sh3.m_ea,sh3.m_xf[m+NATIVE_ENDIAN_VALUE_LE_BE(1,0)] );
		WL(sh3.m_ea+4,sh3.m_xf[m+NATIVE_ENDIAN_VALUE_LE_BE(0,1)] );
	} else {
		if (sh3.m_fpu_sz) {
			if (m & 1) {
				m= m & 14;
				sh3.m_ea = sh3.m_r[n];
				WL(sh3.m_ea,sh3.m_xf[m] );
				WL(sh3.m_ea+4,sh3.m_xf[m+1] );
			} else {
				sh3.m_ea = sh3.m_r[n];
				WL(sh3.m_ea,sh3.m_fr[m] );
				WL(sh3.m_ea+4,sh3.m_fr[m+1] );
			}
		} else {
			sh3.m_ea = sh3.m_r[n];
			WL(sh3.m_ea,sh3.m_fr[m] );
		}
	}
}

static void FMOVFRMDR(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		m= m & 14;
		sh3.m_r[n] -= 8;
		sh3.m_ea = sh3.m_r[n];
		WL(sh3.m_ea,sh3.m_xf[m+NATIVE_ENDIAN_VALUE_LE_BE(1,0)] );
		WL(sh3.m_ea+4,sh3.m_xf[m+NATIVE_ENDIAN_VALUE_LE_BE(0,1)] );
	} else {
		if (sh3.m_fpu_sz) {
			if (m & 1) {
				m= m & 14;
				sh3.m_r[n] -= 8;
				sh3.m_ea = sh3.m_r[n];
				WL(sh3.m_ea,sh3.m_xf[m] );
				WL(sh3.m_ea+4,sh3.m_xf[m+1] );
			} else {
				sh3.m_r[n] -= 8;
				sh3.m_ea = sh3.m_r[n];
				WL(sh3.m_ea,sh3.m_fr[m] );
				WL(sh3.m_ea+4,sh3.m_fr[m+1] );
			}
		} else {
			sh3.m_r[n] -= 4;
			sh3.m_ea = sh3.m_r[n];
			WL(sh3.m_ea,sh3.m_fr[m] );
		}
	}
}

static void FMOVFRS0(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		m= m & 14;
		sh3.m_ea = sh3.m_r[0] + sh3.m_r[n];
		WL(sh3.m_ea,sh3.m_xf[m+NATIVE_ENDIAN_VALUE_LE_BE(1,0)] );
		WL(sh3.m_ea+4,sh3.m_xf[m+NATIVE_ENDIAN_VALUE_LE_BE(0,1)] );
	} else {
		if (sh3.m_fpu_sz) {
			if (m & 1) {
				m= m & 14;
				sh3.m_ea = sh3.m_r[0] + sh3.m_r[n];
				WL(sh3.m_ea,sh3.m_xf[m] );
				WL(sh3.m_ea+4,sh3.m_xf[m+1] );
			} else {
				sh3.m_ea = sh3.m_r[0] + sh3.m_r[n];
				WL(sh3.m_ea,sh3.m_fr[m] );
				WL(sh3.m_ea+4,sh3.m_fr[m+1] );
			}
		} else {
			sh3.m_ea = sh3.m_r[0] + sh3.m_r[n];
			WL(sh3.m_ea,sh3.m_fr[m] );
		}
	}
}

static void FMOVS0FR(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		n= n & 14;
		sh3.m_ea = sh3.m_r[0] + sh3.m_r[m];
		sh3.m_xf[n+NATIVE_ENDIAN_VALUE_LE_BE(1,0)] = RL(sh3.m_ea );
		sh3.m_xf[n+NATIVE_ENDIAN_VALUE_LE_BE(0,1)] = RL(sh3.m_ea+4 );
	} else {
		if (sh3.m_fpu_sz) {
			if (n & 1) {
				n= n & 14;
				sh3.m_ea = sh3.m_r[0] + sh3.m_r[m];
				sh3.m_xf[n] = RL(sh3.m_ea );
				sh3.m_xf[n+1] = RL(sh3.m_ea+4 );
			} else {
				sh3.m_ea = sh3.m_r[0] + sh3.m_r[m];
				sh3.m_fr[n] = RL(sh3.m_ea );
				sh3.m_fr[n+1] = RL(sh3.m_ea+4 );
			}
		} else {
			sh3.m_ea = sh3.m_r[0] + sh3.m_r[m];
			sh3.m_fr[n] = RL(sh3.m_ea );
		}
	}
}

static void FMOVMRFR(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		if (n & 1) {
			n= n & 14;
			sh3.m_ea = sh3.m_r[m];
			sh3.m_xf[n+NATIVE_ENDIAN_VALUE_LE_BE(1,0)] = RL(sh3.m_ea );
			sh3.m_xf[n+NATIVE_ENDIAN_VALUE_LE_BE(0,1)] = RL(sh3.m_ea+4 );
		} else {
			n= n & 14;
			sh3.m_ea = sh3.m_r[m];
			sh3.m_fr[n+NATIVE_ENDIAN_VALUE_LE_BE(1,0)] = RL(sh3.m_ea );
			sh3.m_fr[n+NATIVE_ENDIAN_VALUE_LE_BE(0,1)] = RL(sh3.m_ea+4 );
		}
	} else {
		if (sh3.m_fpu_sz) {
			if (n & 1) {
				n= n & 14;
				sh3.m_ea = sh3.m_r[m];
				sh3.m_xf[n] = RL(sh3.m_ea );
				sh3.m_xf[n+1] = RL(sh3.m_ea+4 );
			} else {
				n= n & 14;
				sh3.m_ea = sh3.m_r[m];
				sh3.m_fr[n] = RL(sh3.m_ea );
				sh3.m_fr[n+1] = RL(sh3.m_ea+4 );
			}
		} else {
			sh3.m_ea = sh3.m_r[m];
			sh3.m_fr[n] = RL(sh3.m_ea );
		}
	}
}

static void FMOVFR(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if ((sh3.m_fpu_sz == 0) && (sh3.m_fpu_pr == 0))
		sh3.m_fr[n] = sh3.m_fr[m];
	else {
		if (m & 1) {
			if (n & 1) {
				sh3.m_xf[n & 14] = sh3.m_xf[m & 14];
				sh3.m_xf[n | 1] = sh3.m_xf[m | 1];
			} else {
				sh3.m_fr[n] = sh3.m_xf[m & 14];
				sh3.m_fr[n | 1] = sh3.m_xf[m | 1];
			}
		} else {
			if (n & 1) {
				sh3.m_xf[n & 14] = sh3.m_fr[m];
				sh3.m_xf[n | 1] = sh3.m_fr[m | 1];
			} else {
				sh3.m_fr[n] = sh3.m_fr[m];
				sh3.m_fr[n | 1] = sh3.m_fr[m | 1];
			}
		}
	}
}

static void FLDI1(const UINT16 opcode)
{
	sh3.m_fr[Rn] = 0x3F800000;
}

static void FLDI0(const UINT16 opcode)
{
	sh3.m_fr[Rn] = 0;
}

static void  FLDS(const UINT16 opcode)
{
	sh3.m_fpul = sh3.m_fr[Rn];
}

static void  FSTS(const UINT16 opcode)
{
	sh3.m_fr[Rn] = sh3.m_fpul;
}

static void FRCHG()
{
	sh3.m_fpscr ^= FR;
	sh4_swap_fp_registers();
}

static void FSCHG()
{
	sh3.m_fpscr ^= SZ;
	sh3.m_fpu_sz = (sh3.m_fpscr & SZ) ? 1 : 0;
}

static void FTRC(const UINT16 opcode)
{
	UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		if(n & 1)
			fatalerror("SH-4: FTRC opcode used with n %d",n);

		n = n & 14;
		*((INT32 *)&sh3.m_fpul) = (INT32)FP_RFD(n);
	} else {

		*((INT32 *)&sh3.m_fpul) = (INT32)FP_RFS(n);
	}
}

static void FLOAT(const UINT16 opcode)
{
	UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		if(n & 1)
			fatalerror("SH-4: FLOAT opcode used with n %d",n);

		n = n & 14;
		FP_RFD(n) = (double)*((INT32 *)&sh3.m_fpul);
	} else {
		FP_RFS(n) = (float)*((INT32 *)&sh3.m_fpul);
	}
}

static void FNEG(const UINT16 opcode)
{
	UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		FP_RFD(n) = -FP_RFD(n);
	} else {
		FP_RFS(n) = -FP_RFS(n);
	}
}

static void FABS(const UINT16 opcode)
{
	UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
#ifdef LSB_FIRST
		n = n | 1;
		sh3.m_fr[n] = sh3.m_fr[n] & 0x7fffffff;
#else
		n = n & 14;
		sh3.m_fr[n] = sh3.m_fr[n] & 0x7fffffff;
#endif
	} else {
		sh3.m_fr[n] = sh3.m_fr[n] & 0x7fffffff;
	}
}

static void FCMP_EQ(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		n = n & 14;
		m = m & 14;
		if (FP_RFD(n) == FP_RFD(m))
			sh3.m_sr |= T;
		else
			sh3.m_sr &= ~T;
	} else {
		if (FP_RFS(n) == FP_RFS(m))
			sh3.m_sr |= T;
		else
			sh3.m_sr &= ~T;
	}
}

static void FCMP_GT(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		n = n & 14;
		m = m & 14;
		if (FP_RFD(n) > FP_RFD(m))
			sh3.m_sr |= T;
		else
			sh3.m_sr &= ~T;
	} else {
		if (FP_RFS(n) > FP_RFS(m))
			sh3.m_sr |= T;
		else
			sh3.m_sr &= ~T;
	}
}

static void FCNVDS(const UINT16 opcode)
{
	UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		n = n & 14;
		if (sh3.m_fpscr & RM)
			sh3.m_fr[n | NATIVE_ENDIAN_VALUE_LE_BE(0,1)] &= 0xe0000000;
		*((float *)&sh3.m_fpul) = (float)FP_RFD(n);
	}
}

static void FCNVSD(const UINT16 opcode)
{
	UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		n = n & 14;
		FP_RFD(n) = (double)*((float *)&sh3.m_fpul);
	}
}

static void FADD(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		n = n & 14;
		m = m & 14;
		FP_RFD(n) = FP_RFD(n) + FP_RFD(m);
	} else {
		FP_RFS(n) = FP_RFS(n) + FP_RFS(m);
	}
}

static void FSUB(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		n = n & 14;
		m = m & 14;
		FP_RFD(n) = FP_RFD(n) - FP_RFD(m);
	} else {
		FP_RFS(n) = FP_RFS(n) - FP_RFS(m);
	}
}

static void FMUL(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		n = n & 14;
		m = m & 14;
		FP_RFD(n) = FP_RFD(n) * FP_RFD(m);
	} else {
		FP_RFS(n) = FP_RFS(n) * FP_RFS(m);
	}
}

static void FDIV(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		n = n & 14;
		m = m & 14;
		if (FP_RFD(m) == 0)
			return;
		FP_RFD(n) = FP_RFD(n) / FP_RFD(m);
	} else {
		if (FP_RFS(m) == 0)
			return;
		FP_RFS(n) = FP_RFS(n) / FP_RFS(m);
	}
}

static void FMAC(const UINT16 opcode)
{
	UINT32 m = Rm; UINT32 n = Rn;

	if (sh3.m_fpu_pr == 0) {
		FP_RFS(n) = (FP_RFS(0) * FP_RFS(m)) + FP_RFS(n);
	}
}

static void FSQRT(const UINT16 opcode)
{
	UINT32 n = Rn;

	if (sh3.m_fpu_pr) {
		n = n & 14;
		if (FP_RFD(n) < 0)
			return;
		FP_RFD(n) = sqrtf(FP_RFD(n));
	} else {
		if (FP_RFS(n) < 0)
			return;
		FP_RFS(n) = sqrtf(FP_RFS(n));
	}
}

static void FSRRA(const UINT16 opcode)
{
	UINT32 n = Rn;

	if (FP_RFS(n) < 0)
		return;
	FP_RFS(n) = 1.0 / sqrtf(FP_RFS(n));
}

static void FSSCA(const UINT16 opcode)
{
	UINT32 n = Rn;

float angle;

	angle = (((float)(sh3.m_fpul & 0xFFFF)) / 65536.0) * 2.0 * M_PI;
	FP_RFS(n) = sinf(angle);
	FP_RFS(n+1) = cosf(angle);
}

static void FIPR(const UINT16 opcode)
{
	UINT32 n = Rn;

UINT32 m;
float ml[4];
int a;

	m = (n & 3) << 2;
	n = n & 12;
	for (a = 0;a < 4;a++)
		ml[a] = FP_RFS(n+a) * FP_RFS(m+a);
	FP_RFS(n+3) = ml[0] + ml[1] + ml[2] + ml[3];
}

static void FTRV(const UINT16 opcode)
{
	UINT32 n = Rn;

int i,j;
float sum[4];

	n = n & 12;
	for (i = 0;i < 4;i++) {
		sum[i] = 0;
		for (j=0;j < 4;j++)
			sum[i] += FP_XFS((j << 2) + i)*FP_RFS(n + j);
	}
	for (i = 0;i < 4;i++)
		FP_RFS(n + i) = sum[i];
}

static void op1111_0xf13(const UINT16 opcode)
{
	if (opcode & 0x100) {
			if (opcode & 0x200) {
				switch (opcode & 0xC00)
				{
					case 0x000:
						FSCHG();
						break;
					case 0x800:
						FRCHG();
						break;
					default:

						break;
				}
			} else {
				FTRV(opcode);
			}
		} else {
			FSSCA(opcode);
		}
}

static void dbreak(const UINT16 opcode)
{

}

static void op1111_0x13(UINT16 opcode)
{
	switch((opcode >> 4) & 0x0f)
	{
		case 0x00:  FSTS(opcode); break;
		case 0x01:  FLDS(opcode); break;
		case 0x02:  FLOAT(opcode); break;
		case 0x03:  FTRC(opcode); break;
		case 0x04:  FNEG(opcode); break;
		case 0x05:  FABS(opcode); break;
		case 0x06:  FSQRT(opcode); break;
		case 0x07:  FSRRA(opcode); break;
		case 0x08:  FLDI0(opcode); break;
		case 0x09:  FLDI1(opcode); break;
		case 0x0a:  FCNVSD(opcode); break;
		case 0x0b:  FCNVDS(opcode); break;
		case 0x0c:  dbreak(opcode); break;
		case 0x0d:  dbreak(opcode); break;
		case 0x0e:  FIPR(opcode); break;
		case 0x0f:  op1111_0xf13(opcode); break;
	}
}

static void device_reset()
{
	sh3.m_ppc = 0;
	sh3.m_spc = 0;
	sh3.m_pr = 0;
	sh3.m_sr = 0;
	sh3.m_ssr = 0;
	sh3.m_gbr = 0;
	sh3.m_vbr = 0;
	sh3.m_mach = 0;
	sh3.m_macl = 0;
	memset(sh3.m_r, 0, sizeof(sh3.m_r));
	memset(sh3.m_rbnk, 0, sizeof(sh3.m_rbnk));
	sh3.m_sgr = 0;
	memset(sh3.m_fr, 0, sizeof(sh3.m_fr));
	memset(sh3.m_xf, 0, sizeof(sh3.m_xf));
	sh3.m_ea = 0;
	sh3.m_delay = 0;
	sh3.m_cpu_off = 0;
	sh3.m_pending_irq = 0;
	sh3.m_test_irq = 0;
	memset(sh3.m_exception_priority, 0, sizeof(sh3.m_exception_priority));
	memset(sh3.m_exception_requesting, 0, sizeof(sh3.m_exception_requesting));
	memset(sh3.m_m, 0, sizeof(sh3.m_m));
	memset(sh3.m_sh3internal_upper, 0, sizeof(sh3.m_sh3internal_upper));
	memset(sh3.m_sh3internal_lower, 0, sizeof(sh3.m_sh3internal_lower));
	memset(sh3.m_irq_line_state, 0, sizeof(sh3.m_irq_line_state));
	sh3.m_SH4_TSTR = 0;
	sh3.m_SH4_TCNT0 = 0;
	sh3.m_SH4_TCNT1 = 0;
	sh3.m_SH4_TCNT2 = 0;
	sh3.m_SH4_TCR0 = 0;
	sh3.m_SH4_TCR1 = 0;
	sh3.m_SH4_TCR2 = 0;
	sh3.m_SH4_TCOR0 = 0;
	sh3.m_SH4_TCOR1 = 0;
	sh3.m_SH4_TCOR2 = 0;
	sh3.m_SH4_TOCR = 0;
	sh3.m_SH4_TCPR2 = 0;
	sh3.m_SH4_IPRA = 0;
	sh3.m_SH4_IPRC = 0;
	sh3.m_SH4_SAR0 = 0;
	sh3.m_SH4_SAR1 = 0;
	sh3.m_SH4_SAR2 = 0;
	sh3.m_SH4_SAR3 = 0;
	sh3.m_SH4_DAR0 = 0;
	sh3.m_SH4_DAR1 = 0;
	sh3.m_SH4_DAR2 = 0;
	sh3.m_SH4_DAR3 = 0;
	sh3.m_SH4_CHCR0 = 0;
	sh3.m_SH4_CHCR1 = 0;
	sh3.m_SH4_CHCR2 = 0;
	sh3.m_SH4_CHCR3 = 0;
	sh3.m_SH4_DMATCR0 = 0;
	sh3.m_SH4_DMATCR1 = 0;
	sh3.m_SH4_DMATCR2 = 0;
	sh3.m_SH4_DMATCR3 = 0;
	sh3.m_SH4_DMAOR = 0;
	sh3.m_nmi_line_state = 0;
	sh3.m_frt_input = 0;
	sh3.m_internal_irq_vector = 0;
	sh3.m_refresh_timer_base = 0;
	memset(sh3.m_dma_timer_active, 0, sizeof(sh3.m_dma_timer_active));
	memset(sh3.m_dma_source, 0, sizeof(sh3.m_dma_source));
	memset(sh3.m_dma_destination, 0, sizeof(sh3.m_dma_destination));
	memset(sh3.m_dma_count, 0, sizeof(sh3.m_dma_count));
	memset(sh3.m_dma_wordsize, 0, sizeof(sh3.m_dma_wordsize));
	memset(sh3.m_dma_source_increment, 0, sizeof(sh3.m_dma_source_increment));
	memset(sh3.m_dma_destination_increment, 0, sizeof(sh3.m_dma_destination_increment));
	memset(sh3.m_dma_mode, 0, sizeof(sh3.m_dma_mode));
	sh3.m_ioport16_pullup = 0;
	sh3.m_ioport16_direction = 0;
	sh3.m_ioport4_pullup = 0;
	sh3.m_ioport4_direction = 0;

	sh4_default_exception_priorities();

	timer_adjust(sh3.m_rtc_timer,TIME_NEVER,0,0);
 sh3.m_SH4_TCOR0=sh3.m_SH4_TCNT0=sh3.m_SH4_TCOR1=sh3.m_SH4_TCNT1=sh3.m_SH4_TCOR2=sh3.m_SH4_TCNT2=0xffffffff;

	sh3.m_pc = 0xa0000000;
	sh3.m_r[15] = RL(4);
	sh3.m_sr = 0x700000f0;
	sh3.m_fpscr = 0x00040001;
	sh3.m_fpu_sz = (sh3.m_fpscr & SZ) ? 1 : 0;
	sh3.m_fpu_pr = (sh3.m_fpscr & PR) ? 1 : 0;
	sh3.m_fpul = 0;
	sh3.m_dbr = 0;

	sh3.m_internal_irq_level = -1;
	sh3.m_irln = 15;
	sh3.m_sleep_mode = 0;

	sh3.m_sh4_mmu_enabled = 0;
}

static void execute_one_0000(const UINT16 opcode)
{
	switch(opcode & 0xff)
	{

		case 0x00:  NOP(opcode); break;
		case 0x10:  NOP(opcode); break;
		case 0x20:  NOP(opcode); break;
		case 0x30:  NOP(opcode); break;
		case 0x40:  NOP(opcode); break;
		case 0x50:  NOP(opcode); break;
		case 0x60:  NOP(opcode); break;
		case 0x70:  NOP(opcode); break;
		case 0x80:  NOP(opcode); break;
		case 0x90:  NOP(opcode); break;
		case 0xa0:  NOP(opcode); break;
		case 0xb0:  NOP(opcode); break;
		case 0xc0:  NOP(opcode); break;
		case 0xd0:  NOP(opcode); break;
		case 0xe0:  NOP(opcode); break;
		case 0xf0:  NOP(opcode); break;

		case 0x01:  NOP(opcode); break;
		case 0x11:  NOP(opcode); break;
		case 0x21:  NOP(opcode); break;
		case 0x31:  NOP(opcode); break;
		case 0x41:  NOP(opcode); break;
		case 0x51:  NOP(opcode); break;
		case 0x61:  NOP(opcode); break;
		case 0x71:  NOP(opcode); break;
		case 0x81:  NOP(opcode); break;
		case 0x91:  NOP(opcode); break;
		case 0xa1:  NOP(opcode); break;
		case 0xb1:  NOP(opcode); break;
		case 0xc1:  NOP(opcode); break;
		case 0xd1:  NOP(opcode); break;
		case 0xe1:  NOP(opcode); break;
		case 0xf1:  NOP(opcode); break;

		case 0x02:  STCSR(opcode); break;
		case 0x12:  STCGBR(opcode); break;
		case 0x22:  STCVBR(opcode); break;
		case 0x32:  STCSSR(opcode); break;
		case 0x42:  STCSPC(opcode); break;
		case 0x52:  NOP(opcode); break;
		case 0x62:  NOP(opcode); break;
		case 0x72:  NOP(opcode); break;
		case 0x82:  STCRBANK(opcode); break;
		case 0x92:  STCRBANK(opcode); break;
		case 0xa2:  STCRBANK(opcode); break;
		case 0xb2:  STCRBANK(opcode); break;
		case 0xc2:  STCRBANK(opcode); break;
		case 0xd2:  STCRBANK(opcode); break;
		case 0xe2:  STCRBANK(opcode); break;
		case 0xf2:  STCRBANK(opcode); break;

		case 0x03:  BSRF(opcode); break;
		case 0x13:  NOP(opcode); break;
		case 0x23:  BRAF(opcode); break;
		case 0x33:  NOP(opcode); break;
		case 0x43:  NOP(opcode); break;
		case 0x53:  NOP(opcode); break;
		case 0x63:  NOP(opcode); break;
		case 0x73:  NOP(opcode); break;
		case 0x83:  PREFM(opcode); break;
		case 0x93:  TODO(opcode); break;
		case 0xa3:  TODO(opcode); break;
		case 0xb3:  TODO(opcode); break;
		case 0xc3:  MOVCAL(opcode); break;
		case 0xd3:  NOP(opcode); break;
		case 0xe3:  NOP(opcode); break;
		case 0xf3:  NOP(opcode); break;

		case 0x04:  MOVBS0(opcode); break;
		case 0x14:  MOVBS0(opcode); break;
		case 0x24:  MOVBS0(opcode); break;
		case 0x34:  MOVBS0(opcode); break;
		case 0x44:  MOVBS0(opcode); break;
		case 0x54:  MOVBS0(opcode); break;
		case 0x64:  MOVBS0(opcode); break;
		case 0x74:  MOVBS0(opcode); break;
		case 0x84:  MOVBS0(opcode); break;
		case 0x94:  MOVBS0(opcode); break;
		case 0xa4:  MOVBS0(opcode); break;
		case 0xb4:  MOVBS0(opcode); break;
		case 0xc4:  MOVBS0(opcode); break;
		case 0xd4:  MOVBS0(opcode); break;
		case 0xe4:  MOVBS0(opcode); break;
		case 0xf4:  MOVBS0(opcode); break;

		case 0x05:  MOVWS0(opcode); break;
		case 0x15:  MOVWS0(opcode); break;
		case 0x25:  MOVWS0(opcode); break;
		case 0x35:  MOVWS0(opcode); break;
		case 0x45:  MOVWS0(opcode); break;
		case 0x55:  MOVWS0(opcode); break;
		case 0x65:  MOVWS0(opcode); break;
		case 0x75:  MOVWS0(opcode); break;
		case 0x85:  MOVWS0(opcode); break;
		case 0x95:  MOVWS0(opcode); break;
		case 0xa5:  MOVWS0(opcode); break;
		case 0xb5:  MOVWS0(opcode); break;
		case 0xc5:  MOVWS0(opcode); break;
		case 0xd5:  MOVWS0(opcode); break;
		case 0xe5:  MOVWS0(opcode); break;
		case 0xf5:  MOVWS0(opcode); break;

		case 0x06:  MOVLS0(opcode); break;
		case 0x16:  MOVLS0(opcode); break;
		case 0x26:  MOVLS0(opcode); break;
		case 0x36:  MOVLS0(opcode); break;
		case 0x46:  MOVLS0(opcode); break;
		case 0x56:  MOVLS0(opcode); break;
		case 0x66:  MOVLS0(opcode); break;
		case 0x76:  MOVLS0(opcode); break;
		case 0x86:  MOVLS0(opcode); break;
		case 0x96:  MOVLS0(opcode); break;
		case 0xa6:  MOVLS0(opcode); break;
		case 0xb6:  MOVLS0(opcode); break;
		case 0xc6:  MOVLS0(opcode); break;
		case 0xd6:  MOVLS0(opcode); break;
		case 0xe6:  MOVLS0(opcode); break;
		case 0xf6:  MOVLS0(opcode); break;

		case 0x07:  MULL(opcode); break;
		case 0x17:  MULL(opcode); break;
		case 0x27:  MULL(opcode); break;
		case 0x37:  MULL(opcode); break;
		case 0x47:  MULL(opcode); break;
		case 0x57:  MULL(opcode); break;
		case 0x67:  MULL(opcode); break;
		case 0x77:  MULL(opcode); break;
		case 0x87:  MULL(opcode); break;
		case 0x97:  MULL(opcode); break;
		case 0xa7:  MULL(opcode); break;
		case 0xb7:  MULL(opcode); break;
		case 0xc7:  MULL(opcode); break;
		case 0xd7:  MULL(opcode); break;
		case 0xe7:  MULL(opcode); break;
		case 0xf7:  MULL(opcode); break;

		case 0x08:  CLRT(opcode); break;
		case 0x18:  SETT(opcode); break;
		case 0x28:  CLRMAC(opcode); break;
		case 0x38:  TODO(opcode); break;
		case 0x48:  CLRS(opcode); break;
		case 0x58:  SETS(opcode); break;
		case 0x68:  NOP(opcode); break;
		case 0x78:  NOP(opcode); break;
		case 0x88:  CLRT(opcode); break;
		case 0x98:  SETT(opcode); break;
		case 0xa8:  CLRMAC(opcode); break;
		case 0xb8:  TODO(opcode); break;
		case 0xc8:  CLRS(opcode); break;
		case 0xd8:  SETS(opcode); break;
		case 0xe8:  NOP(opcode); break;
		case 0xf8:  NOP(opcode); break;

		case 0x09:  NOP(opcode); break;
		case 0x19:  DIV0U(opcode); break;
		case 0x29:  MOVT(opcode); break;
		case 0x39:  NOP(opcode); break;
		case 0x49:  NOP(opcode); break;
		case 0x59:  DIV0U(opcode); break;
		case 0x69:  MOVT(opcode); break;
		case 0x79:  NOP(opcode); break;
		case 0x89:  NOP(opcode); break;
		case 0x99:  DIV0U(opcode); break;
		case 0xa9:  MOVT(opcode); break;
		case 0xb9:  NOP(opcode); break;
		case 0xc9:  NOP(opcode); break;
		case 0xd9:  DIV0U(opcode); break;
		case 0xe9:  MOVT(opcode); break;
		case 0xf9:  NOP(opcode); break;

		case 0x0a:  STSMACH(opcode); break;
		case 0x1a:  STSMACL(opcode); break;
		case 0x2a:  STSPR(opcode); break;
		case 0x3a:  STCSGR(opcode); break;
		case 0x4a:  NOP(opcode); break;
		case 0x5a:  STSFPUL(opcode); break;
		case 0x6a:  STSFPSCR(opcode); break;
		case 0x7a:  STCDBR(opcode); break;
		case 0x8a:  STSMACH(opcode); break;
		case 0x9a:  STSMACL(opcode); break;
		case 0xaa:  STSPR(opcode); break;
		case 0xba:  STCSGR(opcode); break;
		case 0xca:  NOP(opcode); break;
		case 0xda:  STSFPUL(opcode); break;
		case 0xea:  STSFPSCR(opcode); break;
		case 0xfa:  STCDBR(opcode); break;

		case 0x0b:  RTS(opcode); break;
		case 0x1b:  SLEEP(opcode); break;
		case 0x2b:  RTE(opcode); break;
		case 0x3b:  NOP(opcode); break;
		case 0x4b:  RTS(opcode); break;
		case 0x5b:  SLEEP(opcode); break;
		case 0x6b:  RTE(opcode); break;
		case 0x7b:  NOP(opcode); break;
		case 0x8b:  RTS(opcode); break;
		case 0x9b:  SLEEP(opcode); break;
		case 0xab:  RTE(opcode); break;
		case 0xbb:  NOP(opcode); break;
		case 0xcb:  RTS(opcode); break;
		case 0xdb:  SLEEP(opcode); break;
		case 0xeb:  RTE(opcode); break;
		case 0xfb:  NOP(opcode); break;

		case 0x0c:  MOVBL0(opcode); break;
		case 0x1c:  MOVBL0(opcode); break;
		case 0x2c:  MOVBL0(opcode); break;
		case 0x3c:  MOVBL0(opcode); break;
		case 0x4c:  MOVBL0(opcode); break;
		case 0x5c:  MOVBL0(opcode); break;
		case 0x6c:  MOVBL0(opcode); break;
		case 0x7c:  MOVBL0(opcode); break;
		case 0x8c:  MOVBL0(opcode); break;
		case 0x9c:  MOVBL0(opcode); break;
		case 0xac:  MOVBL0(opcode); break;
		case 0xbc:  MOVBL0(opcode); break;
		case 0xcc:  MOVBL0(opcode); break;
		case 0xdc:  MOVBL0(opcode); break;
		case 0xec:  MOVBL0(opcode); break;
		case 0xfc:  MOVBL0(opcode); break;

		case 0x0d:  MOVWL0(opcode); break;
		case 0x1d:  MOVWL0(opcode); break;
		case 0x2d:  MOVWL0(opcode); break;
		case 0x3d:  MOVWL0(opcode); break;
		case 0x4d:  MOVWL0(opcode); break;
		case 0x5d:  MOVWL0(opcode); break;
		case 0x6d:  MOVWL0(opcode); break;
		case 0x7d:  MOVWL0(opcode); break;
		case 0x8d:  MOVWL0(opcode); break;
		case 0x9d:  MOVWL0(opcode); break;
		case 0xad:  MOVWL0(opcode); break;
		case 0xbd:  MOVWL0(opcode); break;
		case 0xcd:  MOVWL0(opcode); break;
		case 0xdd:  MOVWL0(opcode); break;
		case 0xed:  MOVWL0(opcode); break;
		case 0xfd:  MOVWL0(opcode); break;

		case 0x0e:  MOVLL0(opcode); break;
		case 0x1e:  MOVLL0(opcode); break;
		case 0x2e:  MOVLL0(opcode); break;
		case 0x3e:  MOVLL0(opcode); break;
		case 0x4e:  MOVLL0(opcode); break;
		case 0x5e:  MOVLL0(opcode); break;
		case 0x6e:  MOVLL0(opcode); break;
		case 0x7e:  MOVLL0(opcode); break;
		case 0x8e:  MOVLL0(opcode); break;
		case 0x9e:  MOVLL0(opcode); break;
		case 0xae:  MOVLL0(opcode); break;
		case 0xbe:  MOVLL0(opcode); break;
		case 0xce:  MOVLL0(opcode); break;
		case 0xde:  MOVLL0(opcode); break;
		case 0xee:  MOVLL0(opcode); break;
		case 0xfe:  MOVLL0(opcode); break;

		case 0x0f:  MAC_L(opcode); break;
		case 0x1f:  MAC_L(opcode); break;
		case 0x2f:  MAC_L(opcode); break;
		case 0x3f:  MAC_L(opcode); break;
		case 0x4f:  MAC_L(opcode); break;
		case 0x5f:  MAC_L(opcode); break;
		case 0x6f:  MAC_L(opcode); break;
		case 0x7f:  MAC_L(opcode); break;
		case 0x8f:  MAC_L(opcode); break;
		case 0x9f:  MAC_L(opcode); break;
		case 0xaf:  MAC_L(opcode); break;
		case 0xbf:  MAC_L(opcode); break;
		case 0xcf:  MAC_L(opcode); break;
		case 0xdf:  MAC_L(opcode); break;
		case 0xef:  MAC_L(opcode); break;
		case 0xff:  MAC_L(opcode); break;
	}
};

static void execute_one_4000(const UINT16 opcode)
{
	switch(opcode & 0xff)
	{

		case 0x00:  SHLL(opcode); break;
		case 0x10:  DT(opcode); break;
		case 0x20:  SHAL(opcode); break;
		case 0x30:  NOP(opcode); break;
		case 0x40:  SHLL(opcode); break;
		case 0x50:  DT(opcode); break;
		case 0x60:  SHAL(opcode); break;
		case 0x70:  NOP(opcode); break;
		case 0x80:  SHLL(opcode); break;
		case 0x90:  DT(opcode); break;
		case 0xa0:  SHAL(opcode); break;
		case 0xb0:  NOP(opcode); break;
		case 0xc0:  SHLL(opcode); break;
		case 0xd0:  DT(opcode); break;
		case 0xe0:  SHAL(opcode); break;
		case 0xf0:  NOP(opcode); break;

		case 0x01:  SHLR(opcode); break;
		case 0x11:  CMPPZ(opcode); break;
		case 0x21:  SHAR(opcode); break;
		case 0x31:  NOP(opcode); break;
		case 0x41:  SHLR(opcode); break;
		case 0x51:  CMPPZ(opcode); break;
		case 0x61:  SHAR(opcode); break;
		case 0x71:  NOP(opcode); break;
		case 0x81:  SHLR(opcode); break;
		case 0x91:  CMPPZ(opcode); break;
		case 0xa1:  SHAR(opcode); break;
		case 0xb1:  NOP(opcode); break;
		case 0xc1:  SHLR(opcode); break;
		case 0xd1:  CMPPZ(opcode); break;
		case 0xe1:  SHAR(opcode); break;
		case 0xf1:  NOP(opcode); break;

		case 0x02:  STSMMACH(opcode); break;
		case 0x12:  STSMMACL(opcode); break;
		case 0x22:  STSMPR(opcode); break;
		case 0x32:  STCMSGR(opcode); break;
		case 0x42:  NOP(opcode); break;
		case 0x52:  STSMFPUL(opcode); break;
		case 0x62:  STSMFPSCR(opcode); break;
		case 0x72:  NOP(opcode); break;
		case 0x82:  NOP(opcode); break;
		case 0x92:  NOP(opcode); break;
		case 0xa2:  NOP(opcode); break;
		case 0xb2:  NOP(opcode); break;
		case 0xc2:  NOP(opcode); break;
		case 0xd2:  NOP(opcode); break;
		case 0xe2:  NOP(opcode); break;
		case 0xf2:  STCMDBR(opcode); break;

		case 0x03:  STCMSR(opcode); break;
		case 0x13:  STCMGBR(opcode); break;
		case 0x23:  STCMVBR(opcode); break;
		case 0x33:  STCMSSR(opcode); break;
		case 0x43:  STCMSPC(opcode); break;
		case 0x53:  NOP(opcode); break;
		case 0x63:  NOP(opcode); break;
		case 0x73:  NOP(opcode); break;
		case 0x83:  STCMRBANK(opcode); break;
		case 0x93:  STCMRBANK(opcode); break;
		case 0xa3:  STCMRBANK(opcode); break;
		case 0xb3:  STCMRBANK(opcode); break;
		case 0xc3:  STCMRBANK(opcode); break;
		case 0xd3:  STCMRBANK(opcode); break;
		case 0xe3:  STCMRBANK(opcode); break;
		case 0xf3:  STCMRBANK(opcode); break;

		case 0x04:  ROTL(opcode); break;
		case 0x14:  NOP(opcode); break;
		case 0x24:  ROTCL(opcode); break;
		case 0x34:  NOP(opcode); break;
		case 0x44:  ROTL(opcode); break;
		case 0x54:  NOP(opcode); break;
		case 0x64:  ROTCL(opcode); break;
		case 0x74:  NOP(opcode); break;
		case 0x84:  ROTL(opcode); break;
		case 0x94:  NOP(opcode); break;
		case 0xa4:  ROTCL(opcode); break;
		case 0xb4:  NOP(opcode); break;
		case 0xc4:  ROTL(opcode); break;
		case 0xd4:  NOP(opcode); break;
		case 0xe4:  ROTCL(opcode); break;
		case 0xf4:  NOP(opcode); break;

		case 0x05:  ROTR(opcode); break;
		case 0x15:  CMPPL(opcode); break;
		case 0x25:  ROTCR(opcode); break;
		case 0x35:  NOP(opcode); break;
		case 0x45:  ROTR(opcode); break;
		case 0x55:  CMPPL(opcode); break;
		case 0x65:  ROTCR(opcode); break;
		case 0x75:  NOP(opcode); break;
		case 0x85:  ROTR(opcode); break;
		case 0x95:  CMPPL(opcode); break;
		case 0xa5:  ROTCR(opcode); break;
		case 0xb5:  NOP(opcode); break;
		case 0xc5:  ROTR(opcode); break;
		case 0xd5:  CMPPL(opcode); break;
		case 0xe5:  ROTCR(opcode); break;
		case 0xf5:  NOP(opcode); break;

		case 0x06:  LDSMMACH(opcode); break;
		case 0x16:  LDSMMACL(opcode); break;
		case 0x26:  LDSMPR(opcode); break;
		case 0x36:  NOP(opcode); break;
		case 0x46:  NOP(opcode); break;
		case 0x56:  LDSMFPUL(opcode); break;
		case 0x66:  LDSMFPSCR(opcode); break;
		case 0x76:  NOP(opcode); break;
		case 0x86:  NOP(opcode); break;
		case 0x96:  NOP(opcode); break;
		case 0xa6:  NOP(opcode); break;
		case 0xb6:  NOP(opcode); break;
		case 0xc6:  NOP(opcode); break;
		case 0xd6:  NOP(opcode); break;
		case 0xe6:  NOP(opcode); break;
		case 0xf6:  LDCMDBR(opcode); break;

		case 0x07:  LDCMSR(opcode); break;
		case 0x17:  LDCMGBR(opcode); break;
		case 0x27:  LDCMVBR(opcode); break;
		case 0x37:  LDCMSSR(opcode); break;
		case 0x47:  LDCMSPC(opcode); break;
		case 0x57:  NOP(opcode); break;
		case 0x67:  NOP(opcode); break;
		case 0x77:  NOP(opcode); break;
		case 0x87:  LDCMRBANK(opcode); break;
		case 0x97:  LDCMRBANK(opcode); break;
		case 0xa7:  LDCMRBANK(opcode); break;
		case 0xb7:  LDCMRBANK(opcode); break;
		case 0xc7:  LDCMRBANK(opcode); break;
		case 0xd7:  LDCMRBANK(opcode); break;
		case 0xe7:  LDCMRBANK(opcode); break;
		case 0xf7:  LDCMRBANK(opcode); break;

		case 0x08:  SHLL2(opcode); break;
		case 0x18:  SHLL8(opcode); break;
		case 0x28:  SHLL16(opcode); break;
		case 0x38:  NOP(opcode); break;
		case 0x48:  SHLL2(opcode); break;
		case 0x58:  SHLL8(opcode); break;
		case 0x68:  SHLL16(opcode); break;
		case 0x78:  NOP(opcode); break;
		case 0x88:  SHLL2(opcode); break;
		case 0x98:  SHLL8(opcode); break;
		case 0xa8:  SHLL16(opcode); break;
		case 0xb8:  NOP(opcode); break;
		case 0xc8:  SHLL2(opcode); break;
		case 0xd8:  SHLL8(opcode); break;
		case 0xe8:  SHLL16(opcode); break;
		case 0xf8:  NOP(opcode); break;

		case 0x09:  SHLR2(opcode); break;
		case 0x19:  SHLR8(opcode); break;
		case 0x29:  SHLR16(opcode); break;
		case 0x39:  NOP(opcode); break;
		case 0x49:  SHLR2(opcode); break;
		case 0x59:  SHLR8(opcode); break;
		case 0x69:  SHLR16(opcode); break;
		case 0x79:  NOP(opcode); break;
		case 0x89:  SHLR2(opcode); break;
		case 0x99:  SHLR8(opcode); break;
		case 0xa9:  SHLR16(opcode); break;
		case 0xb9:  NOP(opcode); break;
		case 0xc9:  SHLR2(opcode); break;
		case 0xd9:  SHLR8(opcode); break;
		case 0xe9:  SHLR16(opcode); break;
		case 0xf9:  NOP(opcode); break;

		case 0x0a:  LDSMACH(opcode); break;
		case 0x1a:  LDSMACL(opcode); break;
		case 0x2a:  LDSPR(opcode); break;
		case 0x3a:  NOP(opcode); break;
		case 0x4a:  NOP(opcode); break;
		case 0x5a:  LDSFPUL(opcode); break;
		case 0x6a:  LDSFPSCR(opcode); break;
		case 0x7a:  NOP(opcode); break;
		case 0x8a:  NOP(opcode); break;
		case 0x9a:  NOP(opcode); break;
		case 0xaa:  NOP(opcode); break;
		case 0xba:  NOP(opcode); break;
		case 0xca:  NOP(opcode); break;
		case 0xda:  NOP(opcode); break;
		case 0xea:  NOP(opcode); break;
		case 0xfa:  LDCDBR(opcode); break;

		case 0x0b:  JSR(opcode); break;
		case 0x1b:  TAS(opcode); break;
		case 0x2b:  JMP(opcode); break;
		case 0x3b:  NOP(opcode); break;
		case 0x4b:  JSR(opcode); break;
		case 0x5b:  TAS(opcode); break;
		case 0x6b:  JMP(opcode); break;
		case 0x7b:  NOP(opcode); break;
		case 0x8b:  JSR(opcode); break;
		case 0x9b:  TAS(opcode); break;
		case 0xab:  JMP(opcode); break;
		case 0xbb:  NOP(opcode); break;
		case 0xcb:  JSR(opcode); break;
		case 0xdb:  TAS(opcode); break;
		case 0xeb:  JMP(opcode); break;
		case 0xfb:  NOP(opcode); break;

		case 0x0c:  SHAD(opcode); break;
		case 0x1c:  SHAD(opcode); break;
		case 0x2c:  SHAD(opcode); break;
		case 0x3c:  SHAD(opcode); break;
		case 0x4c:  SHAD(opcode); break;
		case 0x5c:  SHAD(opcode); break;
		case 0x6c:  SHAD(opcode); break;
		case 0x7c:  SHAD(opcode); break;
		case 0x8c:  SHAD(opcode); break;
		case 0x9c:  SHAD(opcode); break;
		case 0xac:  SHAD(opcode); break;
		case 0xbc:  SHAD(opcode); break;
		case 0xcc:  SHAD(opcode); break;
		case 0xdc:  SHAD(opcode); break;
		case 0xec:  SHAD(opcode); break;
		case 0xfc:  SHAD(opcode); break;

		case 0x0d:  SHLD(opcode); break;
		case 0x1d:  SHLD(opcode); break;
		case 0x2d:  SHLD(opcode); break;
		case 0x3d:  SHLD(opcode); break;
		case 0x4d:  SHLD(opcode); break;
		case 0x5d:  SHLD(opcode); break;
		case 0x6d:  SHLD(opcode); break;
		case 0x7d:  SHLD(opcode); break;
		case 0x8d:  SHLD(opcode); break;
		case 0x9d:  SHLD(opcode); break;
		case 0xad:  SHLD(opcode); break;
		case 0xbd:  SHLD(opcode); break;
		case 0xcd:  SHLD(opcode); break;
		case 0xdd:  SHLD(opcode); break;
		case 0xed:  SHLD(opcode); break;
		case 0xfd:  SHLD(opcode); break;

		case 0x0e:  LDCSR(opcode); break;
		case 0x1e:  LDCGBR(opcode); break;
		case 0x2e:  LDCVBR(opcode); break;
		case 0x3e:  LDCSSR(opcode); break;
		case 0x4e:  LDCSPC(opcode); break;
		case 0x5e:  NOP(opcode); break;
		case 0x6e:  NOP(opcode); break;
		case 0x7e:  NOP(opcode); break;
		case 0x8e:  LDCRBANK(opcode); break;
		case 0x9e:  LDCRBANK(opcode); break;
		case 0xae:  LDCRBANK(opcode); break;
		case 0xbe:  LDCRBANK(opcode); break;
		case 0xce:  LDCRBANK(opcode); break;
		case 0xde:  LDCRBANK(opcode); break;
		case 0xee:  LDCRBANK(opcode); break;
		case 0xfe:  LDCRBANK(opcode); break;

		case 0x0f:  MAC_W(opcode); break;
		case 0x1f:  MAC_W(opcode); break;
		case 0x2f:  MAC_W(opcode); break;
		case 0x3f:  MAC_W(opcode); break;
		case 0x4f:  MAC_W(opcode); break;
		case 0x5f:  MAC_W(opcode); break;
		case 0x6f:  MAC_W(opcode); break;
		case 0x7f:  MAC_W(opcode); break;
		case 0x8f:  MAC_W(opcode); break;
		case 0x9f:  MAC_W(opcode); break;
		case 0xaf:  MAC_W(opcode); break;
		case 0xbf:  MAC_W(opcode); break;
		case 0xcf:  MAC_W(opcode); break;
		case 0xdf:  MAC_W(opcode); break;
		case 0xef:  MAC_W(opcode); break;
		case 0xff:  MAC_W(opcode); break;
	}
}

static void execute_one(const UINT16 opcode)
{
	switch(opcode & 0xf000)
	{
		case 0x0000:
			execute_one_0000(opcode);
			break;

		case 0x1000:
			MOVLS4(opcode);
			break;

		case 0x2000:
			switch(opcode & 0x0f)
			{
				case 0x00:  MOVBS(opcode); break;
				case 0x01:  MOVWS(opcode); break;
				case 0x02:  MOVLS(opcode); break;
				case 0x03:  NOP(opcode); break;
				case 0x04:  MOVBM(opcode); break;
				case 0x05:  MOVWM(opcode); break;
				case 0x06:  MOVLM(opcode); break;
				case 0x07:  DIV0S(opcode); break;
				case 0x08:  TST(opcode); break;
				case 0x09:  AND(opcode); break;
				case 0x0a:  XOR(opcode); break;
				case 0x0b:  OR(opcode); break;
				case 0x0c:  CMPSTR(opcode); break;
				case 0x0d:  XTRCT(opcode); break;
				case 0x0e:  MULU(opcode); break;
				case 0x0f:  MULS(opcode); break;
			}
			break;

		case 0x3000:
			switch(opcode & 0x0f)
			{
				case 0x00:  CMPEQ(opcode); break;
				case 0x01:  NOP(opcode); break;
				case 0x02:  CMPHS(opcode); break;
				case 0x03:  CMPGE(opcode); break;
				case 0x04:  DIV1(opcode); break;
				case 0x05:  DMULU(opcode); break;
				case 0x06:  CMPHI(opcode); break;
				case 0x07:  CMPGT(opcode); break;
				case 0x08:  SUB(opcode); break;
				case 0x09:  NOP(opcode); break;
				case 0x0a:  SUBC(opcode); break;
				case 0x0b:  SUBV(opcode); break;
				case 0x0c:  ADD(opcode); break;
				case 0x0d:  DMULS(opcode); break;
				case 0x0e:  ADDC(opcode); break;
				case 0x0f:  ADDV(opcode); break;
			}
			break;

		case 0x4000:
			execute_one_4000(opcode);
			break;

		case 0x5000:
			MOVLL4(opcode);
			break;

		case 0x6000:
			switch(opcode & 0x0f)
			{
				case 0x00:  MOVBL(opcode); break;
				case 0x01:  MOVWL(opcode); break;
				case 0x02:  MOVLL(opcode); break;
				case 0x03:  MOV(opcode); break;
				case 0x04:  MOVBP(opcode); break;
				case 0x05:  MOVWP(opcode); break;
				case 0x06:  MOVLP(opcode); break;
				case 0x07:  NOT(opcode); break;
				case 0x08:  SWAPB(opcode); break;
				case 0x09:  SWAPW(opcode); break;
				case 0x0a:  NEGC(opcode); break;
				case 0x0b:  NEG(opcode); break;
				case 0x0c:  EXTUB(opcode); break;
				case 0x0d:  EXTUW(opcode); break;
				case 0x0e:  EXTSB(opcode); break;
				case 0x0f:  EXTSW(opcode); break;
			}
			break;

		case 0x7000:
			ADDI(opcode);
			break;

		case 0x8000:
			switch((opcode >> 8) & 0x0f)
			{
				case 0x00:  MOVBS4(opcode); break;
				case 0x01:  MOVWS4(opcode); break;
				case 0x02:  NOP(opcode); break;
				case 0x03:  NOP(opcode); break;
				case 0x04:  MOVBL4(opcode); break;
				case 0x05:  MOVWL4(opcode); break;
				case 0x06:  NOP(opcode); break;
				case 0x07:  NOP(opcode); break;
				case 0x08:  CMPIM(opcode); break;
				case 0x09:  BT(opcode); break;
				case 0x0a:  NOP(opcode); break;
				case 0x0b:  BF(opcode); break;
				case 0x0c:  NOP(opcode); break;
				case 0x0d:  BTS(opcode); break;
				case 0x0e:  NOP(opcode); break;
				case 0x0f:  BFS(opcode); break;
			}
			break;

		case 0x9000:
			MOVWI(opcode);
			break;

		case 0xa000:
			BRA(opcode);
			break;

		case 0xb000:
			BSR(opcode);
			break;

		case 0xc000:
			switch((opcode >> 8) & 0x0f)
			{
				case 0x00:  MOVBSG(opcode); break;
				case 0x01:  MOVWSG(opcode); break;
				case 0x02:  MOVLSG(opcode); break;
				case 0x03:  TRAPA(opcode); break;
				case 0x04:  MOVBLG(opcode); break;
				case 0x05:  MOVWLG(opcode); break;
				case 0x06:  MOVLLG(opcode); break;
				case 0x07:  MOVA(opcode); break;
				case 0x08:  TSTI(opcode); break;
				case 0x09:  ANDI(opcode); break;
				case 0x0a:  XORI(opcode); break;
				case 0x0b:  ORI(opcode); break;
				case 0x0c:  TSTM(opcode); break;
				case 0x0d:  ANDM(opcode); break;
				case 0x0e:  XORM(opcode); break;
				case 0x0f:  ORM(opcode); break;
			}
			break;

		case 0xd000:
			MOVLI(opcode);
			break;

		case 0xe000:
			MOVI(opcode);
			break;

		case 0xf000:
			switch(opcode & 0x0f)
			{
				case 0x00:  FADD(opcode); break;
				case 0x01:  FSUB(opcode); break;
				case 0x02:  FMUL(opcode); break;
				case 0x03:  FDIV(opcode); break;
				case 0x04:  FCMP_EQ(opcode); break;
				case 0x05:  FCMP_GT(opcode); break;
				case 0x06:  FMOVS0FR(opcode); break;
				case 0x07:  FMOVFRS0(opcode); break;
				case 0x08:  FMOVMRFR(opcode); break;
				case 0x09:  FMOVMRIFR(opcode); break;
				case 0x0a:  FMOVFRMR(opcode); break;
				case 0x0b:  FMOVFRMDR(opcode); break;
				case 0x0c:  FMOVFR(opcode); break;
				case 0x0d:  op1111_0x13(opcode); break;
				case 0x0e:  FMAC(opcode); break;
				case 0x0f:  dbreak(opcode); break;
			}
			break;
	}
}

static void execute_run()
{
	if (sh3.m_cpu_off)
	{
		sh3.m_sh4_icount = 0;
		return;
	}

	do
	{
		if (sh3.m_delay)
		{
			UINT16 opcode;
   change_pc(sh3.m_delay & AM);
   opcode = cpu_readop16(WORD2_XOR_BE((UINT32)(sh3.m_delay & AM)));

			CALL_MAME_DEBUG;

			sh3.m_delay = 0;
			sh3.m_ppc = sh3.m_pc;

			execute_one(opcode);

			if (sh3.m_test_irq && !sh3.m_delay)
			{
				sh4_check_pending_irq("mame_sh4_execute");
			}

		}
		else
		{
			UINT16 opcode;
   change_pc(sh3.m_pc & AM);
   opcode = cpu_readop16(WORD2_XOR_BE((UINT32)(sh3.m_pc & AM)));

			CALL_MAME_DEBUG;

			sh3.m_pc += 2;
			sh3.m_ppc = sh3.m_pc;

			execute_one(opcode);

			if (sh3.m_test_irq && !sh3.m_delay)
			{
				sh4_check_pending_irq("mame_sh4_execute");
			}
		}

		sh3.m_sh4_icount--;
	} while( sh3.m_sh4_icount > 0 );
}


static const int rtcnt_div[8] = { 0, 4, 16, 64, 256, 1024, 2048, 4096 };
static const int daysmonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static const UINT32 exception_priority_default[] = {
	EXPPRI(1,1,0,0),
	EXPPRI(1,2,0,1),
	EXPPRI(1,1,0,2),
	EXPPRI(1,3,0,3),
	EXPPRI(1,4,0,4),

	EXPPRI(2,0,0,5),
	EXPPRI(2,1,0,6),
	EXPPRI(2,2,0,7),
	EXPPRI(2,3,0,8),
	EXPPRI(2,4,0,9),
	EXPPRI(2,4,0,10),
	EXPPRI(2,4,0,11),
	EXPPRI(2,4,0,12),
	EXPPRI(2,5,0,13),
	EXPPRI(2,5,0,14),
	EXPPRI(2,6,0,15),
	EXPPRI(2,6,0,16),
	EXPPRI(2,7,0,17),
	EXPPRI(2,7,0,18),
	EXPPRI(2,8,0,19),
	EXPPRI(2,9,0,20),

	EXPPRI(2,4,0,21),
	EXPPRI(2,10,0,22),

	EXPPRI(3,0,16,SH4_INTC_NMI)

};

static const int exception_codes[] =

{ 0x000,
	0x020,
	0x000,
	0x140,
	0x140,

	0x1E0,
	0x0E0,
	0x040,
	0x0A0,
	0x180,
	0x1A0,
	0x800,
	0x820,
	0x0E0,
	0x100,
	0x040,
	0x060,
	0x0A0,
	0x0C0,
	0x120,
	0x080,

	0x160,
	0x1E0,

	0x1C0,

	0x200,
	0x220,
	0x240,
	0x260,
	0x280,
	0x2A0,
	0x2C0,
	0x2E0,
	0x300,
	0x320,
	0x340,
	0x360,
	0x380,
	0x3A0,
	0x3C0,

	0x240,
	0x2A0,
	0x300,
	0x360,

	0x600,
	0x620,
	0x640,
	0x660,
	0x680,
	0x6A0,

	0x780,
	0x7A0,
	0x7C0,
	0x7E0,

	0x6C0,

	0xB00,
	0xB80,
	0x400,
	0x420,
	0x440,
	0x460,
	0x480,
	0x4A0,
	0x4C0,
	0x4E0,
	0x500,
	0x520,
	0x540,

	0x700,
	0x720,
	0x740,
	0x760,
	0x560,
	0x580,
	0x5A0
};

static const int sh3_intevt2_exception_codes[] =

{ 0x000,
	-1,
	-1,
	-1,
	-1,

	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,

	-1,
	-1,

	-1,

	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,

	0x600,
	0x620,
	0x640,
	0x660,

	-1,
	-1,
	-1,
	-1,
	-1,
	-1,

	-1,
	-1,
	-1,
	-1,

	-1,

	-1,
	-1,
	0x400,
	0x420,
	0x440,
	0x460,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,

	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1
};

static void sh4_change_register_bank(int to)
{
	int s;

	if (to)
	{
		for (s = 0;s < 8;s++)
		{
			sh3.m_rbnk[0][s] = sh3.m_r[s];
			sh3.m_r[s] = sh3.m_rbnk[1][s];
		}
	}
	else
	{
		for (s = 0;s < 8;s++)
		{
			sh3.m_rbnk[1][s] = sh3.m_r[s];
			sh3.m_r[s] = sh3.m_rbnk[0][s];
		}
	}
}

static void sh4_swap_fp_registers()
{
	int s;
	UINT32 z;

	for (s = 0;s <= 15;s++)
	{
		z = sh3.m_fr[s];
		sh3.m_fr[s] = sh3.m_xf[s];
		sh3.m_xf[s] = z;
	}
}

static void sh4_swap_fp_couples()
{
	int s;
	UINT32 z;

	for (s = 0;s <= 15;s = s+2)
	{
		z = sh3.m_fr[s];
		sh3.m_fr[s] = sh3.m_fr[s + 1];
		sh3.m_fr[s + 1] = z;
		z = sh3.m_xf[s];
		sh3.m_xf[s] = sh3.m_xf[s + 1];
		sh3.m_xf[s + 1] = z;
	}
}

static void sh4_syncronize_register_bank(int to)
{
	int s;

	for (s = 0;s < 8;s++)
	{
		sh3.m_rbnk[to][s] = sh3.m_r[s];
	}
}

static void sh4_default_exception_priorities()
{
	int a;

	for (a=0;a <= SH4_INTC_NMI;a++)
		sh3.m_exception_priority[a] = exception_priority_default[a];
	for (a=SH4_INTC_IRLn0;a <= SH4_INTC_IRLnE;a++)
		sh3.m_exception_priority[a] = INTPRI(15-(a-SH4_INTC_IRLn0), a);
	sh3.m_exception_priority[SH4_INTC_IRL0] = INTPRI(13, SH4_INTC_IRL0);
	sh3.m_exception_priority[SH4_INTC_IRL1] = INTPRI(10, SH4_INTC_IRL1);
	sh3.m_exception_priority[SH4_INTC_IRL2] = INTPRI(7, SH4_INTC_IRL2);
	sh3.m_exception_priority[SH4_INTC_IRL3] = INTPRI(4, SH4_INTC_IRL3);
	for (a=SH4_INTC_HUDI;a <= SH4_INTC_ROVI;a++)
		sh3.m_exception_priority[a] = INTPRI(0, a);
}

static void sh4_exception_recompute()
{
	int a,z;

	sh3.m_test_irq = 0;
	if ((!sh3.m_pending_irq) || ((sh3.m_sr & BL) && (sh3.m_exception_requesting[SH4_INTC_NMI] == 0)))
		return;
	z = (sh3.m_sr >> 4) & 15;
	for (a=0;a <= SH4_INTC_ROVI;a++)
	{
		if (sh3.m_exception_requesting[a])
		{
			int pri = (((int)sh3.m_exception_priority[a] >> 8) & 255);

			if (pri > z)
			{

				sh3.m_test_irq = 1;
				break;
			}
		}
	}
}

static void sh4_exception_request(int exception)
{

	if (!sh3.m_exception_requesting[exception])
	{

		sh3.m_exception_requesting[exception] = 1;
		sh3.m_pending_irq++;
		sh4_exception_recompute();
	}
}

static void sh4_exception_unrequest(int exception)
{
	if (sh3.m_exception_requesting[exception])
	{
		sh3.m_exception_requesting[exception] = 0;
		sh3.m_pending_irq--;
		sh4_exception_recompute();
	}
}

static void sh4_exception_checkunrequest(int exception)
{
	if (exception == SH4_INTC_NMI)
		sh4_exception_unrequest(exception);
	if ((exception == SH4_INTC_DMTE0) || (exception == SH4_INTC_DMTE1) ||
		(exception == SH4_INTC_DMTE2) || (exception == SH4_INTC_DMTE3))
		sh4_exception_unrequest(exception);
}

static void sh4_exception(const char *message, int exception)
{
	UINT32 vector;

	if (sh3.m_cpu_type == CPU_TYPE_SH4)
	{
		if (exception < SH4_INTC_NMI)
			return;
		if (exception == SH4_INTC_NMI) {
			if ((sh3.m_sr & BL) && (!(sh3.m_m[ICR] & 0x200)))
				return;

			sh3.m_m[ICR] &= ~0x200;
			sh3.m_m[INTEVT] = 0x1c0;

			vector = 0x600;
			sh3_irq_callback(INPUT_LINE_NMI);
			LOG(("SH-4 '%s' nmi exception after [%s]\n", "sh3", message));
		} else {

			if (sh3.m_sr & BL)
				return;
			if (((sh3.m_exception_priority[exception] >> 8) & 255) <= ((sh3.m_sr >> 4) & 15))
				return;
			sh3.m_m[INTEVT] = exception_codes[exception];
			vector = 0x600;
			if ((exception >= SH4_INTC_IRL0) && (exception <= SH4_INTC_IRL3))
				sh3_irq_callback((exception-SH4_INTC_IRL0)+SH4_IRL0);
			else
				sh3_irq_callback(SH4_IRL3+1);
			LOG(("SH-4 '%s' interrupt exception #%d after [%s]\n", "sh3", exception, message));
		}
	}
	else
	{

		if (exception < SH4_INTC_NMI)
			return;
		if (exception == SH4_INTC_NMI)
		{
			return;
		}
		else
		{
			if (sh3.m_sr & BL)
				return;
			if (((sh3.m_exception_priority[exception] >> 8) & 255) <= ((sh3.m_sr >> 4) & 15))
				return;

			vector = 0x600;

			if ((exception >= SH4_INTC_IRL0) && (exception <= SH4_INTC_IRL3))
				sh3_irq_callback((exception-SH4_INTC_IRL0)+SH4_IRL0);
			else
				sh3_irq_callback(SH4_IRL3+1);

			if (sh3_intevt2_exception_codes[exception]==-1)
				fatalerror("sh3_intevt2_exception_codes unpopulated for exception %02x\n", exception);

			sh3.m_sh3internal_lower[INTEVT2] = sh3_intevt2_exception_codes[exception];
			sh3.m_sh3internal_upper[SH3_EXPEVT_ADDR] = exception_codes[exception];

			LOG(("SH-3 '%s' interrupt exception #%d after [%s]\n", "sh3", exception, message));
		}

	}
	sh4_exception_checkunrequest(exception);

	sh3.m_spc = sh3.m_pc;
	sh3.m_ssr = sh3.m_sr;
	sh3.m_sgr = sh3.m_r[15];

	sh3.m_sr |= MD;
	if ((0) != 0)
		sh4_syncronize_register_bank((sh3.m_sr & sRB) >> 29);
	if (!(sh3.m_sr & sRB))
		sh4_change_register_bank(1);
	sh3.m_sr |= sRB;
	sh3.m_sr |= BL;
	sh4_exception_recompute();

	sh3.m_pc = sh3.m_vbr + vector;

	if(sh3.m_sleep_mode == 1) { sh3.m_sleep_mode = 2; }
}

static UINT32 compute_ticks_refresh_timer(mame_timer *timer, int hertz, int base, int divisor)
{

	return base + (UINT32)((timer_timeelapsed(timer) * (double)hertz) / (double)divisor);
}

static void sh4_refresh_timer_recompute()
{
	UINT32 ticks;

	if (sh3.m_cpu_type != CPU_TYPE_SH4)
		fatalerror("sh4_refresh_timer_recompute uses sh3.m_m[] with SH3\n");

	ticks = sh3.m_m[RTCOR]-sh3.m_m[RTCNT];
	if (ticks <= 0)
		ticks = 256 + ticks;
	timer_adjust(sh3.m_refresh_timer,TIME_IN_HZ(sh3.m_bus_clock) * rtcnt_div[(sh3.m_m[RTCSR] >> 3) & 7] * ticks,0,0);
	sh3.m_refresh_timer_base = sh3.m_m[RTCNT];
}

static void sh4_refresh_timer_callback(int param)
{
	if (sh3.m_cpu_type != CPU_TYPE_SH4)
		fatalerror("sh4_refresh_timer_callback uses sh3.m_m[] with SH3\n");

	sh3.m_m[RTCNT] = 0;
	sh4_refresh_timer_recompute();
	sh3.m_m[RTCSR] |= 128;
	if ((sh3.m_m[MCR] & 4) && !(sh3.m_m[MCR] & 2))
	{
		sh3.m_m[RFCR] = (sh3.m_m[RFCR] + 1) & 1023;
		if (((sh3.m_m[RTCSR] & 1) && (sh3.m_m[RFCR] == 512)) || (sh3.m_m[RFCR] == 0))
		{
			sh3.m_m[RFCR] = 0;
			sh3.m_m[RTCSR] |= 4;
		}
	}
}

static void increment_rtc_time(int mode)
{
	int carry, year, leap, days;

	if (sh3.m_cpu_type != CPU_TYPE_SH4)
		fatalerror("increment_rtc_time uses sh3.m_m[] with SH3\n");

	if (mode == 0)
	{
		carry = 0;
		sh3.m_m[RSECCNT] = sh3.m_m[RSECCNT] + 1;
		if ((sh3.m_m[RSECCNT] & 0xf) == 0xa)
			sh3.m_m[RSECCNT] = sh3.m_m[RSECCNT] + 6;
		if (sh3.m_m[RSECCNT] == 0x60)
		{
			sh3.m_m[RSECCNT] = 0;
			carry=1;
		}
		else
			return;
	}
	else
		carry = 1;

	sh3.m_m[RMINCNT] = sh3.m_m[RMINCNT] + carry;
	if ((sh3.m_m[RMINCNT] & 0xf) == 0xa)
		sh3.m_m[RMINCNT] = sh3.m_m[RMINCNT] + 6;
	carry=0;
	if (sh3.m_m[RMINCNT] == 0x60)
	{
		sh3.m_m[RMINCNT] = 0;
		carry = 1;
	}

	sh3.m_m[RHRCNT] = sh3.m_m[RHRCNT] + carry;
	if ((sh3.m_m[RHRCNT] & 0xf) == 0xa)
		sh3.m_m[RHRCNT] = sh3.m_m[RHRCNT] + 6;
	carry = 0;
	if (sh3.m_m[RHRCNT] == 0x24)
	{
		sh3.m_m[RHRCNT] = 0;
		carry = 1;
	}

	sh3.m_m[RWKCNT] = sh3.m_m[RWKCNT] + carry;
	if (sh3.m_m[RWKCNT] == 0x7)
	{
		sh3.m_m[RWKCNT] = 0;
	}

	days = 0;
	year = (sh3.m_m[RYRCNT] & 0xf) + ((sh3.m_m[RYRCNT] & 0xf0) >> 4)*10 + ((sh3.m_m[RYRCNT] & 0xf00) >> 8)*100 + ((sh3.m_m[RYRCNT] & 0xf000) >> 12)*1000;
	leap = 0;
	if (!(year%100))
	{
		if (!(year%400))
			leap = 1;
	}
	else if (!(year%4))
		leap = 1;
	if (sh3.m_m[RMONCNT] != 2)
		leap = 0;
	if (sh3.m_m[RMONCNT])
		days = daysmonth[(sh3.m_m[RMONCNT] & 0xf) + ((sh3.m_m[RMONCNT] & 0xf0) >> 4)*10 - 1];

	sh3.m_m[RDAYCNT] = sh3.m_m[RDAYCNT] + carry;
	if ((sh3.m_m[RDAYCNT] & 0xf) == 0xa)
		sh3.m_m[RDAYCNT] = sh3.m_m[RDAYCNT] + 6;
	carry = 0;
	if (sh3.m_m[RDAYCNT] > (days+leap))
	{
		sh3.m_m[RDAYCNT] = 1;
		carry = 1;
	}

	sh3.m_m[RMONCNT] = sh3.m_m[RMONCNT] + carry;
	if ((sh3.m_m[RMONCNT] & 0xf) == 0xa)
		sh3.m_m[RMONCNT] = sh3.m_m[RMONCNT] + 6;
	carry=0;
	if (sh3.m_m[RMONCNT] == 0x13)
	{
		sh3.m_m[RMONCNT] = 1;
		carry = 1;
	}

	sh3.m_m[RYRCNT] = sh3.m_m[RYRCNT] + carry;
	if ((sh3.m_m[RYRCNT] & 0xf) >= 0xa)
		sh3.m_m[RYRCNT] = sh3.m_m[RYRCNT] + 6;
	if ((sh3.m_m[RYRCNT] & 0xf0) >= 0xa0)
		sh3.m_m[RYRCNT] = sh3.m_m[RYRCNT] + 0x60;
	if ((sh3.m_m[RYRCNT] & 0xf00) >= 0xa00)
		sh3.m_m[RYRCNT] = sh3.m_m[RYRCNT] + 0x600;
	if ((sh3.m_m[RYRCNT] & 0xf000) >= 0xa000)
		sh3.m_m[RYRCNT] = 0;
}

static void sh4_rtc_timer_callback(int param)
{
	if (sh3.m_cpu_type != CPU_TYPE_SH4)
	{
		logerror("sh4_rtc_timer_callback uses sh3.m_m[] with SH3\n");
		return;
	}

	timer_adjust(sh3.m_rtc_timer,TIME_IN_HZ(128),0,0);
	sh3.m_m[R64CNT] = (sh3.m_m[R64CNT]+1) & 0x7f;
	if (sh3.m_m[R64CNT] == 64)
	{
		sh3.m_m[RCR1] |= 0x80;
		increment_rtc_time(0);

	}
}

static void sh4_dmac_nmi()
{
	int s;

	sh3.m_SH4_DMAOR |= DMAOR_NMIF;
	for (s = 0;s < 4;s++)
	{
		if (sh3.m_dma_timer_active[s])
		{
			logerror("SH4: DMA %d cancelled due to NMI but all data transferred", s);
			timer_adjust(sh3.m_dma_timer[s],TIME_NEVER, s,0);
			sh3.m_dma_timer_active[s] = 0;
		}
	}
}

static void sh4_handler_ipra_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_IPRA);

	sh3.m_exception_priority[SH4_INTC_ATI]     = INTPRI(sh3.m_SH4_IPRA & 0x000f, SH4_INTC_ATI);
	sh3.m_exception_priority[SH4_INTC_PRI]     = INTPRI(sh3.m_SH4_IPRA & 0x000f, SH4_INTC_PRI);
	sh3.m_exception_priority[SH4_INTC_CUI]     = INTPRI(sh3.m_SH4_IPRA & 0x000f, SH4_INTC_CUI);

	sh3.m_exception_priority[SH4_INTC_TUNI2]  = INTPRI((sh3.m_SH4_IPRA & 0x00f0) >> 4, SH4_INTC_TUNI2);
	sh3.m_exception_priority[SH4_INTC_TICPI2] = INTPRI((sh3.m_SH4_IPRA & 0x00f0) >> 4, SH4_INTC_TICPI2);

	sh3.m_exception_priority[SH4_INTC_TUNI1]  = INTPRI((sh3.m_SH4_IPRA & 0x0f00) >> 8, SH4_INTC_TUNI1);

	sh3.m_exception_priority[SH4_INTC_TUNI0]  = INTPRI((sh3.m_SH4_IPRA & 0xf000) >> 12, SH4_INTC_TUNI0);

	logerror("setting priorities TMU0 %01x TMU1 %01x TMU2 %01x RTC %01x\n", (sh3.m_SH4_IPRA & 0xf000)>>12, (sh3.m_SH4_IPRA & 0x0f00)>>8, (sh3.m_SH4_IPRA & 0x00f0)>>4, (sh3.m_SH4_IPRA & 0x000f)>>0);

	sh4_exception_recompute();
}

static void sh4_internal_w(offs_t offset, UINT32 data, UINT32 mem_mask)
{
	int a;
	UINT32 addr = (offset << 2) + 0xfe000000;
	offset = ((addr & 0xfc) >> 2) | ((addr & 0x1fe0000) >> 11);

	if (sh3.m_cpu_type != CPU_TYPE_SH4)
		fatalerror("sh4_internal_w uses sh3.m_m[] with SH3\n");

	UINT32 old = sh3.m_m[offset];
	COMBINE_DATA(sh3.m_m+offset);

	switch( offset )
	{
	case MMUCR:
		if (data & MMUCR_AT)
		{
			printf("SH4 MMU Enabled\n");
			printf("If you're seeing this, but running something other than a Naomi GD-ROM game then chances are it won't work\n");
			printf("The MMU emulation is a hack specific to that system\n");
			sh3.m_sh4_mmu_enabled = 1;

			{
				int i;
				for (i=0;i<64;i++)
				{
					sh3.m_sh4_tlb_address[i] = 0;
					sh3.m_sh4_tlb_data[i] = 0;
				}

			}
		}
		else
		{
			sh3.m_sh4_mmu_enabled = 0;
		}

		break;

	case RTCSR:
		sh3.m_m[RTCSR] &= 255;
		if ((old >> 3) & 7)
			sh3.m_m[RTCNT] = compute_ticks_refresh_timer(sh3.m_refresh_timer, sh3.m_bus_clock, sh3.m_refresh_timer_base, rtcnt_div[(old >> 3) & 7]) & 0xff;
		if ((sh3.m_m[RTCSR] >> 3) & 7)
		{
			sh4_refresh_timer_recompute();
		}
		else
		{
			timer_adjust(sh3.m_refresh_timer,TIME_NEVER,0,0);
		}
		break;

	case RTCNT:
		sh3.m_m[RTCNT] &= 255;
		if ((sh3.m_m[RTCSR] >> 3) & 7)
		{
			sh4_refresh_timer_recompute();
		}
		break;

	case RTCOR:
		sh3.m_m[RTCOR] &= 255;
		if ((sh3.m_m[RTCSR] >> 3) & 7)
		{
			sh3.m_m[RTCNT] = compute_ticks_refresh_timer(sh3.m_refresh_timer, sh3.m_bus_clock, sh3.m_refresh_timer_base, rtcnt_div[(sh3.m_m[RTCSR] >> 3) & 7]) & 0xff;
			sh4_refresh_timer_recompute();
		}
		break;

	case RFCR:
		sh3.m_m[RFCR] &= 1023;
		break;

	case RCR1:
		if ((sh3.m_m[RCR1] & 8) && (~old & 8))
			sh3.m_m[RCR1] ^= 1;
		break;

	case RCR2:
		if (sh3.m_m[RCR2] & 2)
		{
			sh3.m_m[R64CNT] = 0;
			sh3.m_m[RCR2] ^= 2;
		}
		if (sh3.m_m[RCR2] & 4)
		{
			sh3.m_m[R64CNT] = 0;
			if (sh3.m_m[RSECCNT] >= 30)
				increment_rtc_time(1);
			sh3.m_m[RSECCNT] = 0;
		}
		if ((sh3.m_m[RCR2] & 8) && (~old & 8))
		{
			timer_adjust(sh3.m_rtc_timer,TIME_IN_HZ(128),0,0);
		}
		else if (~(sh3.m_m[RCR2]) & 8)
		{
			timer_adjust(sh3.m_rtc_timer,TIME_NEVER,0,0);
		}
		break;

	case SH4_TSTR_ADDR: sh4_handle_tstr_addr_w(data,mem_mask);   break;
	case SH4_TCR0_ADDR: sh4_handle_tcr0_addr_w(data,mem_mask);   break;
	case SH4_TCR1_ADDR: sh4_handle_tcr1_addr_w(data,mem_mask);   break;
	case SH4_TCR2_ADDR: sh4_handle_tcr2_addr_w(data,mem_mask);   break;
	case SH4_TCOR0_ADDR: sh4_handle_tcor0_addr_w(data,mem_mask); break;
	case SH4_TCNT0_ADDR: sh4_handle_tcnt0_addr_w(data,mem_mask); break;
	case SH4_TCOR1_ADDR: sh4_handle_tcor1_addr_w(data,mem_mask); break;
	case SH4_TCNT1_ADDR: sh4_handle_tcnt1_addr_w(data,mem_mask); break;
	case SH4_TCOR2_ADDR: sh4_handle_tcor2_addr_w(data,mem_mask); break;
	case SH4_TCNT2_ADDR: sh4_handle_tcnt2_addr_w(data,mem_mask); break;
	case SH4_TOCR_ADDR: sh4_handle_tocr_addr_w(data,mem_mask);   break;
	case SH4_TCPR2_ADDR: sh4_handle_tcpr2_addr_w(data,mem_mask); break;

	case ICR:
		sh3.m_m[ICR] = (sh3.m_m[ICR] & 0x7fff) | (old & 0x8000);
		break;
	case IPRA: sh4_handler_ipra_w(data, mem_mask); break;
	case IPRB:
		sh3.m_exception_priority[SH4_INTC_SCI1ERI] = INTPRI((sh3.m_m[IPRB] & 0x00f0) >> 4, SH4_INTC_SCI1ERI);
		sh3.m_exception_priority[SH4_INTC_SCI1RXI] = INTPRI((sh3.m_m[IPRB] & 0x00f0) >> 4, SH4_INTC_SCI1RXI);
		sh3.m_exception_priority[SH4_INTC_SCI1TXI] = INTPRI((sh3.m_m[IPRB] & 0x00f0) >> 4, SH4_INTC_SCI1TXI);
		sh3.m_exception_priority[SH4_INTC_SCI1TEI] = INTPRI((sh3.m_m[IPRB] & 0x00f0) >> 4, SH4_INTC_SCI1TEI);
		sh3.m_exception_priority[SH4_INTC_RCMI] = INTPRI((sh3.m_m[IPRB] & 0x0f00) >> 8, SH4_INTC_RCMI);
		sh3.m_exception_priority[SH4_INTC_ROVI] = INTPRI((sh3.m_m[IPRB] & 0x0f00) >> 8, SH4_INTC_ROVI);
		sh3.m_exception_priority[SH4_INTC_ITI] = INTPRI((sh3.m_m[IPRB] & 0xf000) >> 12, SH4_INTC_ITI);
		sh4_exception_recompute();
		break;
	case IPRC:
		sh3.m_exception_priority[SH4_INTC_HUDI] = INTPRI(sh3.m_m[IPRC] & 0x000f, SH4_INTC_HUDI);
		sh3.m_exception_priority[SH4_INTC_SCIFERI] = INTPRI((sh3.m_m[IPRC] & 0x00f0) >> 4, SH4_INTC_SCIFERI);
		sh3.m_exception_priority[SH4_INTC_SCIFRXI] = INTPRI((sh3.m_m[IPRC] & 0x00f0) >> 4, SH4_INTC_SCIFRXI);
		sh3.m_exception_priority[SH4_INTC_SCIFBRI] = INTPRI((sh3.m_m[IPRC] & 0x00f0) >> 4, SH4_INTC_SCIFBRI);
		sh3.m_exception_priority[SH4_INTC_SCIFTXI] = INTPRI((sh3.m_m[IPRC] & 0x00f0) >> 4, SH4_INTC_SCIFTXI);
		sh3.m_exception_priority[SH4_INTC_DMTE0] = INTPRI((sh3.m_m[IPRC] & 0x0f00) >> 8, SH4_INTC_DMTE0);
		sh3.m_exception_priority[SH4_INTC_DMTE1] = INTPRI((sh3.m_m[IPRC] & 0x0f00) >> 8, SH4_INTC_DMTE1);
		sh3.m_exception_priority[SH4_INTC_DMTE2] = INTPRI((sh3.m_m[IPRC] & 0x0f00) >> 8, SH4_INTC_DMTE2);
		sh3.m_exception_priority[SH4_INTC_DMTE3] = INTPRI((sh3.m_m[IPRC] & 0x0f00) >> 8, SH4_INTC_DMTE3);
		sh3.m_exception_priority[SH4_INTC_DMAE] = INTPRI((sh3.m_m[IPRC] & 0x0f00) >> 8, SH4_INTC_DMAE);
		sh3.m_exception_priority[SH4_INTC_GPOI] = INTPRI((sh3.m_m[IPRC] & 0xf000) >> 12, SH4_INTC_GPOI);
		sh4_exception_recompute();
		break;

	case SH4_SAR0_ADDR: sh4_handle_sar0_addr_w(data,mem_mask);   break;
	case SH4_SAR1_ADDR: sh4_handle_sar1_addr_w(data,mem_mask);   break;
	case SH4_SAR2_ADDR: sh4_handle_sar2_addr_w(data,mem_mask);   break;
	case SH4_SAR3_ADDR: sh4_handle_sar3_addr_w(data,mem_mask);   break;
	case SH4_DAR0_ADDR: sh4_handle_dar0_addr_w(data,mem_mask);   break;
	case SH4_DAR1_ADDR: sh4_handle_dar1_addr_w(data,mem_mask);   break;
	case SH4_DAR2_ADDR: sh4_handle_dar2_addr_w(data,mem_mask);   break;
	case SH4_DAR3_ADDR: sh4_handle_dar3_addr_w(data,mem_mask);   break;
	case SH4_DMATCR0_ADDR: sh4_handle_dmatcr0_addr_w(data,mem_mask);   break;
	case SH4_DMATCR1_ADDR: sh4_handle_dmatcr1_addr_w(data,mem_mask);   break;
	case SH4_DMATCR2_ADDR: sh4_handle_dmatcr2_addr_w(data,mem_mask);   break;
	case SH4_DMATCR3_ADDR: sh4_handle_dmatcr3_addr_w(data,mem_mask);   break;
	case SH4_CHCR0_ADDR: sh4_handle_chcr0_addr_w(data,mem_mask);   break;
	case SH4_CHCR1_ADDR: sh4_handle_chcr1_addr_w(data,mem_mask);   break;
	case SH4_CHCR2_ADDR: sh4_handle_chcr2_addr_w(data,mem_mask);   break;
	case SH4_CHCR3_ADDR: sh4_handle_chcr3_addr_w(data,mem_mask);   break;
	case SH4_DMAOR_ADDR: sh4_handle_dmaor_addr_w(data,mem_mask);   break;

	case QACR0:
	case QACR1:
		break;

	case PCTRA:
		sh3.m_ioport16_pullup = 0;
		sh3.m_ioport16_direction = 0;
		for (a=0;a < 16;a++) {
			sh3.m_ioport16_direction |= (sh3.m_m[PCTRA] & (1 << (a*2))) >> a;
			sh3.m_ioport16_pullup |= (sh3.m_m[PCTRA] & (1 << (a*2+1))) >> (a+1);
		}
		sh3.m_ioport16_direction &= 0xffff;
		sh3.m_ioport16_pullup = (sh3.m_ioport16_pullup | sh3.m_ioport16_direction) ^ 0xffff;
		if (sh3.m_m[BCR2] & 1)
			io_write_dword(SH4_IOPORT_16, (UINT64)(sh3.m_m[PDTRA] & sh3.m_ioport16_direction) | ((UINT64)sh3.m_m[PCTRA] << 16));
		break;
	case PDTRA:
		if (sh3.m_m[BCR2] & 1)
			io_write_dword(SH4_IOPORT_16, (UINT64)(sh3.m_m[PDTRA] & sh3.m_ioport16_direction) | ((UINT64)sh3.m_m[PCTRA] << 16));
		break;
	case PCTRB:
		sh3.m_ioport4_pullup = 0;
		sh3.m_ioport4_direction = 0;
		for (a=0;a < 4;a++) {
			sh3.m_ioport4_direction |= (sh3.m_m[PCTRB] & (1 << (a*2))) >> a;
			sh3.m_ioport4_pullup |= (sh3.m_m[PCTRB] & (1 << (a*2+1))) >> (a+1);
		}
		sh3.m_ioport4_direction &= 0xf;
		sh3.m_ioport4_pullup = (sh3.m_ioport4_pullup | sh3.m_ioport4_direction) ^ 0xf;
		if (sh3.m_m[BCR2] & 1)
			io_write_dword(SH4_IOPORT_4, (sh3.m_m[PDTRB] & sh3.m_ioport4_direction) | (sh3.m_m[PCTRB] << 16));
		break;
	case PDTRB:
		if (sh3.m_m[BCR2] & 1)
			io_write_dword(SH4_IOPORT_4, (sh3.m_m[PDTRB] & sh3.m_ioport4_direction) | (sh3.m_m[PCTRB] << 16));
		break;

	case SCBRR2:
		break;

	case SCSPTR2:
		break;

	default:
		logerror("sh4_internal_w:  Unmapped write %08x, %08x @ %08x\n", 0xfe000000+((offset & 0x3fc0) << 11)+((offset & 0x3f) << 2), data, mem_mask);
		break;
	}
}

static UINT32 sh4_internal_r(offs_t offset, UINT32 mem_mask)
{
	if (sh3.m_cpu_type != CPU_TYPE_SH4)
		fatalerror("sh4_internal_r uses sh3.m_m[] with SH3\n");

	UINT32 addr = (offset << 2) + 0xfe000000;
	offset = ((addr & 0xfc) >> 2) | ((addr & 0x1fe0000) >> 11);

	switch( offset )
	{
	case VERSION:
		return PVR_SH7091;
	case PRR:
		return 0;
	case IPRD:
		return 0x00000000;
	case RTCNT:
		if ((sh3.m_m[RTCSR] >> 3) & 7)
		{

			return compute_ticks_refresh_timer(sh3.m_refresh_timer, sh3.m_bus_clock, sh3.m_refresh_timer_base, rtcnt_div[(sh3.m_m[RTCSR] >> 3) & 7]) & 0xff;
		}
		else
			return sh3.m_m[RTCNT];

	case IPRA:
		return sh3.m_SH4_IPRA;

	case SH4_TSTR_ADDR:  return sh4_handle_tstr_addr_r(mem_mask);
	case SH4_TCR0_ADDR:  return sh4_handle_tcr0_addr_r(mem_mask);
	case SH4_TCR1_ADDR:  return sh4_handle_tcr1_addr_r(mem_mask);
	case SH4_TCR2_ADDR:  return sh4_handle_tcr2_addr_r(mem_mask);
	case SH4_TCNT0_ADDR: return sh4_handle_tcnt0_addr_r(mem_mask);
	case SH4_TCNT1_ADDR: return sh4_handle_tcnt1_addr_r(mem_mask);
	case SH4_TCNT2_ADDR: return sh4_handle_tcnt2_addr_r(mem_mask);
	case SH4_TCOR0_ADDR: return sh4_handle_tcor0_addr_r(mem_mask);
	case SH4_TCOR1_ADDR: return sh4_handle_tcor1_addr_r(mem_mask);
	case SH4_TCOR2_ADDR: return sh4_handle_tcor2_addr_r(mem_mask);
	case SH4_TOCR_ADDR:  return sh4_handle_tocr_addr_r(mem_mask);
	case SH4_TCPR2_ADDR: return sh4_handle_tcpr2_addr_r(mem_mask);

	case SH4_SAR0_ADDR: return sh4_handle_sar0_addr_r(mem_mask);
	case SH4_SAR1_ADDR: return sh4_handle_sar1_addr_r(mem_mask);
	case SH4_SAR2_ADDR: return sh4_handle_sar2_addr_r(mem_mask);
	case SH4_SAR3_ADDR: return sh4_handle_sar3_addr_r(mem_mask);
	case SH4_DAR0_ADDR: return sh4_handle_dar0_addr_r(mem_mask);
	case SH4_DAR1_ADDR: return sh4_handle_dar1_addr_r(mem_mask);
	case SH4_DAR2_ADDR: return sh4_handle_dar2_addr_r(mem_mask);
	case SH4_DAR3_ADDR: return sh4_handle_dar3_addr_r(mem_mask);
	case SH4_DMATCR0_ADDR: return sh4_handle_dmatcr0_addr_r(mem_mask);
	case SH4_DMATCR1_ADDR: return sh4_handle_dmatcr1_addr_r(mem_mask);
	case SH4_DMATCR2_ADDR: return sh4_handle_dmatcr2_addr_r(mem_mask);
	case SH4_DMATCR3_ADDR: return sh4_handle_dmatcr3_addr_r(mem_mask);
	case SH4_CHCR0_ADDR: return sh4_handle_chcr0_addr_r(mem_mask);
	case SH4_CHCR1_ADDR: return sh4_handle_chcr1_addr_r(mem_mask);
	case SH4_CHCR2_ADDR: return sh4_handle_chcr2_addr_r(mem_mask);
	case SH4_CHCR3_ADDR: return sh4_handle_chcr3_addr_r(mem_mask);
	case SH4_DMAOR_ADDR: return sh4_handle_dmaor_addr_r(mem_mask);

	case PDTRA:
		if (sh3.m_m[BCR2] & 1)
			return (io_read_dword(SH4_IOPORT_16) & ~sh3.m_ioport16_direction) | (sh3.m_m[PDTRA] & sh3.m_ioport16_direction);
		break;
	case PDTRB:
		if (sh3.m_m[BCR2] & 1)
			return (io_read_dword(SH4_IOPORT_4) & ~sh3.m_ioport4_direction) | (sh3.m_m[PDTRB] & sh3.m_ioport4_direction);
		break;

	case SCFSR2:
		return 0x60;
	}
	return sh3.m_m[offset];
}

static void sh4_set_frt_input(int state)
{
	if (sh3.m_cpu_type != CPU_TYPE_SH4)
		fatalerror("sh4_set_frt_input uses sh3.m_m[] with SH3\n");

	if(state == PULSE_LINE)
	{
		sh4_set_frt_input(ASSERT_LINE);
		sh4_set_frt_input(CLEAR_LINE);
		return;
	}

	if(sh3.m_frt_input == state) {
		return;
	}

	sh3.m_frt_input = state;

	if (sh3.m_cpu_type == CPU_TYPE_SH4)
	{
		if(sh3.m_m[5] & 0x8000) {
			if(state == CLEAR_LINE) {
				return;
			}
		} else {
			if(state == ASSERT_LINE) {
				return;
			}
		}
	}
	else
	{
		fatalerror("sh4_set_frt_input uses sh3.m_m[] with SH3\n");
	}

#if 0
	sh4_timer_resync();
	m_icr = m_frc;
	sh3.m_m[4] |= ICF;
	logerror("SH4 '%s': ICF activated (%x)\n", "sh3", sh3.m_pc & AM);
	sh4_recalc_irq();
#endif
}

static void sh4_set_irln_input(int value)
{
	if (sh3.m_irln == value)
		return;
	sh3.m_irln = value;
	sh3_set_input_line(SH4_IRLn, ASSERT_LINE);
	sh3_set_input_line(SH4_IRLn, CLEAR_LINE);
}

static void execute_set_input(int irqline, int state)
{
	if (sh3.m_cpu_type == CPU_TYPE_SH3)
	{

		if (irqline == INPUT_LINE_NMI)
		{
			fatalerror("SH3 NMI Unimplemented\n");
		}
		else
		{

			if (sh3.m_irq_line_state[irqline] == state)
				return;
			sh3.m_irq_line_state[irqline] = state;

			if( state == CLEAR_LINE )
			{
				LOG(("SH-4 '%s' cleared external irq IRL%d\n", "sh3", irqline));
				sh4_exception_unrequest(SH4_INTC_IRL0+irqline-SH4_IRL0);
			}
			else
			{
				LOG(("SH-4 '%s' assert external irq IRL%d\n", "sh3", irqline));
				sh4_exception_request(SH4_INTC_IRL0+irqline-SH4_IRL0);
			}

		}

	}
	else
	{
		int s;

		if (irqline == INPUT_LINE_NMI)
		{
			if (sh3.m_nmi_line_state == state)
				return;
			if (sh3.m_m[ICR] & 0x100)
			{
				if ((state == CLEAR_LINE) && (sh3.m_nmi_line_state == ASSERT_LINE))
				{
					LOG(("SH-4 '%s' assert nmi\n", "sh3"));
					sh4_exception_request(SH4_INTC_NMI);
					sh4_dmac_nmi();
				}
			}
			else
			{
				if ((state == ASSERT_LINE) && (sh3.m_nmi_line_state == CLEAR_LINE))
				{
					LOG(("SH-4 '%s' assert nmi\n", "sh3"));
					sh4_exception_request(SH4_INTC_NMI);
					sh4_dmac_nmi();
				}
			}
			if (state == CLEAR_LINE)
				sh3.m_m[ICR] ^= 0x8000;
			else
				sh3.m_m[ICR] |= 0x8000;
			sh3.m_nmi_line_state = state;
		}
		else
		{
			if (sh3.m_m[ICR] & 0x80)
			{
				if (irqline > SH4_IRL3)
					return;
				if (sh3.m_irq_line_state[irqline] == state)
					return;
				sh3.m_irq_line_state[irqline] = state;

				if( state == CLEAR_LINE )
				{
					LOG(("SH-4 '%s' cleared external irq IRL%d\n", "sh3", irqline));
					sh4_exception_unrequest(SH4_INTC_IRL0+irqline-SH4_IRL0);
				}
				else
				{
					LOG(("SH-4 '%s' assert external irq IRL%d\n", "sh3", irqline));
					sh4_exception_request(SH4_INTC_IRL0+irqline-SH4_IRL0);
				}
			}
			else
			{
				if (irqline != SH4_IRLn)
					return;
				if ((sh3.m_irln > 15) || (sh3.m_irln < 0))
					return;
				for (s = 0; s < 15; s++)
					sh4_exception_unrequest(SH4_INTC_IRLn0+s);
				if (sh3.m_irln < 15)
					sh4_exception_request(SH4_INTC_IRLn0+sh3.m_irln);
				LOG(("SH-4 '%s' IRLn0-IRLn3 level #%d\n", "sh3", sh3.m_irln));
			}
		}
		if (sh3.m_test_irq && (!sh3.m_delay))
			sh4_check_pending_irq("sh4_set_irq_line");
	}
}

static void sh4_parse_configuration()
{
	if(sh3.c_clock > 0)
	{
		switch((sh3.c_md2 << 2) | (sh3.c_md1 << 1) | (sh3.c_md0))
		{
		case 0:
			sh3.m_cpu_clock = sh3.c_clock;
			sh3.m_bus_clock = sh3.c_clock / 4;
			sh3.m_pm_clock = sh3.c_clock / 4;
			break;
		case 1:
			sh3.m_cpu_clock = sh3.c_clock;
			sh3.m_bus_clock = sh3.c_clock / 6;
			sh3.m_pm_clock = sh3.c_clock / 6;
			break;
		case 2:
			sh3.m_cpu_clock = sh3.c_clock;
			sh3.m_bus_clock = sh3.c_clock / 3;
			sh3.m_pm_clock = sh3.c_clock / 6;
			break;
		case 3:
			sh3.m_cpu_clock = sh3.c_clock;
			sh3.m_bus_clock = sh3.c_clock / 3;
			sh3.m_pm_clock = sh3.c_clock / 6;
			break;
		case 4:
			sh3.m_cpu_clock = sh3.c_clock;
			sh3.m_bus_clock = sh3.c_clock / 2;
			sh3.m_pm_clock = sh3.c_clock / 4;
			break;
		case 5:
			sh3.m_cpu_clock = sh3.c_clock;
			sh3.m_bus_clock = sh3.c_clock / 2;
			sh3.m_pm_clock = sh3.c_clock / 4;
			break;
		}
		sh3.m_is_slave = (~(sh3.c_md7)) & 1;
	}
	else
	{
		sh3.m_cpu_clock = 200000000;
		sh3.m_bus_clock = 100000000;
		sh3.m_pm_clock = 50000000;
		sh3.m_is_slave = 0;
	}
}

static UINT32 sh4_getsqremap(UINT32 address)
{
	if (!sh3.m_sh4_mmu_enabled)
		return address;
	else
	{
		int i;
		UINT32 topaddr = address&0xfff00000;

		for (i=0;i<64;i++)
		{
			UINT32 topcmp = sh3.m_sh4_tlb_address[i]&0xfff00000;
			if (topcmp==topaddr)
				return (address&0x000fffff) | ((sh3.m_sh4_tlb_data[i])&0xfff00000);
		}

	}

	return address;
}

static UINT64 sh4_tlb_r(offs_t offset, UINT64 mem_mask)
{
	int offs = offset*8;

	if (offs >= 0x01000000)
	{
		UINT8 i = (offs>>8)&63;
		return sh3.m_sh4_tlb_data[i];
	}
	else
	{
		UINT8 i = (offs>>8)&63;
		return sh3.m_sh4_tlb_address[i];
	}
}

static void sh4_tlb_w(offs_t offset, UINT64 data, UINT64 mem_mask)
{
	int offs = offset*8;

	if (offs >= 0x01000000)
	{
		UINT8 i = (offs>>8)&63;
		sh3.m_sh4_tlb_data[i]  = data&0xffffffff;
	}
	else
	{
		UINT8 i = (offs>>8)&63;
		sh3.m_sh4_tlb_address[i] = data&0xffffffff;
	}
}


static void sh3_internal_high_w(offs_t offset, UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_sh3internal_upper[offset]);

	switch (offset)
	{
		case SH3_ICR0_IPRA_ADDR:
			if (mem_mask & 0xffff0000)
			{
				logerror("'%s' (%08x): INTC internal write to %08x = %08x & %08x (SH3_ICR0_IPRA_ADDR - ICR0)\n","sh3", sh3.m_pc & AM,(offset *4)+SH3_UPPER_REGBASE,data,mem_mask);
			}

			if (mem_mask & 0x0000ffff)
			{
				logerror("'%s' (%08x): INTC internal write to %08x = %08x & %08x (SH3_ICR0_IPRA_ADDR - IPRA)\n","sh3", sh3.m_pc & AM,(offset *4)+SH3_UPPER_REGBASE,data,mem_mask);
				sh4_handler_ipra_w(data&0xffff,mem_mask&0xffff);
			}

			break;

		case SH3_IPRB_ADDR:
			logerror("'%s' (%08x): INTC internal write to %08x = %08x & %08x (SH3_IPRB_ADDR)\n","sh3", sh3.m_pc & AM,(offset *4)+SH3_UPPER_REGBASE,data,mem_mask);
		break;

		case SH3_TOCR_TSTR_ADDR:
			logerror("'%s' (%08x): TMU internal write to %08x = %08x & %08x (SH3_TOCR_TSTR_ADDR)\n","sh3", sh3.m_pc & AM,(offset *4)+SH3_UPPER_REGBASE,data,mem_mask);
			if (mem_mask&0xff000000)
			{
				sh4_handle_tocr_addr_w((data>>24)&0xffff, (mem_mask>>24)&0xff);
			}
			if (mem_mask&0x0000ff00)
			{
				sh4_handle_tstr_addr_w((data>>8)&0xff, (mem_mask>>8)&0xff);
			}
			if (mem_mask&0x00ff00ff)
			{
				fatalerror("SH3_TOCR_TSTR_ADDR unused bits accessed (write)\n");
			}
			break;
		case SH3_TCOR0_ADDR:  sh4_handle_tcor0_addr_w(data, mem_mask);break;
		case SH3_TCOR1_ADDR:  sh4_handle_tcor1_addr_w(data, mem_mask);break;
		case SH3_TCOR2_ADDR:  sh4_handle_tcor2_addr_w(data, mem_mask);break;
		case SH3_TCNT0_ADDR:  sh4_handle_tcnt0_addr_w(data, mem_mask);break;
		case SH3_TCNT1_ADDR:  sh4_handle_tcnt1_addr_w(data, mem_mask);break;
		case SH3_TCNT2_ADDR:  sh4_handle_tcnt2_addr_w(data, mem_mask);break;
		case SH3_TCR0_ADDR:   sh4_handle_tcr0_addr_w(data>>16, mem_mask>>16);break;
		case SH3_TCR1_ADDR:   sh4_handle_tcr1_addr_w(data>>16, mem_mask>>16);break;
		case SH3_TCR2_ADDR:   sh4_handle_tcr2_addr_w(data>>16, mem_mask>>16);break;
		case SH3_TCPR2_ADDR:  sh4_handle_tcpr2_addr_w(data,  mem_mask);break;

		default:
			logerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x (unk)\n","sh3", sh3.m_pc & AM,(offset *4)+SH3_UPPER_REGBASE,data,mem_mask);
			break;

	}

}

static UINT32 sh3_internal_high_r(offs_t offset, UINT32 mem_mask)
{
	UINT32 ret = 0;

	switch (offset)
	{
		case SH3_ICR0_IPRA_ADDR:
			logerror("'%s' (%08x): INTC internal read from %08x mask %08x (SH3_ICR0_IPRA_ADDR - %08x)\n","sh3", sh3.m_pc & AM,(offset *4)+SH3_UPPER_REGBASE,mem_mask, sh3.m_sh3internal_upper[offset]);
			return (sh3.m_sh3internal_upper[offset] & 0xffff0000) | (sh3.m_SH4_IPRA & 0xffff);

		case SH3_IPRB_ADDR:
			logerror("'%s' (%08x): INTC internal read from %08x mask %08x (SH3_IPRB_ADDR - %08x)\n","sh3", sh3.m_pc & AM,(offset *4)+SH3_UPPER_REGBASE,mem_mask, sh3.m_sh3internal_upper[offset]);
			return sh3.m_sh3internal_upper[offset];

		case SH3_TOCR_TSTR_ADDR:

			if (mem_mask&0xff00000)
			{
				ret |= (sh4_handle_tocr_addr_r(mem_mask)&0xff)<<24;
			}
			if (mem_mask&0x0000ff00)
			{
				ret |= (sh4_handle_tstr_addr_r(mem_mask)&0xff)<<8;
			}
			if (mem_mask&0x00ff00ff)
			{
				fatalerror("SH3_TOCR_TSTR_ADDR unused bits accessed (read)\n");
			}
			return ret;
		case SH3_TCOR0_ADDR:  return sh4_handle_tcor0_addr_r(mem_mask);
		case SH3_TCOR1_ADDR:  return sh4_handle_tcor1_addr_r(mem_mask);
		case SH3_TCOR2_ADDR:  return sh4_handle_tcor2_addr_r(mem_mask);
		case SH3_TCNT0_ADDR:  return sh4_handle_tcnt0_addr_r(mem_mask);
		case SH3_TCNT1_ADDR:  return sh4_handle_tcnt1_addr_r(mem_mask);
		case SH3_TCNT2_ADDR:  return sh4_handle_tcnt2_addr_r(mem_mask);
		case SH3_TCR0_ADDR:   return sh4_handle_tcr0_addr_r(mem_mask)<<16;
		case SH3_TCR1_ADDR:   return sh4_handle_tcr1_addr_r(mem_mask)<<16;
		case SH3_TCR2_ADDR:   return sh4_handle_tcr2_addr_r(mem_mask)<<16;
		case SH3_TCPR2_ADDR:  return sh4_handle_tcpr2_addr_r(mem_mask);

		case SH3_TRA_ADDR:
			logerror("'%s' (%08x): unmapped internal read from %08x mask %08x (SH3 TRA - %08x)\n","sh3", sh3.m_pc & AM,(offset *4)+SH3_UPPER_REGBASE,mem_mask, sh3.m_sh3internal_upper[offset]);
			return sh3.m_sh3internal_upper[offset];

		case SH3_EXPEVT_ADDR:
			logerror("'%s' (%08x): unmapped internal read from %08x mask %08x (SH3 EXPEVT - %08x)\n","sh3", sh3.m_pc & AM,(offset *4)+SH3_UPPER_REGBASE,mem_mask, sh3.m_sh3internal_upper[offset]);
			return sh3.m_sh3internal_upper[offset];

		case SH3_INTEVT_ADDR:
			logerror("'%s' (%08x): unmapped internal read from %08x mask %08x (SH3 INTEVT - %08x)\n","sh3", sh3.m_pc & AM,(offset *4)+SH3_UPPER_REGBASE,mem_mask, sh3.m_sh3internal_upper[offset]);
			fatalerror("INTEVT unsupported on SH3\n");

		default:
			logerror("'%s' (%08x): unmapped internal read from %08x mask %08x\n","sh3", sh3.m_pc & AM,(offset *4)+SH3_UPPER_REGBASE,mem_mask);
			return sh3.m_sh3internal_upper[offset];
	}
}

static UINT32 sh3_internal_r(offs_t offset, UINT32 mem_mask)
{
	if (offset<0x1000)
	{
		switch (offset)
		{
			case SH3_SAR0_ADDR: return sh4_handle_sar0_addr_r(mem_mask);
			case SH3_SAR1_ADDR: return sh4_handle_sar1_addr_r(mem_mask);
			case SH3_SAR2_ADDR: return sh4_handle_sar2_addr_r(mem_mask);
			case SH3_SAR3_ADDR: return sh4_handle_sar3_addr_r(mem_mask);
			case SH3_DAR0_ADDR: return sh4_handle_dar0_addr_r(mem_mask);
			case SH3_DAR1_ADDR: return sh4_handle_dar1_addr_r(mem_mask);
			case SH3_DAR2_ADDR: return sh4_handle_dar2_addr_r(mem_mask);
			case SH3_DAR3_ADDR: return sh4_handle_dar3_addr_r(mem_mask);
			case SH3_DMATCR0_ADDR: return sh4_handle_dmatcr0_addr_r(mem_mask);
			case SH3_DMATCR1_ADDR: return sh4_handle_dmatcr1_addr_r(mem_mask);
			case SH3_DMATCR2_ADDR: return sh4_handle_dmatcr2_addr_r(mem_mask);
			case SH3_DMATCR3_ADDR: return sh4_handle_dmatcr3_addr_r(mem_mask);
			case SH3_CHCR0_ADDR: return sh4_handle_chcr0_addr_r(mem_mask);
			case SH3_CHCR1_ADDR: return sh4_handle_chcr1_addr_r(mem_mask);
			case SH3_CHCR2_ADDR: return sh4_handle_chcr2_addr_r(mem_mask);
			case SH3_CHCR3_ADDR: return sh4_handle_chcr3_addr_r(mem_mask);
			case SH3_DMAOR_ADDR: return sh4_handle_dmaor_addr_r(mem_mask)<<16;

			case INTEVT2:
				{

					return sh3.m_sh3internal_lower[offset];
				}

			case IRR0_IRR1:
				{
					{
						if (mem_mask & 0xff000000)
						{
							logerror("'%s' (%08x): unmapped internal read from %08x mask %08x (IRR0)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,mem_mask);
							return sh3.m_sh3internal_lower[offset];
						}

						if (mem_mask & 0x0000ff00)
						{
							logerror("'%s' (%08x): unmapped internal read from %08x mask %08x (IRR1)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,mem_mask);
							return sh3.m_sh3internal_lower[offset];
						}

						fatalerror("'%s' (%08x): unmapped internal read from %08x mask %08x (IRR0/1 unused bits)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,mem_mask);
					}
				}

			case PADR_PBDR:
				{
					if (mem_mask & 0xffff0000)
					{

						return io_read_qword(SH3_PORT_A)<<24;
					}

					if (mem_mask & 0x0000ffff)
					{

						return io_read_qword(SH3_PORT_B)<<8;
					}
				}
				break;

			case PCDR_PDDR:
				{
					if (mem_mask & 0xffff0000)
					{

						return io_read_qword(SH3_PORT_C)<<24;
					}

					if (mem_mask & 0x0000ffff)
					{

						return io_read_qword(SH3_PORT_D)<<8;
					}
				}
				break;

			case PEDR_PFDR:
				{
					if (mem_mask & 0xffff0000)
					{

						return io_read_qword(SH3_PORT_E)<<24;
					}

					if (mem_mask & 0x0000ffff)
					{

						return io_read_qword(SH3_PORT_F)<<8;
					}
				}
				break;

			case PGDR_PHDR:
				{
					if (mem_mask & 0xffff0000)
					{

						return io_read_qword(SH3_PORT_G)<<24;
					}

					if (mem_mask & 0x0000ffff)
					{

						return io_read_qword(SH3_PORT_H)<<8;
					}
				}
				break;

			case PJDR_PKDR:
				{
					if (mem_mask & 0xffff0000)
					{

						return io_read_qword(SH3_PORT_J)<<24;
					}

					if (mem_mask & 0x0000ffff)
					{

						return io_read_qword(SH3_PORT_K)<<8;
					}
				}
				break;

			case PLDR_SCPDR:
				{
					if (mem_mask & 0xffff0000)
					{

						return io_read_qword(SH3_PORT_L)<<24;
					}

					if (mem_mask & 0x0000ffff)
					{
						logerror("'%s' (%08x): unmapped internal read from %08x mask %08x (SCPDR)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,mem_mask);

					}
				}
				break;

			case SCSMR2_SCBRR2:
				{
					if (mem_mask & 0xff000000)
					{
						logerror("'%s' (%08x): SCIF internal read from %08x mask %08x (SCSMR2 - Serial Mode Register 2)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,mem_mask);
						return sh3.m_sh3internal_lower[offset];
					}

					if (mem_mask & 0x0000ff00)
					{
						logerror("'%s' (%08x): SCIF internal read from %08x mask %08x (SCBRR2 - Bit Rate Register 2)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,mem_mask);
						return sh3.m_sh3internal_lower[offset];
					}
				}
				break;

			case SCSCR2_SCFTDR2:
				{
					if (mem_mask & 0xff000000)
					{
						logerror("'%s' (%08x): SCIF internal read from %08x mask %08x (SCSCR2 - Serial Control Register 2)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,mem_mask);
						return sh3.m_sh3internal_lower[offset];
					}

					if (mem_mask & 0x0000ff00)
					{
						logerror("'%s' (%08x): SCIF internal read from %08x mask %08x (SCFTDR2 - Transmit FIFO Data Register 2)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,mem_mask);
						return sh3.m_sh3internal_lower[offset];
					}
				}
				break;

			case SCSSR2_SCFRDR2:
				{
					if (mem_mask & 0xffff0000)
					{
						logerror("'%s' (%08x): SCIF internal read from %08x mask %08x (SCSSR2 - Serial Status Register 2)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,mem_mask);
						return sh3.m_sh3internal_lower[offset];
					}

					if (mem_mask & 0x0000ff00)
					{
						logerror("'%s' (%08x): SCIF internal read from %08x mask %08x (SCFRDR2 - Receive FIFO Data Register 2)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,mem_mask);
						return sh3.m_sh3internal_lower[offset];
					}
				}
				break;

			case SCFCR2_SCFDR2:
				{
					if (mem_mask & 0xff000000)
					{
						logerror("'%s' (%08x): SCIF internal read from %08x mask %08x (SCFCR2 - Fifo Control Register 2)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,mem_mask);
						return sh3.m_sh3internal_lower[offset];
					}

					if (mem_mask & 0x0000ffff)
					{
						logerror("'%s' (%08x): SCIF internal read from %08x mask %08x (SCFDR2 - Fifo Data Count Register 2)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,mem_mask);
						return sh3.m_sh3internal_lower[offset];
					}
				}
				break;

			default:
				{
					logerror("'%s' (%08x): unmapped internal read from %08x mask %08x\n",
						"sh3", sh3.m_pc & AM,
						(offset *4)+0x4000000,
						mem_mask);
				}
				break;

		}

	}
	else
	{
		logerror("'%s' (%08x): unmapped internal read from %08x mask %08x\n",
			"sh3", sh3.m_pc & AM,
			(offset *4)+0x4000000,
			mem_mask);
	}

	return 0;
}

static void sh3_internal_w(offs_t offset, UINT32 data, UINT32 mem_mask)
{
	if (offset<0x1000)
	{

		COMBINE_DATA(&sh3.m_sh3internal_lower[offset]);

		switch (offset)
		{
			case SH3_SAR0_ADDR: sh4_handle_sar0_addr_w(data,mem_mask);   break;
			case SH3_SAR1_ADDR: sh4_handle_sar1_addr_w(data,mem_mask);   break;
			case SH3_SAR2_ADDR: sh4_handle_sar2_addr_w(data,mem_mask);   break;
			case SH3_SAR3_ADDR: sh4_handle_sar3_addr_w(data,mem_mask);   break;
			case SH3_DAR0_ADDR: sh4_handle_dar0_addr_w(data,mem_mask);   break;
			case SH3_DAR1_ADDR: sh4_handle_dar1_addr_w(data,mem_mask);   break;
			case SH3_DAR2_ADDR: sh4_handle_dar2_addr_w(data,mem_mask);   break;
			case SH3_DAR3_ADDR: sh4_handle_dar3_addr_w(data,mem_mask);   break;
			case SH3_DMATCR0_ADDR: sh4_handle_dmatcr0_addr_w(data,mem_mask);   break;
			case SH3_DMATCR1_ADDR: sh4_handle_dmatcr1_addr_w(data,mem_mask);   break;
			case SH3_DMATCR2_ADDR: sh4_handle_dmatcr2_addr_w(data,mem_mask);   break;
			case SH3_DMATCR3_ADDR: sh4_handle_dmatcr3_addr_w(data,mem_mask);   break;
			case SH3_CHCR0_ADDR: sh4_handle_chcr0_addr_w(data,mem_mask);   break;
			case SH3_CHCR1_ADDR: sh4_handle_chcr1_addr_w(data,mem_mask);   break;
			case SH3_CHCR2_ADDR: sh4_handle_chcr2_addr_w(data,mem_mask);   break;
			case SH3_CHCR3_ADDR: sh4_handle_chcr3_addr_w(data,mem_mask);   break;
			case SH3_DMAOR_ADDR: sh4_handle_dmaor_addr_w(data>>16,mem_mask>>16);   break;

			case IRR0_IRR1:
				{
					{
						if (mem_mask & 0xff000000)
						{
							logerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x (IRR0)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);

							if (!(data & 0x01000000)) execute_set_input(0, CLEAR_LINE);
							if (!(data & 0x02000000)) execute_set_input(1, CLEAR_LINE);
							if (!(data & 0x04000000)) execute_set_input(2, CLEAR_LINE);
							if (!(data & 0x08000000)) execute_set_input(3, CLEAR_LINE);

						}
						if (mem_mask & 0x0000ff00)
						{
							logerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x (IRR1)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
						}
						if (mem_mask & 0x00ff00ff)
						{
							fatalerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x (IRR0/1 unused bits)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
						}
					}
				}
				break;

			case PINTER_IPRC:
				{
					if (mem_mask & 0xffff0000)
					{
						logerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x (PINTER)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}

					if (mem_mask & 0x0000ffff)
					{
						data &= 0xffff; mem_mask &= 0xffff;
						COMBINE_DATA(&sh3.m_SH4_IPRC);
						logerror("'%s' (%08x): INTC internal write to %08x = %08x & %08x (IPRC)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
						sh3.m_exception_priority[SH4_INTC_IRL0]     = INTPRI((sh3.m_SH4_IPRC & 0x000f)>>0, SH4_INTC_IRL0);
						sh3.m_exception_priority[SH4_INTC_IRL1]     = INTPRI((sh3.m_SH4_IPRC & 0x00f0)>>4, SH4_INTC_IRL1);
						sh3.m_exception_priority[SH4_INTC_IRL2]     = INTPRI((sh3.m_SH4_IPRC & 0x0f00)>>8, SH4_INTC_IRL2);
						sh3.m_exception_priority[SH4_INTC_IRL3]     = INTPRI((sh3.m_SH4_IPRC & 0xf000)>>12,SH4_INTC_IRL3);
						sh4_exception_recompute();
					}
				}
				break;

			case PCCR_PDCR:
				{
					if (mem_mask & 0xffff0000)
					{
						logerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x (PCCR)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}

					if (mem_mask & 0x0000ffff)
					{
						logerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x (PDCR)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}
				}
				break;

			case PECR_PFCR:
				{
					if (mem_mask & 0xffff0000)
					{
						logerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x (PECR)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}

					if (mem_mask & 0x0000ffff)
					{
						logerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x (PFCR)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}
				}
				break;

			case PGCR_PHCR:
				{
					if (mem_mask & 0xffff0000)
					{
						logerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x (PGCR)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}

					if (mem_mask & 0x0000ffff)
					{
						logerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x (PHCR)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}
				}
				break;

			case PJCR_PKCR:
				{
					if (mem_mask & 0xffff0000)
					{
						logerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x (PJCR)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}

					if (mem_mask & 0x0000ffff)
					{
						logerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x (PKCR)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}
				}
				break;

			case PLCR_SCPCR:
				{
					if (mem_mask & 0xffff0000)
					{
						logerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x (PLCR)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}

					if (mem_mask & 0x0000ffff)
					{
						logerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x (SCPCR)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}
				}
				break;

			case PADR_PBDR:
				{
					if (mem_mask & 0xffff0000)
					{
						io_write_qword(SH3_PORT_A, (data>>24)&0xff);

					}

					if (mem_mask & 0x0000ffff)
					{
						io_write_qword(SH3_PORT_B, (data>>8)&0xff);

					}
				}
				break;

			case PCDR_PDDR:
				{
					if (mem_mask & 0xffff0000)
					{
						io_write_qword(SH3_PORT_C, (data>>24)&0xff);

					}

					if (mem_mask & 0x0000ffff)
					{
						io_write_qword(SH3_PORT_D, (data>>8)&0xff);

					}
				}
				break;
			case PEDR_PFDR:
				{
					if (mem_mask & 0xffff0000)
					{
						io_write_qword(SH3_PORT_E, (data>>24)&0xff);

					}

					if (mem_mask & 0x0000ffff)
					{
						io_write_qword(SH3_PORT_F, (data>>8)&0xff);

					}
				}
				break;

			case PGDR_PHDR:
				{
					if (mem_mask & 0xffff0000)
					{
						io_write_qword(SH3_PORT_G, (data>>24)&0xff);

					}

					if (mem_mask & 0x0000ffff)
					{
						io_write_qword(SH3_PORT_H, (data>>8)&0xff);

					}
				}
				break;

			case PJDR_PKDR:
				{
					if (mem_mask & 0xffff0000)
					{
						io_write_qword(SH3_PORT_J, (data>>24)&0xff);

					}

					if (mem_mask & 0x0000ffff)
					{
						io_write_qword(SH3_PORT_K, (data>>8)&0xff);

					}
				}
				break;

			case SCSMR2_SCBRR2:
				{
					if (mem_mask & 0xff000000)
					{
						logerror("'%s' (%08x): SCIF internal write to %08x = %08x & %08x (SCSMR2 - Serial Mode Register 2)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}

					if (mem_mask & 0x0000ff00)
					{
						logerror("'%s' (%08x): SCIF internal write to %08x = %08x & %08x (SCBRR2 - Bit Rate Register 2)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}
				}
				break;

			case SCSCR2_SCFTDR2:
				{
					if (mem_mask & 0xff000000)
					{
						logerror("'%s' (%08x): SCIF internal write to %08x = %08x & %08x (SCSCR2 - Serial Control Register 2)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}

					if (mem_mask & 0x0000ff00)
					{
						logerror("'%s' (%08x): SCIF internal write to %08x = %08x & %08x (SCFTDR2 - Transmit FIFO Data Register 2)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}
				}
				break;

			case SCSSR2_SCFRDR2:
				{
					if (mem_mask & 0xffff0000)
					{
						logerror("'%s' (%08x): SCIF internal write to %08x = %08x & %08x (SCSSR2 - Serial Status Register 2)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}

					if (mem_mask & 0x0000ff00)
					{
						logerror("'%s' (%08x): SCIF internal write to %08x = %08x & %08x (SCFRDR2 - Receive FIFO Data Register 2)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}
				}
				break;

			case SCFCR2_SCFDR2:
				{
					if (mem_mask & 0xff000000)
					{
						logerror("'%s' (%08x): SCIF internal write to %08x = %08x & %08x (SCFCR2 - Fifo Control Register 2)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}

					if (mem_mask & 0x0000ffff)
					{
						logerror("'%s' (%08x): SCIF internal write to %08x = %08x & %08x (SCFDR2 - Fifo Data Count Register 2)\n","sh3", sh3.m_pc & AM,(offset *4)+0x4000000,data,mem_mask);
					}
				}
				break;

			default:
				{
					logerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x\n",
							"sh3", sh3.m_pc & AM,
							(offset *4)+0x4000000,
							data,
							mem_mask);
				}
				break;
		}

	}
	else
	{
		logerror("'%s' (%08x): unmapped internal write to %08x = %08x & %08x\n",
				"sh3", sh3.m_pc & AM,
				(offset *4)+0x4000000,
				data,
				mem_mask);
	}

}


static const int dmasize[8] = { 8, 1, 2, 4, 32, 0, 0, 0 };

static const int sh3_dmasize[4] = { 1, 2, 4, 16 };

static void sh4_dmac_callback(int param)
{
	int channel = param;

	LOG(("SH4 '%s': DMA %d complete\n", "sh3", channel));
	sh3.m_dma_timer_active[channel] = 0;
	switch (channel)
	{
	case 0:
		sh3.m_SH4_DMATCR0 = 0;
		sh3.m_SH4_CHCR0 |= CHCR_TE;
		if (sh3.m_SH4_CHCR0 & CHCR_IE)
			sh4_exception_request(SH4_INTC_DMTE0);
		break;
	case 1:
		sh3.m_SH4_DMATCR1 = 0;
		sh3.m_SH4_CHCR1 |= CHCR_TE;
		if (sh3.m_SH4_CHCR1 & CHCR_IE)
			sh4_exception_request(SH4_INTC_DMTE1);
		break;
	case 2:
		sh3.m_SH4_DMATCR2 = 0;
		sh3.m_SH4_CHCR2 |= CHCR_TE;
		if (sh3.m_SH4_CHCR2 & CHCR_IE)
			sh4_exception_request(SH4_INTC_DMTE2);
		break;
	case 3:
		sh3.m_SH4_DMATCR3 = 0;
		sh3.m_SH4_CHCR3 |= CHCR_TE;
		if (sh3.m_SH4_CHCR3 & CHCR_IE)
			sh4_exception_request(SH4_INTC_DMTE3);
		break;
	}
}

static int sh4_dma_transfer(int channel, int timermode, UINT32 chcr, UINT32 *sar, UINT32 *dar, UINT32 *dmatcr)
{
	int incs, incd, size;
	UINT32 src, dst, count;

	incd = (chcr & CHCR_DM) >> 14;
	incs = (chcr & CHCR_SM) >> 12;

	if (sh3.m_cpu_type == CPU_TYPE_SH4)
	{
		size = dmasize[(chcr & CHCR_TS) >> 4];
	}
	else
	{
		size = sh3_dmasize[(chcr >> 3) & 3];
	}

	if(incd == 3 || incs == 3)
	{
		logerror("SH4: DMA: bad increment values (%d, %d, %d, %04x)\n", incd, incs, size, chcr);
		return 0;
	}
	src   = *sar;
	dst   = *dar;
	count = *dmatcr;
	if (!count)
		count = 0x1000000;

	LOG(("SH4: DMA %d start %x, %x, %x, %04x, %d, %d, %d\n", channel, src, dst, count, chcr, incs, incd, size));

	if (timermode == 1)
	{
		sh3.m_dma_timer_active[channel] = 1;
		timer_adjust(sh3.m_dma_timer[channel],TIME_IN_CYCLES(2*count+1, cpu_getactivecpu()), channel,0);
	}
	else if (timermode == 2)
	{
		sh3.m_dma_timer_active[channel] = 1;
		timer_adjust(sh3.m_dma_timer[channel],0, channel,0);
	}

	src &= AM;
	dst &= AM;

	switch(size)
	{
	case 1:
		for(;count > 0; count --)
		{
			if(incs == 2)
				src --;
			if(incd == 2)
				dst --;
			program_write_byte(dst, program_read_byte(src));
			if(incs == 1)
				src ++;
			if(incd == 1)
				dst ++;
		}
		break;
	case 2:
		src &= ~1;
		dst &= ~1;
		for(;count > 0; count --)
		{
			if(incs == 2)
				src -= 2;
			if(incd == 2)
				dst -= 2;
			program_write_word(dst, program_read_word(src));
			if(incs == 1)
				src += 2;
			if(incd == 1)
				dst += 2;
		}
		break;
	case 8:
		src &= ~7;
		dst &= ~7;
		for(;count > 0; count --)
		{
			if(incs == 2)
				src -= 8;
			if(incd == 2)
				dst -= 8;
			program_write_qword(dst, program_read_qword(src));
			if(incs == 1)
				src += 8;
			if(incd == 1)
				dst += 8;

		}
		break;
	case 4:
		src &= ~3;
		dst &= ~3;
		for(;count > 0; count --)
		{
			if(incs == 2)
				src -= 4;
			if(incd == 2)
				dst -= 4;
			program_write_dword(dst, program_read_dword(src));
			if(incs == 1)
				src += 4;
			if(incd == 1)
				dst += 4;

		}
		break;
	case 32:
		src &= ~31;
		dst &= ~31;
		for(;count > 0; count --)
		{
			if(incs == 2)
				src -= 32;
			if(incd == 2)
				dst -= 32;
			program_write_qword(dst, program_read_qword(src));
			program_write_qword(dst+8, program_read_qword(src+8));
			program_write_qword(dst+16, program_read_qword(src+16));
			program_write_qword(dst+24, program_read_qword(src+24));
			if(incs == 1)
				src += 32;
			if(incd == 1)
				dst += 32;
		}
		break;
	}
	*sar    = (*sar & !AM) | src;
	*dar    = (*dar & !AM) | dst;
	*dmatcr = count;
	return 1;
}

static int sh4_dma_transfer_device(int channel, UINT32 chcr, UINT32 *sar, UINT32 *dar, UINT32 *dmatcr)
{
	int incs, incd, size, mod;
	UINT32 src, dst, count;

	incd = (chcr & CHCR_DM) >> 14;
	incs = (chcr & CHCR_SM) >> 12;

	if (sh3.m_cpu_type == CPU_TYPE_SH4)
	{
		size = dmasize[(chcr & CHCR_TS) >> 4];
	}
	else
	{
		size = sh3_dmasize[(chcr >> 3) & 3];
	}

	mod = ((chcr & CHCR_RS) >> 8);
	if (incd == 3 || incs == 3)
	{
		logerror("SH4: DMA: bad increment values (%d, %d, %d, %04x)\n", incd, incs, size, chcr);
		return 0;
	}
	src   = *sar;
	dst   = *dar;
	count = *dmatcr;
	if (!count)
		count = 0x1000000;

	LOG(("SH4: DMA %d start device<->memory %x, %x, %x, %04x, %d, %d, %d\n", channel, src, dst, count, chcr, incs, incd, size));

	sh3.m_dma_timer_active[channel] = 1;

	src &= AM;
	dst &= AM;

	sh3.m_dma_source[channel]=src;
	sh3.m_dma_destination[channel]=dst;
	sh3.m_dma_count[channel]=count;
	sh3.m_dma_wordsize[channel]=size;
	sh3.m_dma_source_increment[channel]=incs;
	sh3.m_dma_destination_increment[channel]=incd;
	sh3.m_dma_mode[channel]=mod;

	io_write_dword(SH4_IOPORT_DMA, channel | (mod << 16));
	return 1;
}

static void sh4_dmac_check(int channel)
{
	UINT32 dmatcr, chcr, sar, dar;

	switch (channel)
	{
	case 0:
		sar = sh3.m_SH4_SAR0;
		dar = sh3.m_SH4_DAR0;
		chcr = sh3.m_SH4_CHCR0;
		dmatcr = sh3.m_SH4_DMATCR0;
		break;
	case 1:
		sar = sh3.m_SH4_SAR1;
		dar = sh3.m_SH4_DAR1;
		chcr = sh3.m_SH4_CHCR1;
		dmatcr = sh3.m_SH4_DMATCR1;
		break;
	case 2:
		sar = sh3.m_SH4_SAR2;
		dar = sh3.m_SH4_DAR2;
		chcr = sh3.m_SH4_CHCR2;
		dmatcr = sh3.m_SH4_DMATCR2;
		break;
	case 3:
		sar = sh3.m_SH4_SAR3;
		dar = sh3.m_SH4_DAR3;
		chcr = sh3.m_SH4_CHCR3;
		dmatcr = sh3.m_SH4_DMATCR3;
		break;
	default:
		return;
	}
	if (chcr & sh3.m_SH4_DMAOR & DMAOR_DME)
	{
		if ((((chcr & CHCR_RS) >> 8) < 2) || (((chcr & CHCR_RS) >> 8) > 6))
			return;
		if (!sh3.m_dma_timer_active[channel] && !(chcr & CHCR_TE) && !(sh3.m_SH4_DMAOR & (DMAOR_AE | DMAOR_NMIF)))
		{
			if (((chcr & CHCR_RS) >> 8) > 3)
				sh4_dma_transfer(channel, 1, chcr, &sar, &dar, &dmatcr);
			else if ((sh3.m_SH4_DMAOR & DMAOR_DDT) == 0)
				sh4_dma_transfer_device(channel, chcr, &sar, &dar, &dmatcr);
		}
	}
	else
	{
		if (sh3.m_dma_timer_active[channel])
		{
			logerror("SH4: DMA %d cancelled in-flight but all data transferred", channel);
			timer_adjust(sh3.m_dma_timer[channel],TIME_NEVER, channel,0);
			sh3.m_dma_timer_active[channel] = 0;
		}
	}
}

static int sh4_dma_data(struct sh4_device_dma *s)
{
	UINT32 pos, len, siz;
	int channel = s->channel;
	void *data = s->buffer;

	if (!sh3.m_dma_timer_active[channel])
		return 0;

	if (sh3.m_dma_mode[channel] == 2)
	{

		len = sh3.m_dma_count[channel];
		if (s->length < len)
			len = s->length;
		siz = sh3.m_dma_wordsize[channel];
		for (pos = 0;pos < len;pos++) {
			switch (siz)
			{
			case 8:
				if (sh3.m_dma_source_increment[channel] == 2)
					sh3.m_dma_source[channel] -= 8;
				*(UINT64 *)data = program_read_qword(sh3.m_dma_source[channel] & ~7);
				if (sh3.m_dma_source_increment[channel] == 1)
					sh3.m_dma_source[channel] += 8;
				break;
			case 1:
				if (sh3.m_dma_source_increment[channel] == 2)
					sh3.m_dma_source[channel]--;
				*(UINT8 *)data = program_read_byte(sh3.m_dma_source[channel]);
				if (sh3.m_dma_source_increment[channel] == 1)
					sh3.m_dma_source[channel]++;
				break;
			case 2:
				if (sh3.m_dma_source_increment[channel] == 2)
					sh3.m_dma_source[channel] -= 2;
				*(UINT16 *)data = program_read_word(sh3.m_dma_source[channel] & ~1);
				if (sh3.m_dma_source_increment[channel] == 1)
					sh3.m_dma_source[channel] += 2;
				break;
			case 4:
				if (sh3.m_dma_source_increment[channel] == 2)
					sh3.m_dma_source[channel] -= 4;
				*(UINT32 *)data = program_read_dword(sh3.m_dma_source[channel] & ~3);
				if (sh3.m_dma_source_increment[channel] == 1)
					sh3.m_dma_source[channel] += 4;
				break;
			case 32:
				if (sh3.m_dma_source_increment[channel] == 2)
					sh3.m_dma_source[channel] -= 32;
				*(UINT64 *)data = program_read_qword(sh3.m_dma_source[channel] & ~31);
				*((UINT64 *)data+1) = program_read_qword((sh3.m_dma_source[channel] & ~31)+8);
				*((UINT64 *)data+2) = program_read_qword((sh3.m_dma_source[channel] & ~31)+16);
				*((UINT64 *)data+3) = program_read_qword((sh3.m_dma_source[channel] & ~31)+24);
				if (sh3.m_dma_source_increment[channel] == 1)
					sh3.m_dma_source[channel] += 32;
				break;
			}
			sh3.m_dma_count[channel]--;
		}
		if (sh3.m_dma_count[channel] == 0)
		{
			timer_adjust(sh3.m_dma_timer[channel],0, channel,0);
			return 2;
		}
		return 1;
	}
	else if (sh3.m_dma_mode[channel] == 3)
	{

		len = sh3.m_dma_count[channel];
		if (s->length < len)
			len = s->length;
		siz = sh3.m_dma_wordsize[channel];
		for (pos = 0;pos < len;pos++) {
			switch (siz)
			{
			case 8:
				if (sh3.m_dma_destination_increment[channel] == 2)
					sh3.m_dma_destination[channel]-=8;
				program_write_qword(sh3.m_dma_destination[channel] & ~7, *(UINT64 *)data);
				if (sh3.m_dma_destination_increment[channel] == 1)
					sh3.m_dma_destination[channel]+=8;
				break;
			case 1:
				if (sh3.m_dma_destination_increment[channel] == 2)
					sh3.m_dma_destination[channel]--;
				program_write_byte(sh3.m_dma_destination[channel], *(UINT8 *)data);
				if (sh3.m_dma_destination_increment[channel] == 1)
					sh3.m_dma_destination[channel]++;
				break;
			case 2:
				if (sh3.m_dma_destination_increment[channel] == 2)
					sh3.m_dma_destination[channel]-=2;
				program_write_word(sh3.m_dma_destination[channel] & ~1, *(UINT16 *)data);
				if (sh3.m_dma_destination_increment[channel] == 1)
					sh3.m_dma_destination[channel]+=2;
				break;
			case 4:
				if (sh3.m_dma_destination_increment[channel] == 2)
					sh3.m_dma_destination[channel]-=4;
				program_write_dword(sh3.m_dma_destination[channel] & ~3, *(UINT32 *)data);
				if (sh3.m_dma_destination_increment[channel] == 1)
					sh3.m_dma_destination[channel]+=4;
				break;
			case 32:
				if (sh3.m_dma_destination_increment[channel] == 2)
					sh3.m_dma_destination[channel]-=32;
				program_write_qword(sh3.m_dma_destination[channel] & ~31, *(UINT64 *)data);
				program_write_qword((sh3.m_dma_destination[channel] & ~31)+8, *((UINT64 *)data+1));
				program_write_qword((sh3.m_dma_destination[channel] & ~31)+16, *((UINT64 *)data+2));
				program_write_qword((sh3.m_dma_destination[channel] & ~31)+24, *((UINT64 *)data+3));
				if (sh3.m_dma_destination_increment[channel] == 1)
					sh3.m_dma_destination[channel]+=32;
				break;
			}
			sh3.m_dma_count[channel]--;
		}

		if (sh3.m_dma_count[channel] == 0)
		{
			timer_adjust(sh3.m_dma_timer[channel],0, channel,0);
			return 2;
		}
		return 1;
	}
	else
		return 0;
}

static void sh4_dma_ddt(struct sh4_ddt_dma *s)
{
	UINT32 chcr;
	UINT32 *p32bits;
	UINT64 *p32bytes;
	UINT32 pos,len,siz;

	if (sh3.m_cpu_type != CPU_TYPE_SH4)
		fatalerror("sh4_dma_ddt uses sh3.m_m[] with SH3\n");

	if (sh3.m_dma_timer_active[s->channel])
		return;
	if (s->mode >= 0) {
		switch (s->channel)
		{
		case 0:
			if (s->mode & 1)
				s->source = sh3.m_SH4_SAR0;
			if (s->mode & 2)
				sh3.m_SH4_SAR0 = s->source;
			if (s->mode & 4)
				s->destination = sh3.m_SH4_DAR0;
			if (s->mode & 8)
				sh3.m_SH4_DAR0 = s->destination;
			break;
		case 1:
			if (s->mode & 1)
				s->source = sh3.m_SH4_SAR1;
			if (s->mode & 2)
				sh3.m_SH4_SAR1 = s->source;
			if (s->mode & 4)
				s->destination = sh3.m_SH4_DAR1;
			if (s->mode & 8)
				sh3.m_SH4_DAR1 = s->destination;
			break;
		case 2:
			if (s->mode & 1)
				s->source = sh3.m_SH4_SAR2;
			if (s->mode & 2)
				sh3.m_SH4_SAR2 = s->source;
			if (s->mode & 4)
				s->destination = sh3.m_SH4_DAR2;
			if (s->mode & 8)
				sh3.m_SH4_DAR2 = s->destination;
			break;
		case 3:
		default:
			if (s->mode & 1)
				s->source = sh3.m_SH4_SAR3;
			if (s->mode & 2)
				sh3.m_SH4_SAR3 = s->source;
			if (s->mode & 4)
				s->destination = sh3.m_SH4_DAR3;
			if (s->mode & 8)
				sh3.m_SH4_DAR3 = s->destination;
			break;
		}
		switch (s->channel)
		{
		case 0:
			chcr = sh3.m_SH4_CHCR0;
			len = sh3.m_SH4_DMATCR0;
			break;
		case 1:
			chcr = sh3.m_SH4_CHCR1;
			len = sh3.m_SH4_DMATCR1;
			break;
		case 2:
			chcr = sh3.m_SH4_CHCR2;
			len = sh3.m_SH4_DMATCR2;
			break;
		case 3:
		default:
			chcr = sh3.m_SH4_CHCR3;
			len = sh3.m_SH4_DMATCR3;
			break;
		}
		if ((s->direction) == 0) {
			chcr = (chcr & 0xffff3fff) | ((s->mode & 0x30) << 10);
		} else {
			chcr = (chcr & 0xffffcfff) | ((s->mode & 0x30) << 8);
		}

		if (sh3.m_cpu_type == CPU_TYPE_SH4)
		{

			siz = dmasize[(chcr >> 4) & 7];
		}
		else
		{
			siz = sh3_dmasize[(chcr >> 3) & 3];
		}

		if (siz && (s->size))
			if ((len * siz) != (s->length * s->size))
				return;
		sh4_dma_transfer(s->channel, 0, chcr, &s->source, &s->destination, &len);
	} else {
		if (s->size == 4) {
			if ((s->direction) == 0) {
				len = s->length;
				p32bits = (UINT32 *)(s->buffer);
				for (pos = 0;pos < len;pos++) {
					*p32bits = program_read_dword(s->source);
					p32bits++;
					s->source = s->source + 4;
				}
			} else {
				len = s->length;
				p32bits = (UINT32 *)(s->buffer);
				for (pos = 0;pos < len;pos++) {
					program_write_dword(s->destination, *p32bits);
					p32bits++;
					s->destination = s->destination + 4;
				}
			}
		}
		if (s->size == 32) {
			if ((s->direction) == 0) {
				len = s->length * 4;
				p32bytes = (UINT64 *)(s->buffer);
				for (pos = 0;pos < len;pos++) {
					*p32bytes = program_read_qword(s->source);
					p32bytes++;
					s->destination = s->destination + 8;
				}
			} else {
				len = s->length * 4;
				p32bytes = (UINT64 *)(s->buffer);
				for (pos = 0;pos < len;pos++) {
					program_write_qword(s->destination, *p32bytes);
					p32bytes++;
					s->destination = s->destination + 8;
				}
			}
		}
	}
}

	static void sh4_handle_sar0_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_SAR0);
}

	static void sh4_handle_sar1_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_SAR1);
}

	static void sh4_handle_sar2_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_SAR2);
}

	static void sh4_handle_sar3_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_SAR3);
}

	static void sh4_handle_dar0_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_DAR0);
}

	static void sh4_handle_dar1_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_DAR1);
}

	static void sh4_handle_dar2_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_DAR2);
}

	static void sh4_handle_dar3_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_DAR3);
}

	static void sh4_handle_dmatcr0_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_DMATCR0);
}

	static void sh4_handle_dmatcr1_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_DMATCR1);
}

	static void sh4_handle_dmatcr2_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_DMATCR2);
}

	static void sh4_handle_dmatcr3_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_DMATCR3);
}

	static void sh4_handle_chcr0_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_CHCR0);
	sh4_dmac_check(0);
}

	static void sh4_handle_chcr1_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_CHCR1);
	sh4_dmac_check(1);
}

	static void sh4_handle_chcr2_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_CHCR2);
	sh4_dmac_check(2);
}

	static void sh4_handle_chcr3_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_CHCR3);
	sh4_dmac_check(3);
}

	static void sh4_handle_dmaor_addr_w(UINT32 data, UINT32 mem_mask)
{
	UINT32 old = sh3.m_SH4_DMAOR;
	COMBINE_DATA(&sh3.m_SH4_DMAOR);

	if ((sh3.m_SH4_DMAOR & DMAOR_AE) && (~old & DMAOR_AE))
		sh3.m_SH4_DMAOR &= ~DMAOR_AE;
	if ((sh3.m_SH4_DMAOR & DMAOR_NMIF) && (~old & DMAOR_NMIF))
		sh3.m_SH4_DMAOR &= ~DMAOR_NMIF;
	sh4_dmac_check(0);
	sh4_dmac_check(1);
	sh4_dmac_check(2);
	sh4_dmac_check(3);
}


static const int tcnt_div[8] = { 4, 16, 64, 256, 1024, 1, 1, 1 };

static double sh4_scale_up_mame_time(double _time1, UINT32 factor1)
{
	return _time1 * factor1 + _time1;
}

static UINT32 compute_ticks_timer(mame_timer *timer, int hertz, int divisor)
{
	double ret;

	ret=((timer_timeleft(timer) * (double)hertz) / (double)divisor) - 1;
	return (UINT32)ret;
}

static void sh4_timer_recompute(int which)
{
	double ticks;

	UINT32 tcnt = 0;
	UINT32 tcr = 0;
	switch (which)
	{
		case 0:
			tcr = sh3.m_SH4_TCR0;
			tcnt = sh3.m_SH4_TCNT0;
			break;

		case 1:
			tcr = sh3.m_SH4_TCR1;
			tcnt = sh3.m_SH4_TCNT1;
			break;

		case 2:
			tcr = sh3.m_SH4_TCR2;
			tcnt = sh3.m_SH4_TCNT2;
			break;
	}

	ticks = tcnt;
	timer_adjust(sh3.m_timer[which],sh4_scale_up_mame_time(TIME_IN_HZ(sh3.m_pm_clock) * tcnt_div[tcr & 7], ticks), which,0);
}

static void sh4_timer_callback(int param)
{
	int which = param;

	switch (which)
	{
		case 0:
			sh3.m_SH4_TCNT0 = sh3.m_SH4_TCOR0;
			break;

		case 1:
			sh3.m_SH4_TCNT1 = sh3.m_SH4_TCOR1;
			break;

		case 2:
			sh3.m_SH4_TCNT2 = sh3.m_SH4_TCOR2;
			break;

	}

	sh4_timer_recompute(which);

	switch (which)
	{
		case 0:
			sh3.m_SH4_TCR0 |= 0x100;
			break;

		case 1:
			sh3.m_SH4_TCR1 |= 0x100;
			break;

		case 2:
			sh3.m_SH4_TCR2 |= 0x100;
			break;

	}

	switch (which)
	{
		case 0:
			if (sh3.m_SH4_TCR0 & 0x20)
			{
				sh4_exception_request(SH4_INTC_TUNI0);

			}
			break;

		case 1:
			if (sh3.m_SH4_TCR1 & 0x20)
			{
				sh4_exception_request(SH4_INTC_TUNI1);

			}
			break;

		case 2:
			if (sh3.m_SH4_TCR2 & 0x20)
			{
				sh4_exception_request(SH4_INTC_TUNI2);

			}
			break;

	}
}

static UINT32 sh4_handle_tcnt0_addr_r(UINT32 mem_mask)
{
	if (sh3.m_SH4_TSTR & 1)
		return compute_ticks_timer(sh3.m_timer[0], sh3.m_pm_clock, tcnt_div[sh3.m_SH4_TCR0 & 7]);
	else
		return sh3.m_SH4_TCNT0;
}

static UINT32 sh4_handle_tcnt1_addr_r(UINT32 mem_mask)
{
	if (sh3.m_SH4_TSTR & 2)
		return compute_ticks_timer(sh3.m_timer[1], sh3.m_pm_clock, tcnt_div[sh3.m_SH4_TCR1 & 7]);
	else
		return sh3.m_SH4_TCNT1;
}

static UINT32 sh4_handle_tcnt2_addr_r(UINT32 mem_mask)
{
	if (sh3.m_SH4_TSTR & 4)
		return compute_ticks_timer(sh3.m_timer[2], sh3.m_pm_clock, tcnt_div[sh3.m_SH4_TCR2 & 7]);
	else
		return sh3.m_SH4_TCNT2;
}

static UINT32 sh4_handle_tcor0_addr_r(UINT32 mem_mask)
{
	return sh3.m_SH4_TCOR0;
}

static UINT32 sh4_handle_tcor1_addr_r(UINT32 mem_mask)
{
	return sh3.m_SH4_TCOR1;
}

static UINT32 sh4_handle_tcor2_addr_r(UINT32 mem_mask)
{
	return sh3.m_SH4_TCOR2;
}

static UINT32 sh4_handle_tcr0_addr_r(UINT32 mem_mask)
{
	return sh3.m_SH4_TCR0;
}

static UINT32 sh4_handle_tcr1_addr_r(UINT32 mem_mask)
{
	return sh3.m_SH4_TCR1;
}

static UINT32 sh4_handle_tcr2_addr_r(UINT32 mem_mask)
{
	return sh3.m_SH4_TCR2;
}

static UINT32 sh4_handle_tstr_addr_r(UINT32 mem_mask)
{
	return sh3.m_SH4_TSTR;
}

static UINT32 sh4_handle_tocr_addr_r(UINT32 mem_mask)
{
	return sh3.m_SH4_TOCR;
}

static UINT32 sh4_handle_tcpr2_addr_r(UINT32 mem_mask)
{
	return sh3.m_SH4_TCPR2;
}

static void sh4_handle_tstr_addr_w(UINT32 data, UINT32 mem_mask)
{
	UINT32 old2 = sh3.m_SH4_TSTR;
	COMBINE_DATA(&sh3.m_SH4_TSTR);

	if (old2 & 1)
		sh3.m_SH4_TCNT0 = compute_ticks_timer(sh3.m_timer[0], sh3.m_pm_clock, tcnt_div[sh3.m_SH4_TCR0 & 7]);
	if ((sh3.m_SH4_TSTR & 1) == 0) {
		timer_adjust(sh3.m_timer[0],TIME_NEVER,0,0);
	} else
		sh4_timer_recompute(0);

	if (old2 & 2)
		sh3.m_SH4_TCNT1 = compute_ticks_timer(sh3.m_timer[1], sh3.m_pm_clock, tcnt_div[sh3.m_SH4_TCR1 & 7]);
	if ((sh3.m_SH4_TSTR & 2) == 0) {
		timer_adjust(sh3.m_timer[1],TIME_NEVER,0,0);
	} else
		sh4_timer_recompute(1);

	if (old2 & 4)
		sh3.m_SH4_TCNT2 = compute_ticks_timer(sh3.m_timer[2], sh3.m_pm_clock, tcnt_div[sh3.m_SH4_TCR2 & 7]);
	if ((sh3.m_SH4_TSTR & 4) == 0) {
		timer_adjust(sh3.m_timer[2],TIME_NEVER,0,0);
	} else
		sh4_timer_recompute(2);
}

static void sh4_handle_tcr0_addr_w(UINT32 data, UINT32 mem_mask)
{
	UINT32 old2 = sh3.m_SH4_TCR0;
	COMBINE_DATA(&sh3.m_SH4_TCR0);
	if (sh3.m_SH4_TSTR & 1)
	{
		sh3.m_SH4_TCNT0 = compute_ticks_timer(sh3.m_timer[0], sh3.m_pm_clock, tcnt_div[old2 & 7]);
		sh4_timer_recompute(0);
	}
	if (!(sh3.m_SH4_TCR0 & 0x20) || !(sh3.m_SH4_TCR0 & 0x100))
		sh4_exception_unrequest(SH4_INTC_TUNI0);
}

static void sh4_handle_tcr1_addr_w(UINT32 data, UINT32 mem_mask)
{
	UINT32 old2 = sh3.m_SH4_TCR1;
	COMBINE_DATA(&sh3.m_SH4_TCR1);
	if (sh3.m_SH4_TSTR & 2)
	{
		sh3.m_SH4_TCNT1 = compute_ticks_timer(sh3.m_timer[1], sh3.m_pm_clock, tcnt_div[old2 & 7]);
		sh4_timer_recompute(1);
	}
	if (!(sh3.m_SH4_TCR1 & 0x20) || !(sh3.m_SH4_TCR1 & 0x100))
		sh4_exception_unrequest(SH4_INTC_TUNI1);
}

static void sh4_handle_tcr2_addr_w(UINT32 data, UINT32 mem_mask)
{
	UINT32 old2 = sh3.m_SH4_TCR2;
	COMBINE_DATA(&sh3.m_SH4_TCR2);
	if (sh3.m_SH4_TSTR & 4)
	{
		sh3.m_SH4_TCNT2 = compute_ticks_timer(sh3.m_timer[2], sh3.m_pm_clock, tcnt_div[old2 & 7]);
		sh4_timer_recompute(2);
	}
	if (!(sh3.m_SH4_TCR2 & 0x20) || !(sh3.m_SH4_TCR2 & 0x100))
		sh4_exception_unrequest(SH4_INTC_TUNI2);
}

static void sh4_handle_tcor0_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_TCOR0);
	if (sh3.m_SH4_TSTR & 1)
	{
		sh3.m_SH4_TCNT0 = compute_ticks_timer(sh3.m_timer[0], sh3.m_pm_clock, tcnt_div[sh3.m_SH4_TCR0 & 7]);
		sh4_timer_recompute(0);
	}
}

static void sh4_handle_tcor1_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_TCOR1);
	if (sh3.m_SH4_TSTR & 2)
	{
		sh3.m_SH4_TCNT1 = compute_ticks_timer(sh3.m_timer[1], sh3.m_pm_clock, tcnt_div[sh3.m_SH4_TCR1 & 7]);
		sh4_timer_recompute(1);
	}
}

static void sh4_handle_tcor2_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_TCOR2);
	if (sh3.m_SH4_TSTR & 4)
	{
		sh3.m_SH4_TCNT2 = compute_ticks_timer(sh3.m_timer[2], sh3.m_pm_clock, tcnt_div[sh3.m_SH4_TCR2 & 7]);
		sh4_timer_recompute(2);
	}
}

static void sh4_handle_tcnt0_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_TCNT0);
	if (sh3.m_SH4_TSTR & 1)
		sh4_timer_recompute(0);
}

static void sh4_handle_tcnt1_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_TCNT1);
	if (sh3.m_SH4_TSTR & 2)
		sh4_timer_recompute(1);
}

static void sh4_handle_tcnt2_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_TCNT2);
	if (sh3.m_SH4_TSTR & 4)
		sh4_timer_recompute(2);
}

static void sh4_handle_tocr_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_TOCR);
}

static void sh4_handle_tcpr2_addr_w(UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(&sh3.m_SH4_TCPR2);
}

static offs_t sh3_dasm(char *buffer, offs_t pc)
{
#ifdef MAME_DEBUG
 extern unsigned DasmSH3(char *buffer, unsigned pc);
 return DasmSH3(buffer, pc);
#else
 sprintf(buffer, "$%04X", cpu_readop16(WORD2_XOR_BE(pc)));
 return 2;
#endif
}
static int sh3_cpu_index;
static void sh3_tmu_tick(int channel)
{
 cpuintrf_push_context(sh3_cpu_index);
 sh4_timer_callback(channel);
 cpuintrf_pop_context();
}
static void sh3_dma_tick(int channel)
{
 cpuintrf_push_context(sh3_cpu_index);
 sh4_dmac_callback(channel);
 cpuintrf_pop_context();
}
static void sh3_refresh_tick(int param)
{
 cpuintrf_push_context(sh3_cpu_index);
 sh4_refresh_timer_callback(param);
 cpuintrf_pop_context();
}
static void sh3_rtc_tick(int param)
{
 cpuintrf_push_context(sh3_cpu_index);
 sh4_rtc_timer_callback(param);
 cpuintrf_pop_context();
}
static void sh3_init(int index, int clock, const void *config, int (*callback)(int))
{
 int i;
 memset(&sh3, 0, sizeof(sh3));
 sh3_cpu_index = index;
 sh3_irq_callback = callback;
 sh3.c_clock = clock;
 sh3.c_md5 = sh3.c_md7 = 1;
 sh3.m_cpu_type = CPU_TYPE_SH3;
 sh4_parse_configuration();
 for (i = 0; i < 3; ++i) {
  sh3.m_timer[i] = timer_alloc(sh3_tmu_tick);
  timer_adjust(sh3.m_timer[i], TIME_NEVER, i, 0);
 }
 for (i = 0; i < 4; ++i) {
  sh3.m_dma_timer[i] = timer_alloc(sh3_dma_tick);
  timer_adjust(sh3.m_dma_timer[i], TIME_NEVER, i, 0);
 }
 sh3.m_refresh_timer = timer_alloc(sh3_refresh_tick);
 sh3.m_rtc_timer = timer_alloc(sh3_rtc_tick);
 timer_adjust(sh3.m_refresh_timer, TIME_NEVER, 0, 0);
 timer_adjust(sh3.m_rtc_timer, TIME_NEVER, 0, 0);
}
static void sh3_reset(void)
{
 int i;
 device_reset();
 for (i = 0; i < 3; ++i) timer_adjust(sh3.m_timer[i], TIME_NEVER, i, 0);
 for (i = 0; i < 4; ++i) timer_adjust(sh3.m_dma_timer[i], TIME_NEVER, i, 0);
 timer_adjust(sh3.m_refresh_timer, TIME_NEVER, 0, 0);
}
static void sh3_exit(void) {}
static void sh3_get_context(void *dst) { if (dst) memcpy(dst, &sh3, sizeof(sh3)); }
static void sh3_set_context(void *src) { if (src) memcpy(&sh3, src, sizeof(sh3)); }
static int sh3_execute(int cycles)
{
 sh3.m_sh4_icount = cycles;
 execute_run();
 return cycles - sh3.m_sh4_icount;
}
static void sh3_set_input_line(int line, int state) { execute_set_input(line, state); }
static void sh3_set_info(UINT32 state, union cpuinfo *info)
{
 if (state >= CPUINFO_INT_INPUT_STATE && state < CPUINFO_INT_INPUT_STATE + MAX_INPUT_LINES)
  execute_set_input(state - CPUINFO_INT_INPUT_STATE, info->i);
 else if (state >= CPUINFO_INT_REGISTER + SH4_R0 && state <= CPUINFO_INT_REGISTER + SH4_R15)
  sh3.m_r[state - CPUINFO_INT_REGISTER - SH4_R0] = info->i;
 else switch (state) {
 case CPUINFO_INT_PC: case CPUINFO_INT_REGISTER + SH4_PC: sh3.m_pc = info->i; sh3.m_delay = 0; break;
 case CPUINFO_INT_SP: sh3.m_r[15] = info->i; break;
 case CPUINFO_INT_REGISTER + SH4_SR: sh3.m_sr = info->i; sh4_exception_recompute(); break;
 case CPUINFO_INT_REGISTER + SH4_PR: sh3.m_pr = info->i; break;
 case CPUINFO_INT_REGISTER + SH4_GBR: sh3.m_gbr = info->i; break;
 case CPUINFO_INT_REGISTER + SH4_VBR: sh3.m_vbr = info->i; break;
 }
}
void sh3be_get_info(UINT32 state, union cpuinfo *info)
{
 if (state >= CPUINFO_STR_REGISTER + SH4_R0 && state <= CPUINFO_STR_REGISTER + SH4_R15) {
  int reg = state - CPUINFO_STR_REGISTER - SH4_R0;
  sprintf(info->s = cpuintrf_temp_str(), "R%-2d :%08X", reg, sh3.m_r[reg]);
  return;
 }
 if (state >= CPUINFO_INT_INPUT_STATE && state < CPUINFO_INT_INPUT_STATE + 5) {
  info->i = sh3.m_irq_line_state[state - CPUINFO_INT_INPUT_STATE];
  return;
 }

 if (state >= CPUINFO_INT_REGISTER + SH4_R0 && state <= CPUINFO_INT_REGISTER + SH4_R15) {
  info->i = sh3.m_r[state - CPUINFO_INT_REGISTER - SH4_R0]; return;
 }
 switch (state) {
 case CPUINFO_INT_CONTEXT_SIZE: info->i = sizeof(sh3); break;
 case CPUINFO_INT_INPUT_LINES: info->i = 5; break;
 case CPUINFO_INT_ENDIANNESS: info->i = CPU_IS_BE; break;
 case CPUINFO_INT_CLOCK_DIVIDER: info->i = 1; break;
 case CPUINFO_INT_MIN_INSTRUCTION_BYTES: case CPUINFO_INT_MAX_INSTRUCTION_BYTES: info->i = 2; break;
 case CPUINFO_INT_MIN_CYCLES: info->i = 1; break;
 case CPUINFO_INT_MAX_CYCLES: info->i = 4; break;
 case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:
 case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO: info->i = 64; break;
 case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM:
 case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: info->i = 32; break;
 case CPUINFO_INT_PREVIOUSPC: info->i = sh3.m_ppc; break;
 case CPUINFO_INT_PC: case CPUINFO_INT_REGISTER + SH4_PC: info->i = sh3.m_pc; break;
 case CPUINFO_INT_SP: info->i = sh3.m_r[15]; break;
 case CPUINFO_INT_REGISTER + SH4_SR: info->i = sh3.m_sr; break;
 case CPUINFO_INT_REGISTER + SH4_PR: info->i = sh3.m_pr; break;
 case CPUINFO_INT_REGISTER + SH4_GBR: info->i = sh3.m_gbr; break;
 case CPUINFO_INT_REGISTER + SH4_VBR: info->i = sh3.m_vbr; break;
 case CPUINFO_INT_REGISTER + SH4_MACH: info->i = sh3.m_mach; break;
 case CPUINFO_INT_REGISTER + SH4_MACL: info->i = sh3.m_macl; break;
 case CPUINFO_INT_REGISTER + SH4_DBR: info->i = sh3.m_dbr; break;
 case CPUINFO_INT_REGISTER + SH4_EA: info->i = sh3.m_ea; break;
 case CPUINFO_INT_REGISTER + SH4_SPC: info->i = sh3.m_spc; break;
 case CPUINFO_INT_REGISTER + SH4_SSR: info->i = sh3.m_ssr; break;
 case CPUINFO_INT_REGISTER + SH4_SGR: info->i = sh3.m_sgr; break;
 case CPUINFO_PTR_INIT: info->init = sh3_init; break;
 case CPUINFO_PTR_RESET: info->reset = sh3_reset; break;
 case CPUINFO_PTR_EXIT: info->exit = sh3_exit; break;
 case CPUINFO_PTR_EXECUTE: info->execute = sh3_execute; break;
 case CPUINFO_PTR_DISASSEMBLE: info->disassemble = sh3_dasm; break;
 case CPUINFO_PTR_GET_CONTEXT: info->getcontext = sh3_get_context; break;
 case CPUINFO_PTR_SET_CONTEXT: info->setcontext = sh3_set_context; break;
 case CPUINFO_PTR_SET_INFO: info->setinfo = sh3_set_info; break;
 case CPUINFO_PTR_INSTRUCTION_COUNTER: info->icount = &sh3.m_sh4_icount; break;
 case CPUINFO_STR_REGISTER + SH4_PC: sprintf(info->s = cpuintrf_temp_str(), "PC  :%08X", sh3.m_pc); break;
 case CPUINFO_STR_REGISTER + SH4_SR: sprintf(info->s = cpuintrf_temp_str(), "SR  :%08X", sh3.m_sr); break;
 case CPUINFO_STR_REGISTER + SH4_PR: sprintf(info->s = cpuintrf_temp_str(), "PR  :%08X", sh3.m_pr); break;
 case CPUINFO_STR_REGISTER + SH4_GBR: sprintf(info->s = cpuintrf_temp_str(), "GBR :%08X", sh3.m_gbr); break;
 case CPUINFO_STR_REGISTER + SH4_VBR: sprintf(info->s = cpuintrf_temp_str(), "VBR :%08X", sh3.m_vbr); break;
 case CPUINFO_STR_REGISTER + SH4_MACH: sprintf(info->s = cpuintrf_temp_str(), "MACH:%08X", sh3.m_mach); break;
 case CPUINFO_STR_REGISTER + SH4_MACL: sprintf(info->s = cpuintrf_temp_str(), "MACL:%08X", sh3.m_macl); break;
 case CPUINFO_STR_REGISTER + SH4_DBR: sprintf(info->s = cpuintrf_temp_str(), "DBR :%08X", sh3.m_dbr); break;
 case CPUINFO_STR_REGISTER + SH4_EA: sprintf(info->s = cpuintrf_temp_str(), "EA  :%08X", sh3.m_ea); break;
 case CPUINFO_STR_REGISTER + SH4_SPC: sprintf(info->s = cpuintrf_temp_str(), "SPC :%08X", sh3.m_spc); break;
 case CPUINFO_STR_REGISTER + SH4_SSR: sprintf(info->s = cpuintrf_temp_str(), "SSR :%08X", sh3.m_ssr); break;
 case CPUINFO_STR_REGISTER + SH4_SGR: sprintf(info->s = cpuintrf_temp_str(), "SGR :%08X", sh3.m_sgr); break;
 case CPUINFO_STR_NAME: strcpy(info->s = cpuintrf_temp_str(), "SH-3 (big endian)"); break;
 case CPUINFO_STR_CORE_FAMILY: strcpy(info->s = cpuintrf_temp_str(), "Hitachi SH-3"); break;
 case CPUINFO_STR_CORE_VERSION: strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
 case CPUINFO_STR_CORE_FILE: strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
 }
}

static READ64_HANDLER(sh3_upper_r)
{
 UINT64 result = 0;
 if ((UINT32)(mem_mask >> 32) != 0xffffffff) result |= (UINT64)sh3_internal_high_r(offset * 2, ~(UINT32)(mem_mask >> 32)) << 32;
 if ((UINT32)mem_mask != 0xffffffff) result |= sh3_internal_high_r(offset * 2 + 1, ~(UINT32)mem_mask);
 return result;
}
static WRITE64_HANDLER(sh3_upper_w)
{
 if ((UINT32)(mem_mask >> 32) != 0xffffffff) sh3_internal_high_w(offset * 2, data >> 32, ~(UINT32)(mem_mask >> 32));
 if ((UINT32)mem_mask != 0xffffffff) sh3_internal_high_w(offset * 2 + 1, data, ~(UINT32)mem_mask);
}

static READ64_HANDLER(sh3_lower_r)
{
 UINT64 result = 0;
 if ((UINT32)(mem_mask >> 32) != 0xffffffff) result |= (UINT64)sh3_internal_r(offset * 2, ~(UINT32)(mem_mask >> 32)) << 32;
 if ((UINT32)mem_mask != 0xffffffff) result |= sh3_internal_r(offset * 2 + 1, ~(UINT32)mem_mask);
 return result;
}
static WRITE64_HANDLER(sh3_lower_w)
{
 if ((UINT32)(mem_mask >> 32) != 0xffffffff) sh3_internal_w(offset * 2, data >> 32, ~(UINT32)(mem_mask >> 32));
 if ((UINT32)mem_mask != 0xffffffff) sh3_internal_w(offset * 2 + 1, data, ~(UINT32)mem_mask);
}

void cv1k_sh3_install(void)
{
 memory_install_read64_handler(0, ADDRESS_SPACE_PROGRAM, 0xffffd000, 0xffffffff, 0, 0, sh3_upper_r);
 memory_install_write64_handler(0, ADDRESS_SPACE_PROGRAM, 0xffffd000, 0xffffffff, 0, 0, sh3_upper_w);
 memory_install_read64_handler(0, ADDRESS_SPACE_PROGRAM, 0x04000000, 0x04003fff, 0, 0, sh3_lower_r);
 memory_install_write64_handler(0, ADDRESS_SPACE_PROGRAM, 0x04000000, 0x04003fff, 0, 0, sh3_lower_w);
}

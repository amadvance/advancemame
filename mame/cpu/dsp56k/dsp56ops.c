// license:BSD-3-Clause
// copyright-holders:Andrew Gardner, Ryan Holtz

#define LOG_UNIMPLEMENTED   (1U << 1)
#define LOG_CHECKS          (1U << 2)

typedef enum addSubOpType { OP_ADD,
					OP_SUB,
					OP_OTHER } addSubOpType;

typedef enum dataType { DT_BYTE,
				DT_WORD,
				DT_DOUBLE_WORD,
				DT_LONG_WORD } dataType;

typedef struct typed_pointer
{
	void* addr;
	char  data_type;
} typed_pointer;

#define BITS(CUR,MASK) (dsp56156_op_mask(CUR,MASK))

static const int dsp56156_add_ovf_table[8] = { 0, 1, 0, 0, 0, 0, 1, 0 };
static const int dsp56156_sub_ovf_table[8] = { 0, 0, 0, 1, 1, 0, 0, 0 };

static size_t dsp56156_op_addsub_2 (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT8* cycles);
static size_t dsp56156_op_mac_1    (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT8* cycles);
static size_t dsp56156_op_macr_1   (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT8* cycles);
static size_t dsp56156_op_move_1   (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT8* cycles);
static size_t dsp56156_op_mpy_1    (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT8* cycles);
static size_t dsp56156_op_mpyr_1   (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT8* cycles);
static size_t dsp56156_op_tfr_2    (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT8* cycles);
static size_t dsp56156_op_mpy_2    (dsp56156_core* cpustate, const UINT16 op_byte, UINT8* cycles);
static size_t dsp56156_op_mac_2    (dsp56156_core* cpustate, const UINT16 op_byte, UINT8* cycles);
static size_t dsp56156_op_clr      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_add      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_move     (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_tfr      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_rnd      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_tst      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_inc      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_inc24    (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_or       (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_asr      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_asl      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_lsr      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_lsl      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_eor      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_subl     (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_sub      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_clr24    (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_sbc      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_cmp      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_neg      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_not      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_dec      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_dec24    (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_and      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_abs      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_ror      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_rol      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_cmpm     (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_mpy      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_mpyr     (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_mac      (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_macr     (dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles);
static size_t dsp56156_op_adc      (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_andi     (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_asl4     (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_asr4     (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_asr16    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_bfop     (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_bfop_1   (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_bfop_2   (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_bcc      (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_bcc_1    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_bcc_2    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_bra      (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_bra_1    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_bra_2    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_brkcc    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_bscc     (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_bscc_1   (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_bsr      (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_bsr_1    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_chkaau   (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_debug    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_debugcc  (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_div      (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_dmac     (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_do       (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_do_1     (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_do_2     (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_doforever(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_enddo    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_ext      (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_illegal  (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_imac     (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_impy     (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_jcc      (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_jcc_1    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_jmp      (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_jmp_1    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_jscc     (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_jscc_1   (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_jsr      (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_jsr_1    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_jsr_2    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_lea      (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_lea_1    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_macsuuu  (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_move_2   (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_movec    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_movec_1  (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_movec_2  (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_movec_3  (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_movec_4  (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_movec_5  (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_movei    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_movem    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_movem_1  (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_movem_2  (dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles);
static size_t dsp56156_op_movep    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_movep_1  (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_moves    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_mpysuuu  (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_negc     (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_nop      (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_norm     (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_ori      (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_rep      (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_rep_1    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_rep_2    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_repcc    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_reset    (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_rti      (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_rts      (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_stop     (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_swap     (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_swi      (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_tcc      (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_tfr2     (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_tfr3     (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_tst2     (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_wait     (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);
static size_t dsp56156_op_zero     (dsp56156_core* cpustate, const UINT16 op, UINT8* cycles);

static int dsp56156_value_is_unnormalized(dsp56156_core* cpustate, const UINT64 value);
static int dsp56156_value_is_extended(dsp56156_core* cpustate, const UINT64 value);

static void execute_register_to_register_data_move(dsp56156_core* cpustate, const UINT16 op, typed_pointer* d_register, UINT64* prev_accum_value);
static void execute_address_register_update(dsp56156_core* cpustate, const UINT16 op, typed_pointer* d_register, UINT64* prev_accum_value);
static void execute_x_memory_data_move (dsp56156_core* cpustate, const UINT16 op, typed_pointer* d_register, UINT64* prev_accum_value);
static void execute_x_memory_data_move2(dsp56156_core* cpustate, const UINT16 op, typed_pointer* d_register);
static void execute_dual_x_memory_data_read(dsp56156_core* cpustate, const UINT16 op, typed_pointer* d_register);
static void execute_x_memory_data_move_with_short_displacement(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2);

static UINT16 decode_BBB_bitmask(dsp56156_core* cpustate, UINT16 BBB, UINT16 *iVal);
static int      decode_cccc_table(dsp56156_core* cpustate, UINT16 cccc);
static void     decode_DDDDD_table(dsp56156_core* cpustate, UINT16 DDDDD, typed_pointer* ret);
static void     decode_DD_table(dsp56156_core* cpustate, UINT16 DD, typed_pointer* ret);
static void     decode_DDF_table(dsp56156_core* cpustate, UINT16 DD, UINT16 F, typed_pointer* src_ret, typed_pointer* dst_ret);
static void     decode_F_table(dsp56156_core* cpustate, UINT16 F, typed_pointer* ret);
static void     decode_h0hF_table(dsp56156_core* cpustate, UINT16 h0h, UINT16 F, typed_pointer* src_ret, typed_pointer* dst_ret);
static void     decode_HH_table(dsp56156_core* cpustate, UINT16 HH, typed_pointer* ret);
static void     decode_HHH_table(dsp56156_core* cpustate, UINT16 HHH, typed_pointer* ret);
static void     decode_IIII_table(dsp56156_core* cpustate, UINT16 IIII, typed_pointer* src_ret, typed_pointer* dst_ret, void* working);
static void     decode_JJJF_table(dsp56156_core* cpustate, UINT16 JJJ, UINT16 F, typed_pointer* src_ret, typed_pointer* dst_ret);
static void     decode_JJF_table(dsp56156_core* cpustate, UINT16 JJ, UINT16 F, typed_pointer* src_ret, typed_pointer* dst_ret);
static void     decode_JF_table(dsp56156_core* cpustate, UINT16 JJ, UINT16 F, typed_pointer* src_ret, typed_pointer* dst_ret);
static void     decode_KKK_table(dsp56156_core* cpustate, UINT16 KKK, typed_pointer* dst_ret1, typed_pointer* dst_ret2, void* working);
static void     decode_QQF_table(dsp56156_core* cpustate, UINT16 QQ, UINT16 F, void **S1, void **S2, void **D);
static void     decode_QQF_special_table(dsp56156_core* cpustate, UINT16 QQ, UINT16 F, void **S1, void **S2, void **D);
static void     decode_QQQF_table(dsp56156_core* cpustate, UINT16 QQQ, UINT16 F, void **S1, void **S2, void **D);
static void     decode_RR_table(dsp56156_core* cpustate, UINT16 RR, typed_pointer* ret);
static void     decode_TT_table(dsp56156_core* cpustate, UINT16 TT, typed_pointer* ret);
static void     decode_uuuuF_table(dsp56156_core* cpustate, UINT16 uuuu, UINT16 F, UINT8 add_sub_other, typed_pointer* src_ret, typed_pointer* dst_ret);
static void     decode_Z_table(dsp56156_core* cpustate, UINT16 Z, typed_pointer* ret);

static void     execute_m_table(dsp56156_core* cpustate, int x, UINT16 m);
static void     execute_mm_table(dsp56156_core* cpustate, UINT16 rnum, UINT16 mm);
static void     execute_MM_table(dsp56156_core* cpustate, UINT16 rnum, UINT16 MM);
static UINT16 execute_q_table(dsp56156_core* cpustate, int RR, UINT16 q);
static void     execute_z_table(dsp56156_core* cpustate, int RR, UINT16 z);

static UINT16 assemble_address_from_Pppppp_table(dsp56156_core* cpustate, UINT16 P, UINT16 ppppp);
static UINT16 assemble_address_from_IO_short_address(dsp56156_core* cpustate, UINT16 pp);

static void dsp56156_process_loop(dsp56156_core* cpustate);
static void dsp56156_process_rep(dsp56156_core* cpustate, size_t repSize);

static UINT16 dsp56156_op_mask(UINT16 op, UINT16 mask);

static UINT64 dsp56156_limit_long_to_long(dsp56156_core* cpustate, UINT64 val);

static void dsp56156_set_destination_value(dsp56156_core* cpustate, typed_pointer source, typed_pointer dest);

static void dsp56156_set_data_memory_value(dsp56156_core* cpustate, typed_pointer source, UINT32 destinationAddr);
static void dsp56156_set_program_memory_value(dsp56156_core* cpustate, typed_pointer source, UINT32 destinationAddr);

static size_t dsp56156_get_op_size(UINT16 op, UINT16 op2)
{

	if ((op & 0xe000) == 0x6000)
	{

		if (((op & 0xe080) == 0x6000) && ((op & 0x0094) != 0x0010))
		{
			return 1;
		}

		else if ((op & 0xe094) == 0x6084)
		{
			return 1;
		}

		else if ((op & 0xe094) == 0x6094)
		{
			return 0;
		}

		else if ((op & 0xe094) == 0x6010)
		{
			return 0;
		}

		else if ((op & 0xe097) == 0x6017)
		{
			return 0;
		}

		else if ((op & 0xe094) == 0x6080)
		{
			return 1;
		}

		else if ((op & 0xe094) == 0x6090)
		{
			return 0;
		}
	}

	else if ((op & 0xfe00) == 0x1600)
	{

		if ((op & 0xff00) == 0x1600)
		{
			return 0;
		}

		else if ((op & 0xff00) == 0x1700)
		{
			return 0;
		}
	}

	else
	{

		enum pType { kNoParallelDataMove,
						kRegisterToRegister,
						kAddressRegister,
						kXMemoryDataMove,
						kXMemoryDataMove2,
						kXMemoryDataMoveWithDisp };

		int parallelType = -1;

		if ((op & 0xff00) == 0x4a00)
		{
			parallelType = kNoParallelDataMove;
		}

		else if ((op & 0xf000) == 0x4000)
		{
			parallelType = kRegisterToRegister;
		}

		else if ((op & 0xf800) == 0x3000)
		{
			parallelType = kAddressRegister;
		}

		else if ((op & 0x8000) == 0x8000)
		{
			parallelType = kXMemoryDataMove;
		}

		else if ((op & 0xf000) == 0x5000)
		{
			parallelType = kXMemoryDataMove2;
		}

		else if ((op & 0xff00) == 0x0500)
		{

			if (((op2 & 0xfe20) != 0x0200) &&
				((op2 & 0xf810) != 0x3800) &&
				((op2 & 0x00ff) != 0x0011))
			{
				return 2;
			}
		}

		if (parallelType != -1)
		{

			if ((op & 0x00f7) == 0x0001)
			{
				return 1;
			}

			else if ((op & 0x00f0) == 0x0000)
			{
				return 1;
			}

			else if ((op & 0x00ff) == 0x0011)
			{
				return 1;
			}

			else if ((op & 0x00f0) == 0x0010)
			{
				return 1;
			}

			else if ((op & 0x00f7) == 0x0020)
			{
				return 1;
			}

			else if ((op & 0x00f7) == 0x0021)
			{
				return 1;
			}

			else if ((op & 0x00f7) == 0x0022)
			{
				return 1;
			}

			else if ((op & 0x00f7) == 0x0023)
			{
				return 1;
			}

			else if ((op & 0x00f4) == 0x0024)
			{
				return 1;
			}

			else if ((op & 0x00f7) == 0x0030)
			{
				return 1;
			}

			else if ((op & 0x00f7) == 0x0031)
			{
				return 1;
			}

			else if ((op & 0x00f7) == 0x0032)
			{
				return 1;
			}

			else if ((op & 0x00f7) == 0x0033)
			{
				return 1;
			}

			else if ((op & 0x00f4) == 0x0034)
			{
				return 1;
			}

			else if ((op & 0x00f7) == 0x0041)
			{
				return 1;
			}

			else if ((op & 0x00f0) == 0x0040)
			{
				return 1;
			}

			else if ((op & 0x00f7) == 0x0051)
			{
				return 1;
			}

			else if ((op & 0x00f6) == 0x0052)
			{
				return 1;
			}

			else if ((op & 0x00f0) == 0x0050)
			{
				return 1;
			}

			else if ((op & 0x00f7) == 0x0060)
			{
				return 1;
			}

			else if ((op & 0x00f7) == 0x0061)
			{
				return 1;
			}

			else if ((op & 0x00f7) == 0x0062)
			{
				return 1;
			}

			else if ((op & 0x00f7) == 0x0063)
			{
				return 1;
			}

			else if ((op & 0x00f4) == 0x0064)
			{
				return 1;
			}

			if ((op & 0x00f7) == 0x0071)
			{
				return 1;
			}

			else if ((op & 0x00f7) == 0x0072)
			{
				return 1;
			}

			else if ((op & 0x00f7) == 0x0073)
			{
				return 1;
			}

			else if ((op & 0x00f0) == 0x0070)
			{
				return 1;
			}

			else if ((op & 0x00b0) == 0x0080)
			{
				return 1;
			}

			else if ((op & 0x00b0) == 0x0090)
			{
				return 1;
			}

			else if ((op & 0x00b0) == 0x00a0)
			{
				return 1;
			}

			else if ((op & 0x00b0) == 0x00b0)
			{
				return 1;
			}
		}
	}

	if ((op & 0xfff6) == 0x1502)
	{
		return 1;
	}

	else if (((op & 0xf900) == 0x1800) & ((op & 0x0600) != 0x0000))
	{
		return 1;
	}

	else if ((op & 0xfff7) == 0x1531)
	{
		return 1;
	}

	else if ((op & 0xfff7) == 0x1530)
	{
		return 1;
	}

	else if ((op & 0xfff7) == 0x1570)
	{
		return 1;
	}

	else if (((op & 0xffc0) == 0x14c0) && ((op2 & 0x1f00) == 0x1200))
	{
		return 2;
	}

	else if (((op & 0xffe0) == 0x14a0) && ((op2 & 0x1f00) == 0x1200))
	{
		return 2;
	}

	else if (((op & 0xffe0) == 0x1480) && ((op2 & 0x1f00) == 0x1200))
	{
		return 2;
	}

	else if (((op & 0xffc0) == 0x14c0) && ((op2 & 0x1f00) == 0x0400))
	{
		return 2;
	}

	else if (((op & 0xffe0) == 0x14a0) && ((op2 & 0x1f00) == 0x0400))
	{
		return 2;
	}

	else if (((op & 0xffe0) == 0x1480) && ((op2 & 0x1f00) == 0x0400))
	{
		return 2;
	}

	else if (((op & 0xffc0) == 0x14c0) && ((op2 & 0x1f00) == 0x1800))
	{
		return 2;
	}

	else if (((op & 0xffe0) == 0x14a0) && ((op2 & 0x1f00) == 0x1800))
	{
		return 2;
	}

	else if (((op & 0xffe0) == 0x1480) && ((op2 & 0x1f00) == 0x1800))
	{
		return 2;
	}

	else if (((op & 0xffc0) == 0x1440) && ((op2 & 0x1f00) == 0x1000))
	{
		return 2;
	}

	else if (((op & 0xffe0) == 0x1420) && ((op2 & 0x1f00) == 0x1000))
	{
		return 2;
	}

	else if (((op & 0xffe0) == 0x1400) && ((op2 & 0x1f00) == 0x1000))
	{
		return 2;
	}

	else if (((op & 0xffc0) == 0x1440) && ((op2 & 0x1f00) == 0x0000))
	{
		return 2;
	}

	else if (((op & 0xffe0) == 0x1420) && ((op2 & 0x1f00) == 0x0000))
	{
		return 2;
	}

	else if (((op & 0xffe0) == 0x1400) && ((op2 & 0x1f00) == 0x0000))
	{
		return 2;
	}

	else if (((op & 0xff30) == 0x0730) && ((op2 & 0x0000) == 0x0000))
	{
		return 2;
	}

	else if ((op & 0xfc00) == 0x2c00)
	{
		return 1;
	}

	else if ((op & 0xff30) == 0x0720)
	{
		return 1;
	}

	else if (((op & 0xfffc) == 0x013c) && ((op2 & 0x0000) == 0x0000))
	{
		return 2;
	}

	else if ((op & 0xff00) == 0x0b00)
	{
		return 1;
	}

	else if ((op & 0xfffc) == 0x012c)
	{
		return 1;
	}

	else if ((op & 0xfff0) == 0x0110)
	{
		return 1;
	}

	else if (((op & 0xff30) == 0x0710) && ((op2 & 0x0000) == 0x0000))
	{
		return 2;
	}

	else if ((op & 0xff30) == 0x0700)
	{
		return 1;
	}

	else if (((op & 0xfffc) == 0x0138) && ((op2 & 0x0000) == 0x0000))
	{
		return 2;
	}

	else if ((op & 0xfffc) == 0x0128)
	{
		return 1;
	}

	else if ((op & 0xffff) == 0x0004)
	{
		return 1;
	}

	else if ((op & 0xffff) == 0x0001)
	{
		return 1;
	}

	else if ((op & 0xfff0) == 0x0050)
	{
		return 1;
	}

	else if ((op & 0xff94) == 0x1504)
	{
		return 1;
	}

	else if ((op & 0xffd0) == 0x1590)
	{
		return 1;
	}

	else if (((op & 0xffe0) == 0x00c0) && ((op2 & 0x0000) == 0x0000))
	{
		return 2;
	}

	else if (((op & 0xff00) == 0x0e00) && ((op2 & 0x0000) == 0x0000))
	{
		return 2;
	}

	else if (((op & 0xffe0) == 0x0400) && ((op2 & 0x0000) == 0x0000))
	{
		return 2;
	}

	else if (((op & 0xffff) == 0x0002) && ((op2 & 0x0000) == 0x0000))
	{
		return 2;
	}

	else if ((op & 0xffff) == 0x0009)
	{
		return 1;
	}

	else if ((op & 0xfff7) == 0x1552)
	{
		return 1;
	}

	else if ((op & 0xffff) == 0x000f)
	{
		return 1;
	}

	else if ((op & 0xfff0) == 0x15a0)
	{
		return 1;
	}

	else if ((op & 0xfff0) == 0x1580)
	{
		return 1;
	}

	else if (((op & 0xff30) == 0x0630) && ((op2 & 0x0000) == 0x0000))
	{
		return 2;
	}

	else if ((op & 0xff30) == 0x0620 )
	{
		return 1;
	}

	else if (((op & 0xfffc) == 0x0134) && ((op2 & 0x0000) == 0x0000))
	{
		return 2;
	}

	else if ((op & 0xfffc) == 0x0124)
	{
		return 1;
	}

	else if (((op & 0xff30) == 0x0610) && ((op2 & 0x0000) == 0x0000))
	{
		return 2;
	}

	else if ((op & 0xff30) == 0x0600)
	{
		return 1;
	}

	else if (((op & 0xfffc) == 0x0130) && ((op2 & 0x0000) == 0x0000))
	{
		return 2;
	}

	else if ((op & 0xff00) == 0x0a00)
	{
		return 1;
	}

	else if ((op & 0xfffc) == 0x0120)
	{
		return 1;
	}

	else if ((op & 0xffc0) == 0x01c0)
	{
		return 1;
	}

	else if ((op & 0xffc0) == 0x0180)
	{
		return 1;
	}

	else if ((op & 0xfff0) == 0x15e0)
	{
		return 1;
	}

	else if (((op & 0xff00) == 0x0500) && ((op2 & 0x00ff) == 0x0011))
	{
		return 2;
	}

	else if ((op & 0xf810) == 0x3800)
	{
		return 1;
	}

	else if ((op & 0xf814) == 0x3810)
	{
		return 1;
	}

	else if ((op & 0xf816) == 0x3816)
	{
		return 1;
	}

	else if (((op & 0xf816) == 0x3814) && ((op2 & 0x0000) == 0x0000))
	{
		return 2;
	}

	else if ((op & 0xfc00) == 0x2800)
	{
		return 1;
	}

	else if (((op & 0xff00) == 0x0500) && ((op2 & 0xf810) == 0x3800))
	{
		return 2;
	}

	else if ((op & 0xfc00) == 0x2000)
	{
		return 1;
	}

	else if ((op & 0xfe20) == 0x0200)
	{
		return 1;
	}

	else if ((op & 0xfe30) == 0x0230)
	{
		return 1;
	}

	else if (((op & 0xff00) == 0x0500) && ((op2 & 0xfe20) == 0x0200))
	{
		return 2;
	}

	else if ((op & 0xfe20) == 0x1820)
	{
		return 1;
	}

	else if ((op & 0xfe00) == 0x0c00)
	{
		return 1;
	}

	else if ((op & 0xfe20) == 0x1800)
	{
		return 1;
	}

	else if ((op & 0xfff0) == 0x15c0)
	{
		return 1;
	}

	else if ((op & 0xfff7) == 0x1560)
	{
		return 1;
	}

	else if ((op & 0xffff) == 0x0000)
	{
		return 1;
	}

	else if ((op & 0xfff4) == 0x1520)
	{
		return 1;
	}

	else if ((op & 0xf900) == 0x1900)
	{
		return 1;
	}

	else if ((op & 0xffe0) == 0x00e0)
	{
		return 1;
	}

	else if ((op & 0xff00) == 0x0f00)
	{
		return 1;
	}

	else if ((op & 0xffe0) == 0x0420)
	{
		return 1;
	}

	else if ((op & 0xfff0) == 0x0150)
	{
		return 1;
	}

	else if ((op & 0xffff) == 0x0008)
	{
		return 1;
	}

	else if ((op & 0xffff) == 0x0007)
	{
		return 1;
	}

	else if ((op & 0xffff) == 0x0006)
	{
		return 1;
	}

	else if ((op & 0xffff) == 0x000a)
	{
		return 1;
	}

	else if ((op & 0xfff7) == 0x1571)
	{
		return 1;
	}

	else if ((op & 0xffff) == 0x0005)
	{
		return 1;
	}

	else if ((op & 0xfc02) == 0x1000)
	{
		return 1;
	}

	else if ((op & 0xfff6) == 0x1500)
	{
		return 1;
	}

	else if ((op & 0xfc00) == 0x2400)
	{
		return 1;
	}

	else if ((op & 0xfff4) == 0x1514)
	{
		return 1;
	}

	else if ((op & 0xffff) == 0x000b)
	{
		return 1;
	}

	else if ((op & 0xfff7) == 0x1550)
	{
		return 1;
	}

	return 1;
}

static void execute_one(dsp56156_core* cpustate)
{
	size_t size = 0x1337;
	UINT8 cycle_count = 0;

	cpustate->ppc = PC;
	if (1)
		CALL_MAME_DEBUG;

	cpustate->op = ROPCODE(PC);

	UINT16 op = ROPCODE(PC);
	UINT16 op2 = ROPCODE(PC + 1);

	if ((op & 0xe000) == 0x6000)
	{
		typed_pointer d_register = {NULL, DT_BYTE};

		UINT16 op_byte = op & 0x00ff;

		if (((op & 0xe080) == 0x6000) && ((op & 0x0094) != 0x0010))
		{
			size = dsp56156_op_addsub_2(cpustate, op_byte, &d_register, &cycle_count);
		}

		else if ((op & 0xe094) == 0x6084)
		{
			size = dsp56156_op_mac_1(cpustate, op_byte, &d_register, &cycle_count);
		}

		else if ((op & 0xe094) == 0x6094)
		{
			size = dsp56156_op_macr_1(cpustate, op_byte, &d_register, &cycle_count);
		}

		else if ((op & 0xe094) == 0x6010)
		{
			size = dsp56156_op_tfr_2(cpustate, op_byte, &d_register, &cycle_count);
		}

		else if ((op & 0xe097) == 0x6017)
		{
			size = dsp56156_op_move_1(cpustate, op_byte, &d_register, &cycle_count);
		}

		else if ((op & 0xe094) == 0x6080)
		{
			size = dsp56156_op_mpy_1(cpustate, op_byte, &d_register, &cycle_count);
		}

		else if ((op & 0xe094) == 0x6090)
		{
			size = dsp56156_op_mpyr_1(cpustate, op_byte, &d_register, &cycle_count);
		}

		execute_dual_x_memory_data_read(cpustate, op, &d_register);
	}

	else if ((op & 0xfe00) == 0x1600)
	{

		UINT16 op_byte = op & 0x00ff;

		if ((op & 0xff00) == 0x1600)
		{
			size = dsp56156_op_mpy_2(cpustate, op_byte, &cycle_count);
		}

		else if ((op & 0xff00) == 0x1700)
		{
			size = dsp56156_op_mac_2(cpustate, op_byte, &cycle_count);
		}

		LOGMASKED(LOG_UNIMPLEMENTED, "DSP56156: Unemulated Dual X Memory Data And Register Data Move @ 0x%x\n", PC);
		fatalerror("Unsupported DSP56156 operation at %04x", PC);
	}

	else
	{

		enum pType { kNoParallelDataMove,
						kRegisterToRegister,
						kAddressRegister,
						kXMemoryDataMove,
						kXMemoryDataMove2,
						kXMemoryDataMoveWithDisp };

		int parallelType = -1;
		UINT16 op_byte = 0x0000;
		typed_pointer d_register = {NULL, DT_BYTE};
		UINT64 prev_accum_value = 0x0000000000000000ULL;

		if ((op & 0xff00) == 0x4a00)
		{
			op_byte = op & 0x00ff;
			parallelType = kNoParallelDataMove;
		}

		else if ((op & 0xf000) == 0x4000)
		{
			op_byte = op & 0x00ff;
			parallelType = kRegisterToRegister;
		}

		else if ((op & 0xf800) == 0x3000)
		{
			op_byte = op & 0x00ff;
			parallelType = kAddressRegister;
		}

		else if ((op & 0x8000) == 0x8000)
		{
			op_byte = op & 0x00ff;
			parallelType = kXMemoryDataMove;
		}

		else if ((op & 0xf000) == 0x5000)
		{
			op_byte = op & 0x00ff;
			parallelType = kXMemoryDataMove2;
		}

		else if ((op & 0xff00) == 0x0500)
		{

			if (((op2 & 0xfe20) != 0x0200) &&
				((op2 & 0xf810) != 0x3800) &&
				((op2 & 0x00ff) != 0x0011))
			{
				op_byte = op2 & 0x00ff;
				parallelType = kXMemoryDataMoveWithDisp;
			}
		}

		if (parallelType != -1)
		{

			if ((op_byte & 0x00f7) == 0x0001)
			{
				size = dsp56156_op_clr(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f0) == 0x0000)
			{
				size = dsp56156_op_add(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00ff) == 0x0011)
			{
				size = dsp56156_op_move(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f0) == 0x0010)
			{
				size = dsp56156_op_tfr(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f7) == 0x0020)
			{
				size = dsp56156_op_rnd(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f7) == 0x0021)
			{
				size = dsp56156_op_tst(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f7) == 0x0022)
			{
				size = dsp56156_op_inc(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f7) == 0x0023)
			{
				size = dsp56156_op_inc24(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f4) == 0x0024)
			{
				size = dsp56156_op_or(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f7) == 0x0030)
			{
				size = dsp56156_op_asr(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f7) == 0x0031)
			{
				size = dsp56156_op_asl(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f7) == 0x0032)
			{
				size = dsp56156_op_lsr(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f7) == 0x0033)
			{
				size = dsp56156_op_lsl(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f4) == 0x0034)
			{
				size = dsp56156_op_eor(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f7) == 0x0041)
			{
				size = dsp56156_op_subl(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f0) == 0x0040)
			{
				size = dsp56156_op_sub(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f7) == 0x0051)
			{
				size = dsp56156_op_clr24(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f6) == 0x0052)
			{
				size = dsp56156_op_sbc(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f0) == 0x0050)
			{
				size = dsp56156_op_cmp(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f7) == 0x0060)
			{
				size = dsp56156_op_neg(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f7) == 0x0061)
			{
				size = dsp56156_op_not(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f7) == 0x0062)
			{
				size = dsp56156_op_dec(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f7) == 0x0063)
			{
				size = dsp56156_op_dec24(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f4) == 0x0064)
			{
				size = dsp56156_op_and(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			if ((op_byte & 0x00f7) == 0x0071)
			{
				size = dsp56156_op_abs(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f7) == 0x0072)
			{
				size = dsp56156_op_ror(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f7) == 0x0073)
			{
				size = dsp56156_op_rol(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00f0) == 0x0070)
			{
				size = dsp56156_op_cmpm(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00b0) == 0x0080)
			{
				size = dsp56156_op_mpy(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00b0) == 0x0090)
			{
				size = dsp56156_op_mpyr(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00b0) == 0x00a0)
			{
				size = dsp56156_op_mac(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			else if ((op_byte & 0x00b0) == 0x00b0)
			{
				size = dsp56156_op_macr(cpustate, op_byte, &d_register, &prev_accum_value, &cycle_count);
			}

			switch (parallelType)
			{
			case kNoParallelDataMove:

				break;
			case kRegisterToRegister:
				execute_register_to_register_data_move(cpustate, op, &d_register, &prev_accum_value);
				break;
			case kAddressRegister:
				execute_address_register_update(cpustate, op, &d_register, &prev_accum_value);
				break;
			case kXMemoryDataMove:
				execute_x_memory_data_move(cpustate, op, &d_register, &prev_accum_value);
				break;
			case kXMemoryDataMove2:
				execute_x_memory_data_move2(cpustate, op, &d_register);
				break;
			case kXMemoryDataMoveWithDisp:
				execute_x_memory_data_move_with_short_displacement(cpustate, op, op2);
				size = 2;
				break;
			}
		}
	}

	if (size != 0x1337)
	{
		PC += (UINT16)size;

		dsp56156_process_loop(cpustate);
		dsp56156_process_rep(cpustate, size);

		cpustate->icount -= 4;
		return;
	}

	if ((op & 0xfff6) == 0x1502)
	{
		size = dsp56156_op_adc(cpustate, op, &cycle_count);
	}

	else if (((op & 0xf900) == 0x1800) & ((op & 0x0600) != 0x0000))
	{
		size = dsp56156_op_andi(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff7) == 0x1531)
	{
		size = dsp56156_op_asl4(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff7) == 0x1530)
	{
		size = dsp56156_op_asr4(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff7) == 0x1570)
	{
		size = dsp56156_op_asr16(cpustate, op, &cycle_count);
	}

	else if (((op & 0xffc0) == 0x14c0) && ((op2 & 0x1f00) == 0x1200))
	{
		size = dsp56156_op_bfop(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xffe0) == 0x14a0) && ((op2 & 0x1f00) == 0x1200))
	{
		size = dsp56156_op_bfop_1(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xffe0) == 0x1480) && ((op2 & 0x1f00) == 0x1200))
	{
		size = dsp56156_op_bfop_2(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xffc0) == 0x14c0) && ((op2 & 0x1f00) == 0x0400))
	{
		size = dsp56156_op_bfop(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xffe0) == 0x14a0) && ((op2 & 0x1f00) == 0x0400))
	{
		size = dsp56156_op_bfop_1(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xffe0) == 0x1480) && ((op2 & 0x1f00) == 0x0400))
	{
		size = dsp56156_op_bfop_2(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xffc0) == 0x14c0) && ((op2 & 0x1f00) == 0x1800))
	{
		size = dsp56156_op_bfop(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xffe0) == 0x14a0) && ((op2 & 0x1f00) == 0x1800))
	{
		size = dsp56156_op_bfop_1(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xffe0) == 0x1480) && ((op2 & 0x1f00) == 0x1800))
	{
		size = dsp56156_op_bfop_2(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xffc0) == 0x1440) && ((op2 & 0x1f00) == 0x1000))
	{
		size = dsp56156_op_bfop(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xffe0) == 0x1420) && ((op2 & 0x1f00) == 0x1000))
	{
		size = dsp56156_op_bfop_1(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xffe0) == 0x1400) && ((op2 & 0x1f00) == 0x1000))
	{
		size = dsp56156_op_bfop_2(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xffc0) == 0x1440) && ((op2 & 0x1f00) == 0x0000))
	{
		size = dsp56156_op_bfop(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xffe0) == 0x1420) && ((op2 & 0x1f00) == 0x0000))
	{
		size = dsp56156_op_bfop_1(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xffe0) == 0x1400) && ((op2 & 0x1f00) == 0x0000))
	{
		size = dsp56156_op_bfop_2(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xff30) == 0x0730) && ((op2 & 0x0000) == 0x0000))
	{
		size = dsp56156_op_bcc(cpustate, op, op2, &cycle_count);
	}

	else if ((op & 0xfc00) == 0x2c00)
	{
		size = dsp56156_op_bcc_1(cpustate, op, &cycle_count);
	}

	else if ((op & 0xff30) == 0x0720)
	{
		size = dsp56156_op_bcc_2(cpustate, op, &cycle_count);
	}

	else if (((op & 0xfffc) == 0x013c) && ((op2 & 0x0000) == 0x0000))
	{
		size = dsp56156_op_bra(cpustate, op, op2, &cycle_count);
	}

	else if ((op & 0xff00) == 0x0b00)
	{
		size = dsp56156_op_bra_1(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfffc) == 0x012c)
	{
		size = dsp56156_op_bra_2(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff0) == 0x0110)
	{
		size = dsp56156_op_brkcc(cpustate, op, &cycle_count);
	}

	else if (((op & 0xff30) == 0x0710) && ((op2 & 0x0000) == 0x0000))
	{
		size = dsp56156_op_bscc(cpustate, op, op2, &cycle_count);
	}

	else if ((op & 0xff30) == 0x0700)
	{
		size = dsp56156_op_bscc_1(cpustate, op, &cycle_count);
	}

	else if (((op & 0xfffc) == 0x0138) && ((op2 & 0x0000) == 0x0000))
	{
		size = dsp56156_op_bsr(cpustate, op, op2, &cycle_count);
	}

	else if ((op & 0xfffc) == 0x0128)
	{
		size = dsp56156_op_bsr_1(cpustate, op, &cycle_count);
	}

	else if ((op & 0xffff) == 0x0004)
	{
		size = dsp56156_op_chkaau(cpustate, op, &cycle_count);
	}

	else if ((op & 0xffff) == 0x0001)
	{
		size = dsp56156_op_debug(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff0) == 0x0050)
	{
		size = dsp56156_op_debugcc(cpustate, op, &cycle_count);
	}

	else if ((op & 0xff94) == 0x1504)
	{
		size = dsp56156_op_div(cpustate, op, &cycle_count);
	}

	else if ((op & 0xffd0) == 0x1590)
	{
		size = dsp56156_op_dmac(cpustate, op, &cycle_count);
	}

	else if (((op & 0xffe0) == 0x00c0) && ((op2 & 0x0000) == 0x0000))
	{
		size = dsp56156_op_do(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xff00) == 0x0e00) && ((op2 & 0x0000) == 0x0000))
	{
		size = dsp56156_op_do_1(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xffe0) == 0x0400) && ((op2 & 0x0000) == 0x0000))
	{
		size = dsp56156_op_do_2(cpustate, op, op2, &cycle_count);
	}

	else if (((op & 0xffff) == 0x0002) && ((op2 & 0x0000) == 0x0000))
	{
		size = dsp56156_op_doforever(cpustate, op, op2, &cycle_count);
	}

	else if ((op & 0xffff) == 0x0009)
	{
		size = dsp56156_op_enddo(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff7) == 0x1552)
	{
		size = dsp56156_op_ext(cpustate, op, &cycle_count);
	}

	else if ((op & 0xffff) == 0x000f)
	{
		size = dsp56156_op_illegal(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff0) == 0x15a0)
	{
		size = dsp56156_op_imac(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff0) == 0x1580)
	{
		size = dsp56156_op_impy(cpustate, op, &cycle_count);
	}

	else if (((op & 0xff30) == 0x0630) && ((op2 & 0x0000) == 0x0000))
	{
		size = dsp56156_op_jcc(cpustate, op, op2, &cycle_count);
	}

	else if ((op & 0xff30) == 0x0620 )
	{
		size = dsp56156_op_jcc_1(cpustate, op, &cycle_count);
	}

	else if (((op & 0xfffc) == 0x0134) && ((op2 & 0x0000) == 0x0000))
	{
		size = dsp56156_op_jmp(cpustate, op, op2, &cycle_count);
	}

	else if ((op & 0xfffc) == 0x0124)
	{
		size = dsp56156_op_jmp_1(cpustate, op, &cycle_count);
	}

	else if (((op & 0xff30) == 0x0610) && ((op2 & 0x0000) == 0x0000))
	{
		size = dsp56156_op_jscc(cpustate, op, op2, &cycle_count);
	}

	else if ((op & 0xff30) == 0x0600)
	{
		size = dsp56156_op_jscc_1(cpustate, op, &cycle_count);
	}

	else if (((op & 0xfffc) == 0x0130) && ((op2 & 0x0000) == 0x0000))
	{
		size = dsp56156_op_jsr(cpustate, op, op2, &cycle_count);
	}

	else if ((op & 0xff00) == 0x0a00)
	{
		size = dsp56156_op_jsr_1(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfffc) == 0x0120)
	{
		size = dsp56156_op_jsr_2(cpustate, op, &cycle_count);
	}

	else if ((op & 0xffc0) == 0x01c0)
	{
		size = dsp56156_op_lea(cpustate, op, &cycle_count);
	}

	else if ((op & 0xffc0) == 0x0180)
	{
		size = dsp56156_op_lea_1(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff0) == 0x15e0)
	{
		size = dsp56156_op_macsuuu(cpustate, op, &cycle_count);
	}

	else if (((op & 0xff00) == 0x0500) && ((op2 & 0x00ff) == 0x0011))
	{
		size = dsp56156_op_move_2(cpustate, op, op2, &cycle_count);
	}

	else if ((op & 0xf810) == 0x3800)
	{
		size = dsp56156_op_movec(cpustate, op, &cycle_count);
	}

	else if ((op & 0xf814) == 0x3810)
	{
		size = dsp56156_op_movec_1(cpustate, op, &cycle_count);
	}

	else if ((op & 0xf816) == 0x3816)
	{
		size = dsp56156_op_movec_2(cpustate, op, &cycle_count);
	}

	else if (((op & 0xf816) == 0x3814) && ((op2 & 0x0000) == 0x0000))
	{
		size = dsp56156_op_movec_3(cpustate, op, op2, &cycle_count);
	}

	else if ((op & 0xfc00) == 0x2800)
	{
		size = dsp56156_op_movec_4(cpustate, op, &cycle_count);
	}

	else if (((op & 0xff00) == 0x0500) && ((op2 & 0xf810) == 0x3800))
	{
		size = dsp56156_op_movec_5(cpustate, op, op2, &cycle_count);
	}

	else if ((op & 0xfc00) == 0x2000)
	{
		size = dsp56156_op_movei(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfe20) == 0x0200)
	{
		size = dsp56156_op_movem(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfe30) == 0x0230)
	{
		size = dsp56156_op_movem_1(cpustate, op, &cycle_count);
	}

	else if (((op & 0xff00) == 0x0500) && ((op2 & 0xfe20) == 0x0200))
	{
		size = dsp56156_op_movem_2(cpustate, op, op2, &cycle_count);
	}

	else if ((op & 0xfe20) == 0x1820)
	{
		size = dsp56156_op_movep(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfe00) == 0x0c00)
	{
		size = dsp56156_op_movep_1(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfe20) == 0x1800)
	{
		size = dsp56156_op_moves(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff0) == 0x15c0)
	{
		size = dsp56156_op_mpysuuu(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff7) == 0x1560)
	{
		size = dsp56156_op_negc(cpustate, op, &cycle_count);
	}

	else if ((op & 0xffff) == 0x0000)
	{
		size = dsp56156_op_nop(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff4) == 0x1520)
	{
		size = dsp56156_op_norm(cpustate, op, &cycle_count);
	}

	else if ((op & 0xf900) == 0x1900)
	{
		size = dsp56156_op_ori(cpustate, op, &cycle_count);
	}

	else if ((op & 0xffe0) == 0x00e0)
	{
		size = dsp56156_op_rep(cpustate, op, &cycle_count);
	}

	else if ((op & 0xff00) == 0x0f00)
	{
		size = dsp56156_op_rep_1(cpustate, op, &cycle_count);
	}

	else if ((op & 0xffe0) == 0x0420)
	{
		size = dsp56156_op_rep_2(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff0) == 0x0150)
	{
		size = dsp56156_op_repcc(cpustate, op, &cycle_count);
	}

	else if ((op & 0xffff) == 0x0008)
	{
		size = dsp56156_op_reset(cpustate, op, &cycle_count);
	}

	else if ((op & 0xffff) == 0x0007)
	{
		size = dsp56156_op_rti(cpustate, op, &cycle_count);
	}

	else if ((op & 0xffff) == 0x0006)
	{
		size = dsp56156_op_rts(cpustate, op, &cycle_count);
	}

	else if ((op & 0xffff) == 0x000a)
	{
		size = dsp56156_op_stop(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff7) == 0x1571)
	{
		size = dsp56156_op_swap(cpustate, op, &cycle_count);
	}

	else if ((op & 0xffff) == 0x0005)
	{
		size = dsp56156_op_swi(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfc02) == 0x1000)
	{
		size = dsp56156_op_tcc(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff6) == 0x1500)
	{
		size = dsp56156_op_tfr2(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfc00) == 0x2400)
	{
		size = dsp56156_op_tfr3(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff4) == 0x1514)
	{
		size = dsp56156_op_tst2(cpustate, op, &cycle_count);
	}

	else if ((op & 0xffff) == 0x000b)
	{
		size = dsp56156_op_wait(cpustate, op, &cycle_count);
	}

	else if ((op & 0xfff7) == 0x1550)
	{
		size = dsp56156_op_zero(cpustate, op, &cycle_count);
	}

	if (size == 0x1337)
	{
		LOGMASKED(LOG_UNIMPLEMENTED, "DSP56156: Unimplemented opcode at 0x%04x : %04x\n", PC, op);
		size = 1;
	}

	PC += (UINT16)size;

	dsp56156_process_loop(cpustate);
	dsp56156_process_rep(cpustate, size);

	cpustate->icount -= 4;
}

static size_t dsp56156_op_addsub_2(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT8* cycles)
{
	UINT64 useVal = 0;
	UINT8 op_type = OP_OTHER;
	typed_pointer S = {NULL, DT_BYTE};
	typed_pointer D = {NULL, DT_BYTE};

	decode_uuuuF_table(cpustate, BITS(op_byte,0x0017), BITS(op_byte,0x0008), op_type, &S, &D);

	if (op_type == OP_OTHER)
	{
		d_register->addr = BIT(op_byte, 3) ? &B : &A;
		d_register->data_type = DT_LONG_WORD;
		cycles += 2;
		return 1;
	}

	switch(S.data_type)
	{
		case DT_WORD:        useVal = (UINT64)*((UINT16*)S.addr) << 16; break;
		case DT_DOUBLE_WORD: useVal = (UINT64)*((UINT32*)S.addr);       break;
		case DT_LONG_WORD:   useVal = (UINT64)*((UINT64*)S.addr);       break;
	}

	if ((S.data_type != DT_LONG_WORD) && (useVal & 0x0000000080000000ULL))
		useVal |= 0x000000ff00000000ULL;

	useVal &= 0x000000ffffffffffULL;
	*((UINT64*)D.addr) &= 0x000000ffffffffffULL;

	if (op_type == OP_ADD)
		*((UINT64*)D.addr) += useVal;
	else if (op_type == OP_SUB)
		*((UINT64*)D.addr) -= useVal;

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	if (*((UINT64*)D.addr) & 0x0000008000000000ULL) DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if (*((UINT64*)D.addr) == 0)                     DSP56156_Z_SET(); else DSP56156_Z_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_mac_1(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT8* cycles)
{
	void* D = NULL;
	void* S1 = NULL;
	void* S2 = NULL;

	decode_QQF_table(cpustate, BITS(op_byte,0x0003), BITS(op_byte,0x0008), &S1, &S2, &D);

	const INT32 s1 = *((INT16*)S1);
	const INT32 s2 = *((INT16*)S2);

	INT64 result = (s1 * s2) << 1;

	INT64 opD = dsp_sign_extend(*((UINT64*)D), 40);

	opD += result;

	opD &= 0x000000ffffffffffULL;

	(*((UINT64*)D)) = (UINT64)opD;

	d_register->addr = D;
	d_register->data_type = DT_LONG_WORD;

	if ( *((UINT64*)D) & 0x0000008000000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D) & 0x000000ffffffffffULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_macr_1(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_macr_1\n");
	return 0;
}

static size_t dsp56156_op_move_1(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_move_1\n");
	return 0;
}

static size_t dsp56156_op_mpy_1(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT8* cycles)
{
	void* D = NULL;
	void* S1 = NULL;
	void* S2 = NULL;

	decode_QQF_table(cpustate, BITS(op_byte,0x0003), BITS(op_byte,0x0008), &S1, &S2, &D);

	const INT32 s1 = *((INT16*)S1);
	const INT32 s2 = *((INT16*)S2);

	INT64 result = (s1 * s2) << 1;

	(*((UINT64*)D)) = result & 0x000000ffffffffffULL;

	d_register->addr = D;
	d_register->data_type = DT_LONG_WORD;

	if ( *((UINT64*)D) & 0x0000008000000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D) & 0x000000ffffffffffULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_mpyr_1(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_mpyr_1\n");
	return 0;
}

static size_t dsp56156_op_tfr_2(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_tfr_2\n");
	return 0;
}

static size_t dsp56156_op_mpy_2(dsp56156_core* cpustate, const UINT16 op_byte, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_mpy_2\n");
	return 0;
}

static size_t dsp56156_op_mac_2(dsp56156_core* cpustate, const UINT16 op_byte, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_mac_2\n");
	return 0;
}

static size_t dsp56156_op_clr(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	typed_pointer D = {NULL, DT_LONG_WORD};
	typed_pointer clear = {NULL, DT_LONG_WORD};
	UINT64 clear_val = 0x0000000000000000ULL;

	decode_F_table(cpustate, BITS(op_byte,0x0008), &D);

	*p_accum = *((UINT64*)D.addr);

	clear.addr = &clear_val;
	clear.data_type = DT_LONG_WORD;
	dsp56156_set_destination_value(cpustate, clear, D);

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	DSP56156_E_CLEAR();
	DSP56156_U_SET();
	DSP56156_N_CLEAR();
	DSP56156_Z_SET();
	DSP56156_V_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_add(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	UINT64 addVal = 0;

	typed_pointer S = {NULL, DT_BYTE};
	typed_pointer D = {NULL, DT_BYTE};
	decode_JJJF_table(cpustate, BITS(op_byte,0x0007),BITS(op_byte,0x0008), &S, &D);

	*p_accum = *((UINT64*)D.addr);

	switch(S.data_type)
	{
		case DT_WORD:        addVal = (UINT64)*((UINT16*)S.addr) << 16; break;
		case DT_DOUBLE_WORD: addVal = (UINT64)*((UINT32*)S.addr);       break;
		case DT_LONG_WORD:   addVal = (UINT64)*((UINT64*)S.addr);       break;
	}

	if ((S.data_type != DT_LONG_WORD) && (addVal & 0x0000000080000000ULL))
		addVal |= 0x000000ff00000000ULL;

	addVal &= 0x000000ffffffffffULL;
	*((UINT64*)D.addr) &= 0x000000ffffffffffULL;

	const UINT64 result = *((UINT64*)D.addr) + addVal;
	const UINT8 ovf_index = (BIT(*((UINT64*)D.addr), 39) << 2) | (BIT(addVal, 39) << 1) | BIT(result, 39);
	*((UINT64*)D.addr) = result;

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	if (*((UINT64*)D.addr) & 0x0000008000000000ULL)        DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if (*((UINT64*)D.addr) == 0)                            DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	if (dsp56156_add_ovf_table[ovf_index])                    DSP56156_V_SET(); else DSP56156_V_CLEAR();
	if ((*((UINT64*)D.addr) & 0xffffff0000000000ULL) != 0) DSP56156_C_SET(); else DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_move(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{

	*p_accum = 0;
	d_register->addr = NULL;
	d_register->data_type = DT_BYTE;

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_tfr(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	typed_pointer S = {NULL, DT_BYTE};
	typed_pointer D = {NULL, DT_BYTE};

	decode_JJJF_table(cpustate, BITS(op_byte,0x0007),BITS(op_byte,0x0008), &S, &D);

	*p_accum = *((UINT64*)D.addr);

	dsp56156_set_destination_value(cpustate, S, D);

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_rnd(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	typed_pointer D = {NULL, DT_BYTE};

	decode_F_table(cpustate, BITS(op_byte,0x0008), &D);

	*p_accum = *((UINT64*)D.addr);

	UINT64 *d_ptr = (UINT64*)D.addr;
	if (R_bit(cpustate))
	{
		if ((*d_ptr & 0x000000000000ffffULL) >= 0x8000)
			*d_ptr += 0x0000000000010000ULL;
	}
	else
	{
		UINT64 lower = *d_ptr & 0x000000000000ffffULL;
		if (lower > 0x8000)
			*d_ptr += 0x0000000000010000ULL;
		else if (lower == 0x8000 && BIT(*d_ptr, 16))
			*d_ptr += 0x0000000000010000ULL;
	}

	*((UINT64*)D.addr) = *((UINT64*)D.addr) & 0x000000ffffff0000ULL;

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	if ((*((UINT64*)D.addr)) & 0x0000008000000000ULL) DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D.addr)) == 0)                     DSP56156_Z_SET(); else DSP56156_Z_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_tst(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	typed_pointer D = {NULL, DT_LONG_WORD};

	decode_F_table(cpustate, BITS(op_byte,0x0008), &D);

	*p_accum = *((UINT64*)D.addr);

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	if ((*((UINT64*)D.addr)) & 0x0000008000000000ULL) DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D.addr)) == 0)                     DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	DSP56156_V_CLEAR();
	DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_inc(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	typed_pointer D = {NULL, DT_BYTE};
	decode_F_table(cpustate, BITS(op_byte,0x0008), &D);

	*p_accum = *((UINT64*)D.addr);

	*((UINT64*)D.addr) &= 0x000000ffffffffffULL;

	*((UINT64*)D.addr) = *((UINT64*)D.addr) + 1;

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	if ( *((UINT64*)D.addr) & 0x0000008000000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D.addr) & 0x000000ffffffffffULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	if ((*((UINT64*)D.addr) & 0xffffff0000000000ULL) != 0) DSP56156_V_SET(); else DSP56156_V_CLEAR();
	if ((*((UINT64*)D.addr) & 0xffffff0000000000ULL) != 0) DSP56156_C_SET(); else DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_inc24(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	UINT32 workBits24;

	typed_pointer D = {NULL, DT_BYTE};
	decode_F_table(cpustate, BITS(op_byte,0x0008), &D);

	*p_accum = *((UINT64*)D.addr);

	workBits24 = ((*((UINT64*)D.addr)) & 0x000000ffffff0000ULL) >> 16;
	workBits24++;

	*((UINT64*)D.addr) &= 0x000000000000ffffULL;
	*((UINT64*)D.addr) |= (((UINT64)(workBits24)) << 16);

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	if ( *((UINT64*)D.addr) & 0x0000008000000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D.addr) & 0x000000ffffff0000ULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	if ((workBits24 & 0xff000000) != 0)                       DSP56156_V_SET(); else DSP56156_V_CLEAR();
	if ((workBits24 & 0xff000000) != 0)                       DSP56156_C_SET(); else DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_or(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	typed_pointer S = {NULL, DT_BYTE};
	typed_pointer D = {NULL, DT_BYTE};

	decode_JJF_table(cpustate, BITS(op_byte,0x0003), BITS(op_byte,0x0008), &S, &D);

	*p_accum = *((UINT64*)D.addr);

	((PAIR64*)D.addr)->w.h = *((UINT16*)S.addr) | ((PAIR64*)D.addr)->w.h;

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	if ( *((UINT64*)D.addr) & 0x0000000080000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D.addr) & 0x00000000ffff0000ULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	DSP56156_V_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_asr(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	typed_pointer D = {NULL, DT_BYTE};
	decode_F_table(cpustate, BITS(op_byte,0x0008), &D);

	*p_accum = *((UINT64*)D.addr);

	*((UINT64*)D.addr) = (*((UINT64*)D.addr)) >> 1;

	if (*p_accum & 0x0000008000000000ULL)
		*((UINT64*)D.addr) |= 0x0000008000000000ULL;
	else
		*((UINT64*)D.addr) &= (~(UINT64)(0x0000008000000000ULL));

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	if (*((UINT64*)D.addr) & 0x0000008000000000ULL) DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if (*((UINT64*)D.addr) == 0)                     DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	DSP56156_V_CLEAR();
	if (*p_accum & 0x0000000000000001ULL)             DSP56156_C_SET(); else DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_asl(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	typed_pointer D = {NULL, DT_BYTE};
	decode_F_table(cpustate, BITS(op_byte,0x0008), &D);

	*p_accum = *((UINT64*)D.addr);

	UINT64 dVal = *(UINT64*)D.addr;
	const UINT64 oldBit39 = BIT(dVal, 39);
	dVal = (dVal << 1) & 0x000000ffffffffffULL;
	*((UINT64*)D.addr) = dVal;
	const UINT64 newBit39 = BIT(dVal, 39);

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	if (oldBit39 != newBit39) DSP56156_V_SET(); else DSP56156_V_CLEAR();
	if (newBit39)             DSP56156_C_SET(); else DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_lsr(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	typed_pointer D = {NULL, DT_BYTE};
	decode_F_table(cpustate, BITS(op_byte,0x0008), &D);

	*p_accum = *((UINT64*)D.addr);

	((PAIR64*)D.addr)->w.h = (((PAIR64*)D.addr)->w.h) >> 1;

	((PAIR64*)D.addr)->w.h &= (~0x8000);

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	DSP56156_N_CLEAR();
	if (((PAIR64*)D.addr)->w.h == 0)       DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	DSP56156_V_CLEAR();
	if (*p_accum & 0x0000000000010000ULL) DSP56156_C_SET(); else DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_lsl(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_lsl\n");
	return 0;
}

static size_t dsp56156_op_eor(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	typed_pointer S = {NULL, DT_BYTE};
	typed_pointer D = {NULL, DT_BYTE};

	decode_JJF_table(cpustate, BITS(op_byte,0x0003), BITS(op_byte,0x0008), &S, &D);

	*p_accum = *((UINT64*)D.addr);

	((PAIR64*)D.addr)->w.h = *((UINT16*)S.addr) ^ ((PAIR64*)D.addr)->w.h;

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	if ( *((UINT64*)D.addr) & 0x0000000080000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D.addr) & 0x00000000ffff0000ULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	DSP56156_V_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_subl(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_subl\n");
	return 0;
}

static size_t dsp56156_op_sub(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	UINT64 useVal = 0;
	typed_pointer S = {NULL, DT_BYTE};
	typed_pointer D = {NULL, DT_BYTE};

	decode_JJJF_table(cpustate, BITS(op_byte,0x0007), BITS(op_byte,0x0008), &S, &D);

	*p_accum = *((UINT64*)D.addr);

	switch(S.data_type)
	{
		case DT_WORD:        useVal = (UINT64)*((UINT16*)S.addr) << 16; break;
		case DT_DOUBLE_WORD: useVal = (UINT64)*((UINT32*)S.addr);       break;
		case DT_LONG_WORD:   useVal = (UINT64)*((UINT64*)S.addr);       break;
	}

	if ((S.data_type != DT_LONG_WORD) && (useVal & 0x0000000080000000ULL))
		useVal |= 0x000000ff00000000ULL;

	useVal &= 0x000000ffffffffffULL;
	*((UINT64*)D.addr) &= 0x000000ffffffffffULL;

	UINT64 result = *((UINT64*)D.addr) - useVal;
	const UINT8 ovf_index = (BIT(*((UINT64*)D.addr), 39) << 2) | (BIT(useVal, 39) << 1) | BIT(result, 39);
	*((UINT64*)D.addr) = result;

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	if ( *((UINT64*)D.addr) & 0x0000008000000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ( *((UINT64*)D.addr) == 0)                           DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	if (dsp56156_sub_ovf_table[ovf_index])                    DSP56156_V_SET(); else DSP56156_V_CLEAR();
	if ((*((UINT64*)D.addr) & 0xffffff0000000000ULL) != 0) DSP56156_C_SET(); else DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_clr24(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_clr24\n");
	return 0;
}

static size_t dsp56156_op_sbc(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_sbc\n");
	return 0;
}

static size_t dsp56156_op_cmp(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	UINT64 cmpVal = 0;
	UINT64 dVal = 0;

	typed_pointer S = {NULL, DT_BYTE};
	typed_pointer D = {NULL, DT_BYTE};

	decode_JJJF_table(cpustate, BITS(op_byte,0x0007),BITS(op_byte,0x0008), &S, &D);

	*p_accum = *((UINT64*)D.addr);
	dVal = *((INT64*)D.addr);

	switch(S.data_type)
	{
		case DT_WORD:        cmpVal = (INT64)*((UINT16*)S.addr) << 16; break;
		case DT_DOUBLE_WORD: cmpVal = (INT64)*((UINT32*)S.addr);       break;
		case DT_LONG_WORD:   cmpVal = (INT64)*((UINT64*)S.addr);       break;
	}

	if ((S.data_type != DT_LONG_WORD) && (cmpVal & 0x0000000080000000ULL))
		cmpVal |= 0x000000ff00000000ULL;

	cmpVal &= 0x000000ffffffffffULL;
	dVal &= 0x000000ffffffffffULL;

	UINT64 result = dVal - cmpVal;

	d_register->data_type = DT_LONG_WORD;
	d_register->addr = D.addr;

	UINT8 ovf_index = (BIT(dVal, 39) << 2) | (BIT(cmpVal, 39) << 1) | BIT(result, 39);

	if (result & 0x0000008000000000ULL)        DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if (result == 0)                            DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	if (dsp56156_sub_ovf_table[ovf_index])      DSP56156_V_SET(); else DSP56156_V_CLEAR();
	if ((result & 0xffffff0000000000ULL) != 0) DSP56156_C_SET(); else DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_neg(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	typed_pointer D = {NULL, DT_LONG_WORD};
	typed_pointer neg = {NULL, DT_LONG_WORD};

	decode_F_table(cpustate, BITS(op_byte,0x0008), &D);

	const UINT64 sVal = *p_accum = *((UINT64*)D.addr);
	const UINT64 dVal = 0 - sVal;
	UINT64 dMasked = dVal & 0x000000ffffffffffULL;

	neg.addr = &dMasked;
	neg.data_type = DT_LONG_WORD;
	dsp56156_set_destination_value(cpustate, neg, D);

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	const UINT8 ovf_index = (BIT(sVal, 39) << 1) | BIT(dVal, 39);

	if (dsp56156_value_is_unnormalized(cpustate, dVal)) DSP56156_U_SET(); else DSP56156_U_CLEAR();
	if (dsp56156_value_is_extended(cpustate, dVal))     DSP56156_E_SET(); else DSP56156_E_CLEAR();
	if (dVal & 0x0000008000000000ULL)                  DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if (dVal == 0)                                      DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	if (dsp56156_sub_ovf_table[ovf_index])              DSP56156_V_SET(); else DSP56156_V_CLEAR();
	if ((dVal & 0xffffff0000000000ULL) != 0)           DSP56156_C_SET(); else DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_not(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	typed_pointer D = {NULL, DT_BYTE};
	decode_F_table(cpustate, BITS(op_byte,0x0008), &D);

	*p_accum = *((UINT64*)D.addr);

	((PAIR64*)D.addr)->w.h = ~(((PAIR64*)D.addr)->w.h);

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	if ( *((UINT64*)D.addr) & 0x0000000080000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D.addr) & 0x00000000ffff0000ULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	DSP56156_V_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_dec(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_dec\n");
	return 0;
}

static size_t dsp56156_op_dec24(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	UINT32 workBits24;

	typed_pointer D = {NULL, DT_BYTE};
	decode_F_table(cpustate, BITS(op_byte,0x0008), &D);

	*p_accum = *((UINT64*)D.addr);

	workBits24 = ((*((UINT64*)D.addr)) & 0x000000ffffff0000ULL) >> 16;
	workBits24--;
	const int carry = BIT(workBits24, 24);
	workBits24 &= 0x00ffffff;

	*((UINT64*)D.addr) &= 0x000000000000ffffULL;
	*((UINT64*)D.addr) |= (UINT64)(workBits24) << 16;

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	if ( *((UINT64*)D.addr) & 0x0000008000000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D.addr) & 0x000000ffffff0000ULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	if (carry)                                                DSP56156_C_SET(); else DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_and(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	typed_pointer S = {NULL, DT_BYTE};
	typed_pointer D = {NULL, DT_BYTE};

	decode_JJF_table(cpustate, BITS(op_byte,0x0003), BITS(op_byte,0x0008), &S, &D);

	*p_accum = *((UINT64*)D.addr);

	((PAIR64*)D.addr)->w.h = *((UINT16*)S.addr) & ((PAIR64*)D.addr)->w.h;

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	if ( *((UINT64*)D.addr) & 0x0000000080000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D.addr) & 0x00000000ffff0000ULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	DSP56156_V_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_abs(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	typed_pointer D = {NULL, DT_LONG_WORD};

	decode_F_table(cpustate, BITS(op_byte,0x0008), &D);

	*p_accum = *((UINT64*)D.addr);

	INT64 opD = dsp_sign_extend(*p_accum, 40);

	opD = (opD < 0) ? -opD : opD;
	opD &= 0x000000ffffffffffULL;

	*((UINT64*)D.addr) = opD;

	if ((*p_accum) == 0x0000008000000000ULL)
		*((UINT64*)D.addr) = 0x0000007fffffffffULL;

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	if ( *((UINT64*)D.addr) & 0x0000008000000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D.addr) & 0x000000ffffffffffULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	if ((*p_accum) == 0x0000008000000000ULL)                 DSP56156_V_SET(); else DSP56156_V_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_ror(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_ror\n");
	return 0;
}

static size_t dsp56156_op_rol(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	typed_pointer D = {NULL, DT_BYTE};
	decode_F_table(cpustate, BITS(op_byte,0x0008), &D);

	*p_accum = *((UINT64*)D.addr);

	UINT64 dVal = *(UINT64*)D.addr;
	UINT16 middle = (UINT16)(dVal >> 16);
	int new_carry = BIT(middle, 15);
	middle = (middle << 1) | C();
	dVal &= ~0x00000000ffff0000ULL;
	dVal |= middle << 16;

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	*((UINT64*)D.addr) = dVal;

	if (middle & 0x8000)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if (middle == 0)           DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	DSP56156_V_CLEAR();
	if (new_carry)             DSP56156_C_SET(); else DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_cmpm(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	typed_pointer S = {NULL, DT_BYTE};
	typed_pointer D = {NULL, DT_BYTE};

	decode_JJJF_table(cpustate, BITS(op_byte,0x0007),BITS(op_byte,0x0008), &S, &D);

	*p_accum = *((UINT64*)D.addr);

	UINT64 absS;
	if (S.addr == &A || S.addr == &B)
		absS = llabs(dsp_sign_extend(*((UINT64*)S.addr), 40));
	else
		absS = llabs((INT64)(INT32)((*((UINT16*)S.addr)) << 16));

	UINT64 absD;
	if (D.addr == &A || D.addr == &B)
		absD = llabs(dsp_sign_extend(*((UINT64*)D.addr), 40));
	else
		absD = llabs((INT64)(INT32)(*((UINT16*)D.addr) << 16));

	absS &= 0x000000ffffffffffULL;
	absD &= 0x000000ffffffffffULL;

	const UINT64 absResult = absD - absS;

	d_register->addr = D.addr;
	d_register->data_type = D.data_type;

	UINT8 ovf_index = (BIT(absD, 39) << 2) | (BIT(absS, 39) << 1) | BIT(absResult, 39);

	if (absResult & 0x0000008000000000ULL)        DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if (absResult == 0)                            DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	if (dsp56156_sub_ovf_table[ovf_index])         DSP56156_V_SET(); else DSP56156_V_CLEAR();
	if ((absResult & 0xffffff0000000000ULL) != 0) DSP56156_C_SET(); else DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_mpy(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	void* D = NULL;
	void* S1 = NULL;
	void* S2 = NULL;

	decode_QQQF_table(cpustate, BITS(op_byte,0x0007), BITS(op_byte,0x0008), &S1, &S2, &D);

	*p_accum = *((UINT64*)D);

	const UINT16 k = BITS(op_byte,0x0040);

	const INT32 s1 = *((INT16*)S1);
	const INT32 s2 = *((INT16*)S2);

	INT64 result = (s1 * s2) << 1;

	if (k)
		result *= -1;

	(*((UINT64*)D)) = result & 0x000000ffffffffffULL;

	d_register->addr = D;
	d_register->data_type = DT_LONG_WORD;

	if ( *((UINT64*)D) & 0x0000008000000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D) & 0x000000ffffffffffULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_mpyr(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_mpyr\n");
	return 0;
}

static size_t dsp56156_op_mac(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	void* D = NULL;
	void* S1 = NULL;
	void* S2 = NULL;

	decode_QQQF_table(cpustate, BITS(op_byte,0x0007), BITS(op_byte,0x0008), &S1, &S2, &D);

	*p_accum = *((UINT64*)D);

	const UINT16 k = BITS(op_byte,0x0040);

	const INT32 s1 = *((INT16*)S1);
	const INT32 s2 = *((INT16*)S2);

	INT64 result = (s1 * s2) << 1;

	INT64 opD = dsp_sign_extend(*((UINT64*)D), 40);

	if (k)
		result *= -1;

	opD += result;

	opD &= 0x000000ffffffffffULL;

	(*((UINT64*)D)) = (UINT64)opD;

	d_register->addr = D;
	d_register->data_type = DT_LONG_WORD;

	if ( *((UINT64*)D) & 0x0000008000000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D) & 0x000000ffffffffffULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_macr(dsp56156_core* cpustate, const UINT16 op_byte, typed_pointer* d_register, UINT64* p_accum, UINT8* cycles)
{
	void* D = NULL;
	void* S1 = NULL;
	void* S2 = NULL;

	decode_QQQF_table(cpustate, BITS(op_byte,0x0007), BITS(op_byte,0x0008), &S1, &S2, &D);

	*p_accum = *((UINT64*)D);

	const UINT16 k = BITS(op_byte,0x0040);

	const INT32 s1 = *((INT16*)S1);
	const INT32 s2 = *((INT16*)S2);

	INT64 result = (s1 * s2) << 1;

	INT64 opD = dsp_sign_extend(*((UINT64*)D), 40);

	if (k)
		result *= -1;

	opD += result;

	if (R_bit(cpustate))
	{
		if ((opD & 0x000000000000ffffULL) >= 0x8000)
			opD += 0x0000000000010000ULL;
	}
	else
	{
		const UINT64 lower = opD & 0x000000000000ffffULL;
		if (lower > 0x8000)
			opD += 0x0000000000010000ULL;
		else if (lower == 0x8000 && BIT(opD, 16))
			opD += 0x0000000000010000ULL;
	}
	opD &= 0x000000ffffff0000ULL;

	(*((UINT64*)D)) = (UINT64)opD;

	d_register->addr = D;
	d_register->data_type = DT_LONG_WORD;

	if ( *((UINT64*)D) & 0x0000008000000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D) & 0x000000ffffffffffULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_adc(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_adc\n");
	return 0;
}

static size_t dsp56156_op_andi(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	UINT16 immediate = BITS(op,0x00ff);

	switch(BITS(op,0x0600))
	{
		case 0x01:
			SR &= ((immediate << 8) | 0x00ff);
			break;

		case 0x02:
			OMR &= (UINT8)(immediate);
			break;

		case 0x03:
			SR &= (immediate | 0xff00);
			break;

		default:
			fatalerror("DSP56156 - BAD EE value in andi operation\n");
	}

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_asl4(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	UINT64 p_accum = 0;
	typed_pointer D = {NULL, DT_BYTE};
	decode_F_table(cpustate, BITS(op,0x0008), &D);

	p_accum = *((UINT64*)D.addr);

	*((UINT64*)D.addr) = (*((UINT64*)D.addr)) << 4;
	*((UINT64*)D.addr) = (*((UINT64*)D.addr)) & 0x000000ffffffffffULL;

	if (*((UINT64*)D.addr) & 0x0000008000000000ULL) DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if (*((UINT64*)D.addr) == 0)                     DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	if ((*((UINT64*)D.addr) & 0x000000ff00000000ULL) != (p_accum & 0x000000ff00000000ULL)) DSP56156_V_SET(); else DSP56156_V_CLEAR();
	if (p_accum & 0x0000001000000000ULL)              DSP56156_C_SET(); else DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_asr4(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	UINT64 p_accum = 0;
	typed_pointer D = {NULL, DT_BYTE};
	decode_F_table(cpustate, BITS(op,0x0008), &D);

	p_accum = *((UINT64*)D.addr);

	*((UINT64*)D.addr) = (*((UINT64*)D.addr)) >> 4;
	*((UINT64*)D.addr) = (*((UINT64*)D.addr)) & 0x000000ffffffffffULL;

	if (p_accum & 0x0000008000000000ULL)
		*((UINT64*)D.addr) |= 0x000000f000000000ULL;
	else
		*((UINT64*)D.addr) &= ~0x000000f000000000ULL;

	if (*((UINT64*)D.addr) & 0x0000008000000000ULL) DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if (*((UINT64*)D.addr) == 0)                     DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	DSP56156_V_CLEAR();
	if (p_accum & 0x0000000000000008ULL)              DSP56156_C_SET(); else DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_asr16(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	UINT64 backupVal;
	typed_pointer D = {NULL, DT_BYTE};

	decode_F_table(cpustate, BITS(op,0x0008), &D);

	backupVal = *((UINT64*)D.addr);

	*((UINT64*)D.addr) = *((UINT64*)D.addr) >> 16;

	if (backupVal & 0x0000008000000000ULL)
		*((UINT64*)D.addr) |= 0x000000ffff000000ULL;
	else
		*((UINT64*)D.addr) &= 0x0000000000ffffffULL;

	if (*((UINT64*)D.addr) & 0x0000008000000000ULL) DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if (*((UINT64*)D.addr) == 0)                     DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	DSP56156_V_CLEAR();
	if (backupVal & 0x0000000000008000ULL)            DSP56156_C_SET(); else DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_bfop(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{
	UINT16 workAddr = 0x0000;
	UINT16 workingWord = 0x0000;
	UINT16 previousValue = 0x0000;
	typed_pointer tempTP = { NULL, DT_BYTE };

	UINT16 iVal = op2 & 0x00ff;
	decode_BBB_bitmask(cpustate, BITS(op2,0xe000), &iVal);

	workAddr = assemble_address_from_Pppppp_table(cpustate, BITS(op,0x0020), BITS(op,0x001f));
	previousValue = dsp_data_read(workAddr);
	workingWord = previousValue;

	switch(BITS(op2, 0x1f00))
	{
		case 0x12:
			workingWord ^= iVal;
			break;
		case 0x04:
			workingWord = workingWord & (~iVal);
			break;
		case 0x18:
			workingWord = workingWord | iVal;
			break;
		case 0x10:

			break;
		case 0x00:

			break;
	}

	tempTP.addr = &workingWord;
	tempTP.data_type = DT_WORD;
	dsp56156_set_data_memory_value(cpustate, tempTP, workAddr);

	switch(BITS(op2, 0x1f00))
	{
		case 0x12:
			if ((iVal & previousValue) == iVal) DSP56156_C_SET(); else DSP56156_C_CLEAR(); break;
		case 0x04:
			if ((iVal & previousValue) == 0x0000) DSP56156_C_SET(); else DSP56156_C_CLEAR(); break;
		case 0x18:
			if ((iVal & previousValue) == iVal) DSP56156_C_SET(); else DSP56156_C_CLEAR(); break;
		case 0x10:
			if ((iVal & previousValue) == iVal) DSP56156_C_SET(); else DSP56156_C_CLEAR(); break;
		case 0x00:
			if ((iVal & previousValue) == 0x0000) DSP56156_C_SET(); else DSP56156_C_CLEAR(); break;
	}

	cycles += 4;
	return 2;
}

static size_t dsp56156_op_bfop_1(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{
	UINT16 workAddr = 0x0000;
	UINT16 workingWord = 0x0000;
	UINT16 previousValue = 0x0000;
	typed_pointer R = { NULL, DT_BYTE };
	typed_pointer tempTP = { NULL, DT_BYTE };

	UINT16 iVal = op2 & 0x00ff;
	decode_BBB_bitmask(cpustate, BITS(op2,0xe000), &iVal);

	decode_RR_table(cpustate, BITS(op,0x0003), &R);

	workAddr = *((UINT16*)R.addr);
	previousValue = dsp_data_read(workAddr);
	workingWord = previousValue;

	switch(BITS(op2, 0x1f00))
	{
		case 0x12:
			workingWord ^= iVal;
			break;
		case 0x04:
			workingWord = workingWord & (~iVal);
			break;
		case 0x18:
			workingWord = workingWord | iVal;
			break;
		case 0x10:

			break;
		case 0x00:

			break;
	}

	tempTP.addr = &workingWord;
	tempTP.data_type = DT_WORD;
	dsp56156_set_data_memory_value(cpustate, tempTP, workAddr);

	switch(BITS(op2, 0x1f00))
	{
		case 0x12:
			if ((iVal & previousValue) == iVal) DSP56156_C_SET(); else DSP56156_C_CLEAR(); break;
		case 0x04:
			if ((iVal & previousValue) == 0x0000) DSP56156_C_SET(); else DSP56156_C_CLEAR(); break;
		case 0x18:
			if ((iVal & previousValue) == iVal) DSP56156_C_SET(); else DSP56156_C_CLEAR(); break;
		case 0x10:
			if ((iVal & previousValue) == iVal) DSP56156_C_SET(); else DSP56156_C_CLEAR(); break;
		case 0x00:
			if ((iVal & previousValue) == 0x0000) DSP56156_C_SET(); else DSP56156_C_CLEAR(); break;
	}

	cycles += 4;
	return 2;
}

static size_t dsp56156_op_bfop_2(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{
	UINT16 workingWord = 0;
	UINT16 previousWord = 0;
	UINT64 previousAccum = 0;

	UINT16 iVal = op2 & 0x00ff;
	typed_pointer S = { NULL, DT_BYTE };

	decode_BBB_bitmask(cpustate, BITS(op2,0xe000), &iVal);
	decode_DDDDD_table(cpustate, BITS(op,0x001f), &S);

	if (S.data_type == DT_LONG_WORD)
	{
		previousAccum = dsp56156_limit_long_to_long(cpustate, ((PAIR64*)S.addr)->lw);
		previousWord = (UINT16)(previousAccum >> 16);
	}
	else
	{
		previousWord = *((UINT16*)S.addr);
	}

	workingWord = previousWord;

	switch(BITS(op2, 0x1f00))
	{
		case 0x12:
			workingWord ^= iVal;
			break;
		case 0x04:
			workingWord = workingWord & (~iVal);
			break;
		case 0x18:
			workingWord = workingWord | iVal;
			break;
		case 0x10:

			break;
		case 0x00:

			break;
	}

	if (S.data_type == DT_LONG_WORD)
	{
		((PAIR64*)S.addr)->lw = (previousAccum & 0x000000ff00000000ULL) | ((UINT64)workingWord << 16);
	}
	else
	{
		*(UINT16*)S.addr = workingWord;
	}

	switch(BITS(op2, 0x1f00))
	{
		case 0x12:
			if ((iVal & previousWord) == iVal) DSP56156_C_SET(); else DSP56156_C_CLEAR(); break;
		case 0x04:
			if ((iVal & previousWord) == 0x0000) DSP56156_C_SET(); else DSP56156_C_CLEAR(); break;
		case 0x18:
			if ((iVal & previousWord) == iVal) DSP56156_C_SET(); else DSP56156_C_CLEAR(); break;
		case 0x10:
			if ((iVal & previousWord) == iVal) DSP56156_C_SET(); else DSP56156_C_CLEAR(); break;
		case 0x00:
			if ((iVal & previousWord) == 0x0000) DSP56156_C_SET(); else DSP56156_C_CLEAR(); break;
	}

	cycles += 4;
	return 2;
}

static size_t dsp56156_op_bcc(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{
	int shouldBranch = decode_cccc_table(cpustate, BITS(op,0x000f));

	if (shouldBranch)
	{
		INT16 offset = (INT16)op2;

		PC += offset + 2;

		cycles += 4;
		return 0;
	}
	else
	{
		cycles += 4;
		return 2;
	}

	return 0;
}

static size_t dsp56156_op_bcc_1(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	int shouldBranch = decode_cccc_table(cpustate, BITS(op,0x03c0));

	if (shouldBranch)
	{
		INT16 offset = dsp_sign_extend(op, 6);

		PC += offset + 1;

		cycles += 4;
		return 0;
	}
	else
	{
		cycles += 4;
		return 1;
	}

	return 0;
}

static size_t dsp56156_op_bcc_2(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_bcc_2\n");
	return 0;
}

static size_t dsp56156_op_bra(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_bra\n");
	return 0;
}

static size_t dsp56156_op_bra_1(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	INT8 branchOffset = (INT8)BITS(op,0x00ff);

	PC += 1;

	PC += branchOffset;

	cycles += 4;
	return 0;
}

static size_t dsp56156_op_bra_2(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_bra_2\n");
	return 0;
}

static size_t dsp56156_op_brkcc(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	int shouldBreak = decode_cccc_table(cpustate, BITS(op,0x000f));

	if (shouldBreak)
	{

		PC = LA;

		SR &= 0x3fff;
		SR |= SSL & 0xc000;
		SP--;

		LA = SSH;
		LC = SSL;
		SP--;

		cycles += 2;
		return 0;
	}

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_bscc(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{
	int shouldBranch = decode_cccc_table(cpustate, BITS(op,0x000f));

	if (shouldBranch)
	{

		PC += 2;

		SP++;
		SSH = (PC < 0x80) ? PC : PC;
		SSL = SR;

		PC = PC + (INT16)op2;

		cycles += 4;
		return 0;
	}

	cycles += 4;
	return 2;
}

static size_t dsp56156_op_bscc_1(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_bscc_1\n");
	return 0;
}

static size_t dsp56156_op_bsr(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{

	PC += 2;

	SP++;
	SSH = (PC < 0x80) ? PC : PC;
	SSL = SR;

	PC = PC + (INT16)op2;

	cycles += 4;
	return 0;
}

static size_t dsp56156_op_bsr_1(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_bsr_1\n");
	return 0;
}

static size_t dsp56156_op_chkaau(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_chkaau\n");
	return 0;
}

static size_t dsp56156_op_debug(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_debug\n");
	return 0;
}

static size_t dsp56156_op_debugcc(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_debugcc\n");
	return 0;
}

static size_t dsp56156_op_div(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	typed_pointer S = {NULL, DT_BYTE};
	typed_pointer D = {NULL, DT_BYTE};

	decode_DDF_table(cpustate, BITS(op,0x0003), BITS(op,0x0008), &S, &D);

	UINT64 dVal = *(UINT64*)D.addr;
	UINT64 sVal = *(UINT16*)S.addr;

	const int quotient_negative = BIT(dVal, 39) != BIT(sVal, 15);

	const UINT64 oldBit39 = BIT(dVal, 39);
	dVal <<= 1;
	const UINT64 newBit39 = BIT(dVal, 39);
	dVal |= C();
	dVal &= 0x000000ffffffffffULL;

	if (quotient_negative)
		dVal += ((UINT32)(sVal) << 16) | (BIT(sVal, 15) ? 0xff00000000ULL : 0ULL);
	else
		dVal -= ((UINT32)(sVal) << 16) | (BIT(sVal, 15) ? 0xff00000000ULL : 0ULL);

	*(UINT64*)D.addr = dVal & 0x000000ffffffffffULL;

	if (newBit39 != oldBit39) DSP56156_V_SET(); else DSP56156_V_CLEAR();
	if (!BIT(dVal, 39))       DSP56156_C_SET(); else DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_dmac(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	INT64 result = 0;

	void* D = NULL;
	void* S1 = NULL;
	void* S2 = NULL;

	decode_QQF_special_table(cpustate, BITS(op,0x0003), BITS(op,0x0008), &S1, &S2, &D);

	const UINT8 ss = BITS(op,0x0024);

	if (ss == 0x00 || ss == 0x01)
	{

		const INT64 s1 = (INT64)(*((INT16*)S1));
		const INT32 s2 = (INT32)(*((INT16*)S2));
		result = (s1 * s2) << 1;
	}
	else if (ss == 0x2)
	{

		const INT64 s1 = (INT64)(*((INT16*)S1));
		const INT32 s2 = (UINT32)(*((UINT16*)S2));
		result = (s1 * s2) << 1;
	}
	else if (ss == 0x3)
	{

		const UINT64 s1 = (UINT64)(*((UINT16*)S1));
		const UINT32 s2 = (UINT32)(*((UINT16*)S2));
		result = (s1 * s2) << 1;
	}

	*((UINT64*)D) = (UINT64)((*(INT64*)D << 24) >> 40);
	*((UINT64*)D) += result;

	if ( *((UINT64*)D) & 0x0000008000000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D) & 0x000000ffffffffffULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_do(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_do\n");
	return 0;
}

static size_t dsp56156_op_do_1(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{
	UINT8 retSize = 0;
	const UINT8 iValue = BITS(op,0x00ff);

	if (iValue != 0x00)
	{

		SP++;
		SSH = LA;
		SSL = LC;
		LC = (UINT16)iValue;

		SP++;
		SSH = PC + 2;
		SSL = SR;
		LA = PC + 2 + op2;

		LF_bit_set(cpustate, 1);

		FV_bit_set(cpustate, 0);

		cycles += 6;
		retSize = 2;
	}
	else
	{

		PC = PC + 2 + op2;

		cycles += 10;
		retSize = 0;
	}

	return retSize;
}

static size_t dsp56156_op_do_2(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{
	UINT8 retSize = 0;
	UINT16 lValue = 0x0000;
	typed_pointer S = {NULL, DT_BYTE};
	decode_DDDDD_table(cpustate, BITS(op,0x001f), &S);

	if      (S.addr == &A) lValue = *((UINT16*)(&A1));
	else if (S.addr == &B) lValue = *((UINT16*)(&B1));
	else                   lValue = *((UINT16*)S.addr);

	if (S.addr == &SP)
		LOGMASKED(LOG_UNIMPLEMENTED, "DSP56156: do with SP as the source not properly implemented yet.\n");

	if (S.addr == &SSL)
		LOGMASKED(LOG_UNIMPLEMENTED, "DSP56156: do with SP as the source not properly implemented yet.\n");

	if (lValue != 0x00)
	{

		SP++;
		SSH = LA;
		SSL = LC;
		LC = (UINT16)lValue;

		SP++;
		SSH = PC + 2;
		SSL = SR;
		LA = PC + 2 + op2;

		LF_bit_set(cpustate, 1);

		FV_bit_set(cpustate, 0);

		cycles += 6;
		retSize = 2;
	}
	else
	{

		PC = PC + 2 + op2;

		cycles += 10;
		retSize = 0;
	}

	return retSize;
}

static size_t dsp56156_op_doforever(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{

	SP++;
	SSH = LA;
	SSL = LC;

	SP++;
	SSH = PC + 2;
	SSL = SR;
	LA = PC + 2 + op2;

	LF_bit_set(cpustate, 1);
	FV_bit_set(cpustate, 1);

	cycles += 6;
	return 2;
}

static size_t dsp56156_op_enddo(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	SR &= 0x3fff;
	SR |= SSL & 0xc000;
	SP--;

	LA = SSH;
	LC = SSL;
	SP--;

	cycles += 8;
	return 1;
}

static size_t dsp56156_op_ext(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_ext\n");
	return 0;
}

static size_t dsp56156_op_illegal(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_illegal\n");
	return 0;
}

static size_t dsp56156_op_imac(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	void* D = NULL;
	void* S1 = NULL;
	void* S2 = NULL;

	decode_QQQF_table(cpustate, BITS(op,0x0007), BITS(op,0x0008), &S1, &S2, &D);

	const INT32 s1 = *((INT16*)S1);
	const INT32 s2 = *((INT16*)S2);

	INT64 result = (s1 * s2);

	result = (result << 16) & 0xffff0000;

	INT64 opD = dsp_sign_extend(*((UINT64*)D), 40);

	opD += result;

	opD &= 0x000000ffffffffffULL;

	(*((UINT64*)D)) = (UINT64)opD;

	if ( *((UINT64*)D) & 0x0000008000000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D) & 0x000000ffffff0000ULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();
	DSP56156_V_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_impy(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_impy\n");
	return 0;
}

static size_t dsp56156_op_jcc(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_jcc\n");
	return 0;
}

static size_t dsp56156_op_jcc_1(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_jcc_1\n");
	return 0;
}

static size_t dsp56156_op_jmp(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{
	PC = op2;

	cycles += 4;
	return 0;
}

static size_t dsp56156_op_jmp_1(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	typed_pointer R = { NULL, DT_BYTE };
	decode_RR_table(cpustate, BITS(op,0x0003), &R);

	PC = *((UINT16*)R.addr);

	cycles += 4;
	return 0;
}

static size_t dsp56156_op_jscc(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{
	const int shouldJump = decode_cccc_table(cpustate, BITS(op,0x000f));

	if (shouldJump)
	{

		const UINT16 branchOffset = op2;

		PC += 2;

		SP++;
		SSH = (PC < 0x80) ? PC : PC;
		SSL = SR;

		PC = branchOffset;

		cycles += 4;
		return 0;
	}
	else
	{
		cycles += 4;
		return 2;
	}

	return 0;
}

static size_t dsp56156_op_jscc_1(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_jscc_1\n");
	return 0;
}

static size_t dsp56156_op_jsr(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{

	const UINT16 branchOffset = op2;

	PC += 2;

	SP++;
	SSH = (PC < 0x80) ? IPC : PC;
	SSL = SR;
	PC = branchOffset;

	cycles += 4;
	return 0;
}

static size_t dsp56156_op_jsr_1(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_jsr_1\n");
	return 0;
}

static size_t dsp56156_op_jsr_2(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_jsr_2\n");
	return 0;
}

static size_t dsp56156_op_lea(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	typed_pointer D = {NULL, DT_BYTE};
	decode_TT_table(cpustate, BITS(op,0x0030), &D);

	UINT16 *rX = NULL;
	UINT16 *nX = NULL;
	switch(BITS(op,0x0003))
	{
		case 0x0: rX = &R0;  nX = &N0;  break;
		case 0x1: rX = &R1;  nX = &N1;  break;
		case 0x2: rX = &R2;  nX = &N2;  break;
		case 0x3: rX = &R3;  nX = &N3;  break;
	}

	UINT16 ea = 0;
	switch(BITS(op,0x000c))
	{
		case 0x0: ea = *rX;       break;
		case 0x1: ea = *rX + 1;   break;
		case 0x2: ea = *rX - 1;   break;
		case 0x3: ea = *rX + *nX; break;
	}

	*((UINT16*)D.addr) = ea;

	return 1;
}

static size_t dsp56156_op_lea_1(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_lea_1\n");
	return 0;
}

static size_t dsp56156_op_macsuuu(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	void* D = NULL;
	void* S1 = NULL;
	void* S2 = NULL;

	decode_QQF_special_table(cpustate, BITS(op,0x0003), BITS(op,0x0008), &S1, &S2, &D);

	const UINT8 s = BITS(op,0x0004);

	INT64 result = 0;
	if (s)
	{

		const INT64 s1 = (UINT64)(*((UINT16*)S1));
		const INT32 s2 = (UINT32)(*((UINT16*)S2));
		result = (s1 * s2) << 1;
	}
	else
	{

		const INT64 s1 = (INT64)(*((UINT16*)S1));
		const INT32 s2 = (UINT32)(*((UINT16*)S2));
		result = (s1 * s2) << 1;
	}

	(*((UINT64*)D)) += result;

	(*((UINT64*)D)) &= 0x000000ffffffffffULL;

	if ( *((UINT64*)D) & 0x0000008000000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D) & 0x000000ffffffffffULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_move_2(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_move_2\n");
	return 0;
}

static size_t dsp56156_op_movec(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	UINT8 W;
	typed_pointer R = { NULL, DT_BYTE };
	typed_pointer SD = { NULL, DT_BYTE };

	W = BITS(op,0x0400);
	decode_DDDDD_table(cpustate, BITS(op,0x03e0), &SD);
	decode_RR_table(cpustate, BITS(op,0x0003), &R);

	if (W)
	{

		UINT16 value = dsp_data_read(*((UINT16*)R.addr));
		typed_pointer temp_src = { &value, DT_WORD };
		dsp56156_set_destination_value(cpustate, temp_src, SD);
	}
	else
	{

		UINT16 dataMemOffset = *((UINT16*)R.addr);
		dsp56156_set_data_memory_value(cpustate, SD, dataMemOffset);
	}

	execute_MM_table(cpustate, BITS(op,0x0003), BITS(op,0x000c));

	if (W && (SD.addr != &SR))
	{

	}

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_movec_1(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	typed_pointer SD = {NULL, DT_BYTE};

	const UINT8 W = BITS(op,0x0400);
	decode_DDDDD_table(cpustate, BITS(op,0x03e0), &SD);
	const UINT16 memOffset = execute_q_table(cpustate, BITS(op,0x0003), BITS(op,0x0008));

	if (W)
	{

		UINT16 tempData = dsp_data_read(memOffset);
		typed_pointer temp_src = { (void*)&tempData, DT_WORD };

		if (SD.data_type == DT_LONG_WORD)
		{
			SD.data_type = DT_WORD;
			SD.addr = &((PAIR64*)SD.addr)->w.h;
		}

		dsp56156_set_destination_value(cpustate, temp_src, SD);
	}
	else
	{

		UINT16 tempData = *((UINT16*)SD.addr);

		if (SD.data_type == DT_LONG_WORD)
		{
			tempData = (UINT16)(*((UINT64*)SD.addr) >> 16);
		}

		typed_pointer temp_src = { (void*)&tempData, DT_WORD };
		dsp56156_set_data_memory_value(cpustate, temp_src, memOffset);
	}

	if (W && (SD.addr != &SR))
	{

	}

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_movec_2(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	typed_pointer SD = {NULL, DT_BYTE};
	typed_pointer XMemOffset = {NULL, DT_BYTE};

	const UINT8 W = BITS(op,0x0400);
	decode_Z_table(cpustate, BITS(op,0x0008), &XMemOffset);
	decode_DDDDD_table(cpustate, BITS(op,0x03e0), &SD);

	const UINT16 memOffset = *((UINT16*)XMemOffset.addr);

	if (W)
	{

		UINT16 tempData = dsp_data_read(memOffset);
		typed_pointer temp_src = { (void*)&tempData, DT_WORD };
		dsp56156_set_destination_value(cpustate, temp_src, SD);
	}
	else
	{

		UINT16 tempData = *((UINT16*)SD.addr);
		typed_pointer temp_src = { (void*)&tempData, DT_WORD };
		dsp56156_set_data_memory_value(cpustate, temp_src, memOffset);
	}

	if (W && (SD.addr != &SR))
	{

	}

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_movec_3(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{
	typed_pointer SD = { NULL, DT_BYTE };

	const UINT8 W = BITS(op,0x0400);
	const UINT8 t = BITS(op,0x0008);
	decode_DDDDD_table(cpustate, BITS(op,0x03e0), &SD);

	if (W)
	{

		if (t)
		{

			typed_pointer temp_src = { (void*)&op2, DT_WORD };
			dsp56156_set_destination_value(cpustate, temp_src, SD);
		}
		else
		{

			UINT16 tempD = dsp_data_read(op2);
			typed_pointer tempTP = {&tempD, DT_WORD};
			dsp56156_set_destination_value(cpustate, tempTP, SD);
		}
	}
	else
	{

		if (t)
		{

			LOGMASKED(LOG_UNIMPLEMENTED, "DSP56156: Potentially nonexistent movec_3 at %04x", PC);
		}
		else
		{

			dsp56156_set_data_memory_value(cpustate, SD, op2);
		}
	}

	if (W && (SD.addr != &SR))
	{

	}

	cycles += 2;
	return 2;
}

static size_t dsp56156_op_movec_4(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	typed_pointer S = {NULL, DT_BYTE};
	typed_pointer D = {NULL, DT_BYTE};

	decode_DDDDD_table(cpustate, BITS(op,0x03e0), &S);
	decode_DDDDD_table(cpustate, BITS(op,0x001f), &D);

	const int both_accum = (S.data_type == DT_LONG_WORD && D.data_type == DT_LONG_WORD);

	if (S.data_type == DT_LONG_WORD && D.data_type == DT_WORD)
	{
		S.data_type = DT_WORD;
		if (S.addr == &A)
			S.addr = &A1;
		else if (S.addr == &B)
			S.addr = &B1;
		dsp56156_set_destination_value(cpustate, S, D);
	}
	else if (both_accum)
	{
		typed_pointer tempTP;
		UINT64 temp_val = dsp56156_limit_long_to_long(cpustate, *(UINT64*)S.addr);
		tempTP.addr = &temp_val;
		tempTP.data_type = DT_LONG_WORD;
		dsp56156_set_destination_value(cpustate, tempTP, D);
	}
	else
	{
		dsp56156_set_destination_value(cpustate, S, D);
	}

	if (D.addr != &SR)
	{

	}

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_movec_5(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{
	typed_pointer SD = { NULL, DT_BYTE };

	const INT8 xx = (INT8)(op & 0x00ff);
	const UINT8 W = BITS(op2,0x0400);
	decode_DDDDD_table(cpustate, BITS(op2,0x03e0), &SD);

	const UINT16 memOffset = R2 + (INT16)xx;

	if (W)
	{

		UINT16 tempData = dsp_data_read(memOffset);
		typed_pointer temp_src = { (void*)&tempData, DT_WORD };
		dsp56156_set_destination_value(cpustate, temp_src, SD);
	}
	else
	{

		UINT16 tempData = *((UINT16*)SD.addr);
		typed_pointer temp_src = { (void*)&tempData, DT_WORD };
		dsp56156_set_data_memory_value(cpustate, temp_src, memOffset);
	}

	if (W && (SD.addr != &SR))
	{

	}

	cycles += 2;
	return 2;
}

static size_t dsp56156_op_movei(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	typed_pointer D = {NULL, DT_BYTE};
	typed_pointer immTP = {NULL, DT_BYTE};

	UINT16 immediateSignExtended = (INT16)(op & 0x00ff);
	immTP.addr = &immediateSignExtended;
	immTP.data_type = DT_WORD;

	decode_DD_table(cpustate, BITS(op,0x0300), &D);

	dsp56156_set_destination_value(cpustate, immTP, D);

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_movem(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	typed_pointer R = { NULL, DT_BYTE };
	typed_pointer SD = { NULL, DT_BYTE };

	const UINT8 W = BITS(op,0x0100);
	decode_RR_table(cpustate, BITS(op,0x00c0), &R);
	decode_HHH_table(cpustate, BITS(op,0x0007), &SD);

	if (W)
	{

		typed_pointer data;
		UINT16 ldata = dsp_program_read(*((UINT16*)R.addr));

		data.addr = &ldata;
		data.data_type = DT_WORD;
		dsp56156_set_destination_value(cpustate, data, SD);
	}
	else
	{

		dsp56156_set_program_memory_value(cpustate, SD, *((UINT16*)R.addr));
	}

	execute_MM_table(cpustate, BITS(op,0x00c0), BITS(op,0x0018));

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_movem_1(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_movem_1\n");
	return 0;
}

static size_t dsp56156_op_movem_2(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_movem_2\n");
	return 0;
}

static size_t dsp56156_op_movep(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	typed_pointer SD = {NULL, DT_BYTE};

	decode_HH_table(cpustate, BITS(op,0x00c0), &SD);

	const UINT16 pp = assemble_address_from_IO_short_address(cpustate, op & 0x001f);

	const UINT16 W = BITS(op,0x0100);

	if (W)
	{
		UINT16 data = dsp_data_read(pp);

		typed_pointer tempTP;
		tempTP.addr = &data;
		tempTP.data_type = DT_WORD;

		dsp56156_set_destination_value(cpustate, tempTP, SD);
	}
	else
	{
		dsp56156_set_data_memory_value(cpustate, SD, pp);
	}

	cycles += 4;
	return 1;
}

static size_t dsp56156_op_movep_1(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	typed_pointer SD = {NULL, DT_BYTE};
	decode_RR_table(cpustate, BITS(op,0x00c0), &SD);

	const UINT16 pp = assemble_address_from_IO_short_address(cpustate, op & 0x001f);

	const UINT16 W = BITS(op,0x0100);

	if (W)
	{
		UINT16 data = dsp_data_read(*((UINT16*)SD.addr));

		typed_pointer tempTP;
		tempTP.addr = &data;
		tempTP.data_type = DT_WORD;

		dsp56156_set_data_memory_value(cpustate, tempTP, pp);
	}
	else
	{

		fatalerror("dsp56156 : move(p) NOTHING HERE (yet)\n");
	}

	execute_m_table(cpustate, BITS(op,0x00c0), BITS(op,0x0020));

	cycles += 4;
	return 1;
}

static size_t dsp56156_op_moves(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_moves\n");
	return 0;
}

static size_t dsp56156_op_mpysuuu(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	INT64 result = 0;

	void* D = NULL;
	void* S1 = NULL;
	void* S2 = NULL;

	decode_QQF_special_table(cpustate, BITS(op,0x0003), BITS(op,0x0008), &S1, &S2, &D);

	const UINT8 s = BITS(op,0x0004);

	if (s)
	{

		const INT64 s1 = (UINT64)(*((UINT16*)S1));
		const INT32 s2 = (UINT32)(*((UINT16*)S2));
		result = (s1 * s2) << 1;
	}
	else
	{

		const INT64 s1 = (INT64)(*((INT16*)S1));
		const INT32 s2 = (UINT32)(*((UINT16*)S2));
		result = (s1 * s2) << 1;
	}

	(*((UINT64*)D)) = result;

	(*((UINT64*)D)) &= 0x000000ffffffffffULL;

	if ( *((UINT64*)D) & 0x0000008000000000ULL)       DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT64*)D) & 0x000000ffffffffffULL) == 0) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_negc(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_negc\n");
	return 0;
}

static size_t dsp56156_op_nop(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	return 1;
}

static size_t dsp56156_op_norm(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_norm\n");
	return 0;
}

static size_t dsp56156_op_ori(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_ori\n");
	return 0;
}

static size_t dsp56156_op_rep(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_rep\n");
	return 0;
}

static size_t dsp56156_op_rep_1(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	const UINT16 iVal = op & 0x00ff;

	if (iVal != 0)
	{
		TEMP = LC;
		LC = iVal;

		cpustate->repFlag = 1;
		cpustate->repAddr = PC + 2;

		cycles += 4;
	}
	else
	{
		const UINT16 op1 = ROPCODE(PC + 1);
		const UINT16 op2 = ROPCODE(PC + 2);
		cycles += 6;
		return 1 + dsp56156_get_op_size(op1, op2);
	}

	return 1;
}

static size_t dsp56156_op_rep_2(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	typed_pointer D = {NULL, DT_BYTE};
	decode_DDDDD_table(cpustate, BITS(op,0x001f), &D);

	if (D.addr == &A || D.addr == &B)
		LOGMASKED(LOG_UNIMPLEMENTED, "DSP56156 ERROR: rep_2 with A or B instruction not implemented yet at %04x\n", PC);

	const UINT16 repValue = *((UINT16*)D.addr);

	if (repValue != 0)
	{
		TEMP = LC;
		LC = repValue;

		cpustate->repFlag = 1;
		cpustate->repAddr = PC + 2;

		cycles += 4;
	}
	else
	{
		const UINT16 op1 = ROPCODE(PC + 1);
		const UINT16 op2 = ROPCODE(PC + 2);
		cycles += 6;
		return 1 + dsp56156_get_op_size(op1, op2);
	}

	return 1;
}

static size_t dsp56156_op_repcc(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_repcc\n");
	return 0;
}

static size_t dsp56156_op_reset(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_reset\n");
	return 0;
}

static size_t dsp56156_op_rti(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	if (SP == 0)
	{
		dsp56156_add_pending_interrupt(cpustate, "Stack Error");
		return 0;
	}

	PC = SSH;

	SR = SSL;
	SP = SP - 1;

	cycles += 4;
	return 0;
}

static size_t dsp56156_op_rts(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	PC = SSH;

	SP--;

	cycles += 4;
	return 0;
}

static size_t dsp56156_op_stop(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_stop\n");
	return 0;
}

static size_t dsp56156_op_swap(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_swap\n");
	return 0;
}

static size_t dsp56156_op_swi(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_swi\n");
	return 0;
}

static size_t dsp56156_op_tcc(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	const int shouldTransfer = decode_cccc_table(cpustate, BITS(op,0x03c0));

	if (shouldTransfer)
	{
		typed_pointer S = {NULL, DT_BYTE};
		typed_pointer D = {NULL, DT_BYTE};
		typed_pointer S2 = {&R0, DT_WORD};
		typed_pointer D2 = {NULL, DT_BYTE};

		decode_h0hF_table(cpustate, BITS(op,0x0007),BITS(op,0x0008), &S, &D);
		dsp56156_set_destination_value(cpustate, S, D);

		decode_RR_table(cpustate, BITS(op,0x0030), &D2);
		dsp56156_set_destination_value(cpustate, S2, D2);
	}

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_tfr2(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	typed_pointer S = {NULL, DT_BYTE};
	typed_pointer D = {NULL, DT_BYTE};

	decode_JF_table(cpustate, BITS(op,0x0001), BITS(op,0x0008), &S, &D);

	dsp56156_set_destination_value(cpustate, S, D);

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_tfr3(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_tfr3\n");
	return 0;
}

static size_t dsp56156_op_tst2(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{
	typed_pointer D = {NULL, DT_BYTE};
	decode_DD_table(cpustate, BITS(op,0x0003), &D);

	DSP56156_L_CLEAR();
	DSP56156_E_CLEAR();

	if ((*((UINT16*)D.addr)) &  0x8000) DSP56156_N_SET(); else DSP56156_N_CLEAR();
	if ((*((UINT16*)D.addr)) == 0x0000) DSP56156_Z_SET(); else DSP56156_Z_CLEAR();

	DSP56156_C_CLEAR();

	cycles += 2;
	return 1;
}

static size_t dsp56156_op_wait(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_wait\n");
	return 0;
}

static size_t dsp56156_op_zero(dsp56156_core* cpustate, const UINT16 op, UINT8* cycles)
{

	LOGMASKED(LOG_UNIMPLEMENTED, "dsp56156_op_zero\n");
	return 0;
}

static UINT16 decode_BBB_bitmask(dsp56156_core* cpustate, UINT16 BBB, UINT16 *iVal)
{
	UINT16 retVal = 0x0000;

	switch(BBB)
	{
		case 0x4: retVal = 0xff00;  *iVal <<= 8;  break;
		case 0x2: retVal = 0x0ff0;  *iVal <<= 4;  break;
		case 0x1: retVal = 0x00ff;  *iVal <<= 0;  break;
	}

	return retVal;
}

static int decode_cccc_table(dsp56156_core* cpustate, UINT16 cccc)
{
	int retVal = 0;

	switch (cccc)
	{

		case 0x0: if( C() == 0)                         retVal = 1;  break;
		case 0x8: if( C() == 1)                         retVal = 1;  break;
		case 0x5: if( E() == 0)                         retVal = 1;  break;
		case 0xa: if( Z() == 1)                         retVal = 1;  break;
		case 0xd: if( E() == 1)                         retVal = 1;  break;
		case 0x1: if((N() ^  V()) == 0)                 retVal = 1;  break;
		case 0x7: if((Z() | (N() ^ V())) == 0)          retVal = 1;  break;
		case 0x6: if( L() == 0)                         retVal = 1;  break;
		case 0xf: if((Z() | (N() ^ V())) == 1)          retVal = 1;  break;
		case 0xe: if( L() == 1)                         retVal = 1;  break;
		case 0x9: if((N() ^  V()) == 1)                 retVal = 1;  break;
		case 0xb: if( N() == 1)                         retVal = 1;  break;
		case 0x2: if( Z() == 0)                         retVal = 1;  break;
		case 0xc: if((Z() || ((!U()) && (!E()))))       retVal = 1;  break;
		case 0x3: if( N() == 0)                         retVal = 1;  break;
		case 0x4: if(!(Z() || ((!U()) && (!E()))))      retVal = 1;  break;
	}

	return retVal;
}

static void decode_DDDDD_table(dsp56156_core* cpustate, UINT16 DDDDD, typed_pointer* ret)
{
	switch(DDDDD)
	{
		case 0x00: ret->addr = &X0;  ret->data_type = DT_WORD;       break;
		case 0x01: ret->addr = &Y0;  ret->data_type = DT_WORD;       break;
		case 0x02: ret->addr = &X1;  ret->data_type = DT_WORD;       break;
		case 0x03: ret->addr = &Y1;  ret->data_type = DT_WORD;       break;
		case 0x04: ret->addr = &A;   ret->data_type = DT_LONG_WORD;  break;
		case 0x05: ret->addr = &B;   ret->data_type = DT_LONG_WORD;  break;
		case 0x06: ret->addr = &A0;  ret->data_type = DT_WORD;       break;
		case 0x07: ret->addr = &B0;  ret->data_type = DT_WORD;       break;
		case 0x08: ret->addr = &LC;  ret->data_type = DT_WORD;       break;
		case 0x09: ret->addr = &SR;  ret->data_type = DT_WORD;       break;
		case 0x0a: ret->addr = &OMR; ret->data_type = DT_BYTE;       break;
		case 0x0b: ret->addr = &SP;  ret->data_type = DT_BYTE;       break;
		case 0x0c: ret->addr = &A1;  ret->data_type = DT_WORD;       break;
		case 0x0d: ret->addr = &B1;  ret->data_type = DT_WORD;       break;
		case 0x0e: ret->addr = &A2;  ret->data_type = DT_BYTE;       break;
		case 0x0f: ret->addr = &B2;  ret->data_type = DT_BYTE;       break;

		case 0x10: ret->addr = &R0;  ret->data_type = DT_WORD;       break;
		case 0x11: ret->addr = &R1;  ret->data_type = DT_WORD;       break;
		case 0x12: ret->addr = &R2;  ret->data_type = DT_WORD;       break;
		case 0x13: ret->addr = &R3;  ret->data_type = DT_WORD;       break;
		case 0x14: ret->addr = &M0;  ret->data_type = DT_WORD;       break;
		case 0x15: ret->addr = &M1;  ret->data_type = DT_WORD;       break;
		case 0x16: ret->addr = &M2;  ret->data_type = DT_WORD;       break;
		case 0x17: ret->addr = &M3;  ret->data_type = DT_WORD;       break;
		case 0x18: ret->addr = &SSH; ret->data_type = DT_WORD;       break;
		case 0x19: ret->addr = &SSL; ret->data_type = DT_WORD;       break;
		case 0x1a: ret->addr = &LA;  ret->data_type = DT_WORD;       break;

		case 0x1c: ret->addr = &N0;  ret->data_type = DT_WORD;       break;
		case 0x1d: ret->addr = &N1;  ret->data_type = DT_WORD;       break;
		case 0x1e: ret->addr = &N2;  ret->data_type = DT_WORD;       break;
		case 0x1f: ret->addr = &N3;  ret->data_type = DT_WORD;       break;
	}
}

static void decode_DD_table(dsp56156_core* cpustate, UINT16 DD, typed_pointer* ret)
{
	switch(DD)
	{
		case 0x00: ret->addr = &X0;  ret->data_type = DT_WORD;  break;
		case 0x01: ret->addr = &Y0;  ret->data_type = DT_WORD;  break;
		case 0x02: ret->addr = &X1;  ret->data_type = DT_WORD;  break;
		case 0x03: ret->addr = &Y1;  ret->data_type = DT_WORD;  break;
	}
}

static void decode_DDF_table(dsp56156_core* cpustate, UINT16 DD, UINT16 F, typed_pointer* src_ret, typed_pointer* dst_ret)
{
	UINT16 switchVal = (DD << 1) | F;

	switch (switchVal)
	{
		case 0x0: src_ret->addr = &X0;  src_ret->data_type = DT_WORD; dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x1: src_ret->addr = &X0;  src_ret->data_type = DT_WORD; dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x2: src_ret->addr = &Y0;  src_ret->data_type = DT_WORD; dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x3: src_ret->addr = &Y0;  src_ret->data_type = DT_WORD; dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x4: src_ret->addr = &X1;  src_ret->data_type = DT_WORD; dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x5: src_ret->addr = &X1;  src_ret->data_type = DT_WORD; dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x6: src_ret->addr = &Y1;  src_ret->data_type = DT_WORD; dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x7: src_ret->addr = &Y1;  src_ret->data_type = DT_WORD; dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
	}
}

static void decode_F_table(dsp56156_core* cpustate, UINT16 F, typed_pointer* ret)
{
	switch(F)
	{
		case 0x0: ret->addr = &A;  ret->data_type = DT_LONG_WORD;  break;
		case 0x1: ret->addr = &B;  ret->data_type = DT_LONG_WORD;  break;
	}
}

static void decode_h0hF_table(dsp56156_core* cpustate, UINT16 h0h, UINT16 F, typed_pointer* src_ret, typed_pointer* dst_ret)
{
	UINT16 switchVal = (h0h << 1) | F;

	switch (switchVal)
	{
		case 0x8: src_ret->addr = &X0;  src_ret->data_type = DT_WORD;       dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x9: src_ret->addr = &X0;  src_ret->data_type = DT_WORD;       dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0xa: src_ret->addr = &Y0;  src_ret->data_type = DT_WORD;       dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0xb: src_ret->addr = &Y0;  src_ret->data_type = DT_WORD;       dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x2: src_ret->addr = &A;   src_ret->data_type = DT_LONG_WORD;  dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x1: src_ret->addr = &A;   src_ret->data_type = DT_LONG_WORD;  dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x0: src_ret->addr = &B;   src_ret->data_type = DT_LONG_WORD;  dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x3: src_ret->addr = &B;   src_ret->data_type = DT_LONG_WORD;  dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
	}
}

static void decode_HH_table(dsp56156_core* cpustate, UINT16 HH, typed_pointer* ret)
{
	switch(HH)
	{
		case 0x0: ret->addr = &X0;  ret->data_type = DT_WORD;       break;
		case 0x1: ret->addr = &Y0;  ret->data_type = DT_WORD;       break;
		case 0x2: ret->addr = &A;   ret->data_type = DT_LONG_WORD;  break;
		case 0x3: ret->addr = &B;   ret->data_type = DT_LONG_WORD;  break;
	}
}

static void decode_HHH_table(dsp56156_core* cpustate, UINT16 HHH, typed_pointer* ret)
{
	switch(HHH)
	{
		case 0x0: ret->addr = &X0;  ret->data_type = DT_WORD;       break;
		case 0x1: ret->addr = &Y0;  ret->data_type = DT_WORD;       break;
		case 0x2: ret->addr = &X1;  ret->data_type = DT_WORD;       break;
		case 0x3: ret->addr = &Y1;  ret->data_type = DT_WORD;       break;
		case 0x4: ret->addr = &A;   ret->data_type = DT_LONG_WORD;  break;
		case 0x5: ret->addr = &B;   ret->data_type = DT_LONG_WORD;  break;
		case 0x6: ret->addr = &A0;  ret->data_type = DT_WORD;       break;
		case 0x7: ret->addr = &B0;  ret->data_type = DT_WORD;       break;
	}
}

static void decode_IIII_table(dsp56156_core* cpustate, UINT16 IIII, typed_pointer* src_ret, typed_pointer* dst_ret, void *working)
{
	void *opposite = NULL;

	if (working == &A) opposite = &B;
	else               opposite = &A;

	switch(IIII)
	{
		case 0x0: src_ret->addr = &X0;      src_ret->data_type = DT_WORD;       dst_ret->addr = opposite;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x1: src_ret->addr = &Y0;      src_ret->data_type = DT_WORD;       dst_ret->addr = opposite;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x2: src_ret->addr = &X1;      src_ret->data_type = DT_WORD;       dst_ret->addr = opposite;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x3: src_ret->addr = &Y1;      src_ret->data_type = DT_WORD;       dst_ret->addr = opposite;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x4: src_ret->addr = &A1;      src_ret->data_type = DT_WORD;       dst_ret->addr = &X0;       dst_ret->data_type = DT_WORD;       break;
		case 0x5: src_ret->addr = &B1;      src_ret->data_type = DT_WORD;       dst_ret->addr = &Y0;       dst_ret->data_type = DT_WORD;       break;
		case 0x6: src_ret->addr = &A0;      src_ret->data_type = DT_WORD;       dst_ret->addr = &X0;       dst_ret->data_type = DT_WORD;       break;
		case 0x7: src_ret->addr = &B0;      src_ret->data_type = DT_WORD;       dst_ret->addr = &Y0;       dst_ret->data_type = DT_WORD;       break;
		case 0x8:
		case 0x9: src_ret->addr = working;  src_ret->data_type = DT_LONG_WORD;  dst_ret->addr = opposite;  dst_ret->data_type = DT_LONG_WORD;  break;

		case 0xc: src_ret->addr = &A1;      src_ret->data_type = DT_WORD;       dst_ret->addr = &X1;       dst_ret->data_type = DT_WORD;       break;
		case 0xd: src_ret->addr = &B1;      src_ret->data_type = DT_WORD;       dst_ret->addr = &Y1;       dst_ret->data_type = DT_WORD;       break;
		case 0xe: src_ret->addr = &A0;      src_ret->data_type = DT_WORD;       dst_ret->addr = &X1;       dst_ret->data_type = DT_WORD;       break;
		case 0xf: src_ret->addr = &B0;      src_ret->data_type = DT_WORD;       dst_ret->addr = &Y1;       dst_ret->data_type = DT_WORD;       break;
	}
}

static void decode_JJJF_table(dsp56156_core* cpustate, UINT16 JJJ, UINT16 F, typed_pointer* src_ret, typed_pointer* dst_ret)
{
	UINT16 switchVal = (JJJ << 1) | F;

	switch(switchVal)
	{
		case 0x0: src_ret->addr = &B;   src_ret->data_type = DT_LONG_WORD;    dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x1: src_ret->addr = &A;   src_ret->data_type = DT_LONG_WORD;    dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x4: src_ret->addr = &X;   src_ret->data_type = DT_DOUBLE_WORD;  dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x5: src_ret->addr = &X;   src_ret->data_type = DT_DOUBLE_WORD;  dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x6: src_ret->addr = &Y;   src_ret->data_type = DT_DOUBLE_WORD;  dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x7: src_ret->addr = &Y;   src_ret->data_type = DT_DOUBLE_WORD;  dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x8: src_ret->addr = &X0;  src_ret->data_type = DT_WORD;         dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x9: src_ret->addr = &X0;  src_ret->data_type = DT_WORD;         dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0xa: src_ret->addr = &Y0;  src_ret->data_type = DT_WORD;         dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0xb: src_ret->addr = &Y0;  src_ret->data_type = DT_WORD;         dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0xc: src_ret->addr = &X1;  src_ret->data_type = DT_WORD;         dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0xd: src_ret->addr = &X1;  src_ret->data_type = DT_WORD;         dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0xe: src_ret->addr = &Y1;  src_ret->data_type = DT_WORD;         dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0xf: src_ret->addr = &Y1;  src_ret->data_type = DT_WORD;         dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
	}
}

static void decode_JJF_table(dsp56156_core* cpustate, UINT16 JJ, UINT16 F, typed_pointer* src_ret, typed_pointer* dst_ret)
{
	UINT16 switchVal = (JJ << 1) | F;

	switch (switchVal)
	{
		case 0x0: src_ret->addr = &X0;  src_ret->data_type = DT_WORD;  dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x1: src_ret->addr = &X0;  src_ret->data_type = DT_WORD;  dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x2: src_ret->addr = &Y0;  src_ret->data_type = DT_WORD;  dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x3: src_ret->addr = &Y0;  src_ret->data_type = DT_WORD;  dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x4: src_ret->addr = &X1;  src_ret->data_type = DT_WORD;  dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x5: src_ret->addr = &X1;  src_ret->data_type = DT_WORD;  dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x6: src_ret->addr = &Y1;  src_ret->data_type = DT_WORD;  dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD;  break;
		case 0x7: src_ret->addr = &Y1;  src_ret->data_type = DT_WORD;  dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD;  break;
	}
}

static void decode_JF_table(dsp56156_core* cpustate, UINT16 J, UINT16 F, typed_pointer* src_ret, typed_pointer* dst_ret)
{
	UINT16 switchVal = (J << 1) | F;

	switch (switchVal)
	{
		case 0x0: src_ret->addr = &A;  src_ret->data_type = DT_LONG_WORD;  dst_ret->addr = &X;  dst_ret->data_type = DT_DOUBLE_WORD;  break;
		case 0x1: src_ret->addr = &B;  src_ret->data_type = DT_LONG_WORD;  dst_ret->addr = &X;  dst_ret->data_type = DT_DOUBLE_WORD;  break;
		case 0x2: src_ret->addr = &A;  src_ret->data_type = DT_LONG_WORD;  dst_ret->addr = &Y;  dst_ret->data_type = DT_DOUBLE_WORD;  break;
		case 0x3: src_ret->addr = &B;  src_ret->data_type = DT_LONG_WORD;  dst_ret->addr = &Y;  dst_ret->data_type = DT_DOUBLE_WORD;  break;
	}
}

static void decode_KKK_table(dsp56156_core* cpustate, UINT16 KKK, typed_pointer* dst_ret1, typed_pointer* dst_ret2, void* working)
{
	void *opposite = NULL;

	if (working == &A) opposite = &B;
	else               opposite = &A;

	switch(KKK)
	{
		case 0x0: dst_ret1->addr = opposite;  dst_ret1->data_type = DT_LONG_WORD;  dst_ret2->addr = &X0;  dst_ret2->data_type = DT_WORD;  break;
		case 0x1: dst_ret1->addr = &Y0;       dst_ret1->data_type = DT_WORD;       dst_ret2->addr = &X0;  dst_ret2->data_type = DT_WORD;  break;
		case 0x2: dst_ret1->addr = &X1;       dst_ret1->data_type = DT_WORD;       dst_ret2->addr = &X0;  dst_ret2->data_type = DT_WORD;  break;
		case 0x3: dst_ret1->addr = &Y1;       dst_ret1->data_type = DT_WORD;       dst_ret2->addr = &X0;  dst_ret2->data_type = DT_WORD;  break;
		case 0x4: dst_ret1->addr = &X0;       dst_ret1->data_type = DT_WORD;       dst_ret2->addr = &X1;  dst_ret2->data_type = DT_WORD;  break;
		case 0x5: dst_ret1->addr = &Y0;       dst_ret1->data_type = DT_WORD;       dst_ret2->addr = &X1;  dst_ret2->data_type = DT_WORD;  break;
		case 0x6: dst_ret1->addr = opposite;  dst_ret1->data_type = DT_LONG_WORD;  dst_ret2->addr = &Y0;  dst_ret2->data_type = DT_WORD;  break;
		case 0x7: dst_ret1->addr = &Y1;       dst_ret1->data_type = DT_WORD;       dst_ret2->addr = &X1;  dst_ret2->data_type = DT_WORD;  break;
	}
}

static void decode_QQF_table(dsp56156_core* cpustate, UINT16 QQ, UINT16 F, void **S1, void **S2, void **D)
{
	UINT16 switchVal = (QQ << 1) | F;

	switch(switchVal)
	{
		case 0x0: *S1 = &X0;  *S2 = &Y0;  *D = &A;  break;
		case 0x1: *S1 = &X0;  *S2 = &Y0;  *D = &B;  break;
		case 0x2: *S1 = &X0;  *S2 = &Y1;  *D = &A;  break;
		case 0x3: *S1 = &X0;  *S2 = &Y1;  *D = &B;  break;
		case 0x4: *S1 = &X1;  *S2 = &Y0;  *D = &A;  break;
		case 0x5: *S1 = &X1;  *S2 = &Y0;  *D = &B;  break;
		case 0x6: *S1 = &X1;  *S2 = &Y1;  *D = &A;  break;
		case 0x7: *S1 = &X1;  *S2 = &Y1;  *D = &B;  break;
	}
}

static void decode_QQF_special_table(dsp56156_core* cpustate, UINT16 QQ, UINT16 F, void **S1, void **S2, void **D)
{
	UINT16 switchVal = (QQ << 1) | F;

	switch(switchVal)
	{
		case 0x0: *S1 = &Y0;  *S2 = &X0;  *D = &A;  break;
		case 0x1: *S1 = &Y0;  *S2 = &X0;  *D = &B;  break;
		case 0x2: *S1 = &Y1;  *S2 = &X0;  *D = &A;  break;
		case 0x3: *S1 = &Y1;  *S2 = &X0;  *D = &B;  break;
		case 0x4: *S1 = &X1;  *S2 = &Y0;  *D = &A;  break;
		case 0x5: *S1 = &X1;  *S2 = &Y0;  *D = &B;  break;
		case 0x6: *S1 = &X1;  *S2 = &Y1;  *D = &A;  break;
		case 0x7: *S1 = &X1;  *S2 = &Y1;  *D = &B;  break;
	}
}

static void decode_QQQF_table(dsp56156_core* cpustate, UINT16 QQQ, UINT16 F, void **S1, void **S2, void **D)
{
	UINT16 switchVal = (QQQ << 1) | F;

	switch(switchVal)
	{
		case 0x0: *S1 = &X0;  *S2 = &X0;  *D = &A;  break;
		case 0x1: *S1 = &X0;  *S2 = &X0;  *D = &B;  break;
		case 0x2: *S1 = &X1;  *S2 = &X0;  *D = &A;  break;
		case 0x3: *S1 = &X1;  *S2 = &X0;  *D = &B;  break;
		case 0x4: *S1 = &A1;  *S2 = &Y0;  *D = &A;  break;
		case 0x5: *S1 = &A1;  *S2 = &Y0;  *D = &B;  break;
		case 0x6: *S1 = &B1;  *S2 = &X0;  *D = &A;  break;
		case 0x7: *S1 = &B1;  *S2 = &X0;  *D = &B;  break;
		case 0x8: *S1 = &Y0;  *S2 = &X0;  *D = &A;  break;
		case 0x9: *S1 = &Y0;  *S2 = &X0;  *D = &B;  break;
		case 0xa: *S1 = &Y1;  *S2 = &X0;  *D = &A;  break;
		case 0xb: *S1 = &Y1;  *S2 = &X0;  *D = &B;  break;
		case 0xc: *S1 = &Y0;  *S2 = &X1;  *D = &A;  break;
		case 0xd: *S1 = &Y0;  *S2 = &X1;  *D = &B;  break;
		case 0xe: *S1 = &Y1;  *S2 = &X1;  *D = &A;  break;
		case 0xf: *S1 = &Y1;  *S2 = &X1;  *D = &B;  break;
	}
}

static void decode_RR_table(dsp56156_core* cpustate, UINT16 RR, typed_pointer* ret)
{
	switch(RR)
	{
		case 0x00: ret->addr = &R0;  ret->data_type = DT_WORD;  break;
		case 0x01: ret->addr = &R1;  ret->data_type = DT_WORD;  break;
		case 0x02: ret->addr = &R2;  ret->data_type = DT_WORD;  break;
		case 0x03: ret->addr = &R3;  ret->data_type = DT_WORD;  break;
	}
}

static void decode_TT_table(dsp56156_core* cpustate, UINT16 TT, typed_pointer* ret)
{
	switch(TT)
	{
		case 0x00: ret->addr = &R0;  ret->data_type = DT_WORD;  break;
		case 0x01: ret->addr = &R1;  ret->data_type = DT_WORD;  break;
		case 0x02: ret->addr = &R2;  ret->data_type = DT_WORD;  break;
		case 0x03: ret->addr = &R3;  ret->data_type = DT_WORD;  break;
	}
}

static void decode_uuuuF_table(dsp56156_core* cpustate, UINT16 uuuu, UINT16 F, UINT8 add_sub_other, typed_pointer* src_ret, typed_pointer* dst_ret)
{
	UINT16 switchVal = (uuuu << 1) | F;

	add_sub_other = OP_OTHER;
	src_ret->addr = NULL; src_ret->data_type = DT_BYTE;
	dst_ret->addr = NULL; dst_ret->data_type = DT_BYTE;

	switch(switchVal)
	{
		case 0x00: add_sub_other = OP_ADD;
					src_ret->addr = &X0; src_ret->data_type = DT_WORD;
					dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x08: add_sub_other = OP_SUB;
					src_ret->addr = &X0; src_ret->data_type = DT_WORD;
					dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x01: add_sub_other = OP_ADD;
					src_ret->addr = &X0; src_ret->data_type = DT_WORD;
					dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x09: add_sub_other = OP_SUB;
					src_ret->addr = &X0; src_ret->data_type = DT_WORD;
					dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x02: add_sub_other = OP_ADD;
					src_ret->addr = &Y0; src_ret->data_type = DT_WORD;
					dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x0a: add_sub_other = OP_SUB;
					src_ret->addr = &Y0; src_ret->data_type = DT_WORD;
					dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x03: add_sub_other = OP_ADD;
					src_ret->addr = &Y0; src_ret->data_type = DT_WORD;
					dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x0b: add_sub_other = OP_SUB;
					src_ret->addr = &Y0; src_ret->data_type = DT_WORD;
					dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x04: add_sub_other = OP_ADD;
					src_ret->addr = &X1; src_ret->data_type = DT_WORD;
					dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x0c: add_sub_other = OP_SUB;
					src_ret->addr = &X1; src_ret->data_type = DT_WORD;
					dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x05: add_sub_other = OP_ADD;
					src_ret->addr = &X1; src_ret->data_type = DT_WORD;
					dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x0d: add_sub_other = OP_SUB;
					src_ret->addr = &X1; src_ret->data_type = DT_WORD;
					dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x06: add_sub_other = OP_ADD;
					src_ret->addr = &Y1; src_ret->data_type = DT_WORD;
					dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x0e: add_sub_other = OP_SUB;
					src_ret->addr = &Y1; src_ret->data_type = DT_WORD;
					dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x07: add_sub_other = OP_ADD;
					src_ret->addr = &Y1; src_ret->data_type = DT_WORD;
					dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x0f: add_sub_other = OP_SUB;
					src_ret->addr = &Y1; src_ret->data_type = DT_WORD;
					dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x18: add_sub_other = OP_ADD;
					src_ret->addr = &B;  src_ret->data_type = DT_LONG_WORD;
					dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x1a: add_sub_other = OP_SUB;
					src_ret->addr = &B;  src_ret->data_type = DT_LONG_WORD;
					dst_ret->addr = &A;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x19: add_sub_other = OP_ADD;
					src_ret->addr = &A;  src_ret->data_type = DT_LONG_WORD;
					dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD; break;
		case 0x1b: add_sub_other = OP_SUB;
					src_ret->addr = &A;  src_ret->data_type = DT_LONG_WORD;
					dst_ret->addr = &B;  dst_ret->data_type = DT_LONG_WORD; break;
	}
}

static void decode_Z_table(dsp56156_core* cpustate, UINT16 Z, typed_pointer* ret)
{
	switch(Z)
	{

		case 0x01: ret->addr = &A1;  ret->data_type = DT_WORD;  break;
		case 0x00: ret->addr = &B1;  ret->data_type = DT_WORD;  break;
	}
}

static void execute_m_table(dsp56156_core* cpustate, int x, UINT16 m)
{
	UINT16 *rX = NULL;
	UINT16 *nX = NULL;

	switch(x)
	{
		case 0x0: rX = &R0;  nX = &N0; break;
		case 0x1: rX = &R1;  nX = &N1; break;
		case 0x2: rX = &R2;  nX = &N2; break;
		case 0x3: rX = &R3;  nX = &N3; break;
	}

	switch(m)
	{
		case 0x0: (*rX)++;             break;
		case 0x1: (*rX) = (*rX)+(*nX); break;
	}
}

static void execute_mm_table(dsp56156_core* cpustate, UINT16 rnum, UINT16 mm)
{
	UINT16 *rX = NULL;
	UINT16 *nX = NULL;

	switch(rnum)
	{
		case 0x0: rX = &R0;  nX = &N0;  break;
		case 0x1: rX = &R1;  nX = &N1;  break;
		case 0x2: rX = &R2;  nX = &N2;  break;
		case 0x3: fatalerror("Dsp56156: Error. execute_mm_table specified R3 as its first source!\n");  break;
	}

	switch(mm)
	{
		case 0x0: (*rX)++;                  R3++;           break;
		case 0x1: (*rX)++;                  R3 = R3 + N3;   break;
		case 0x2: (*rX) = (*rX) + (*nX);    R3++;           break;
		case 0x3: (*rX) = (*rX) + (*nX);    R3 = R3 + N3;   break;
	}
}

static void execute_MM_table(dsp56156_core* cpustate, UINT16 rnum, UINT16 MM)
{
	UINT16 *rX = NULL;
	UINT16 *nX = NULL;

	switch(rnum)
	{
		case 0x0: rX = &R0;  nX = &N0;  break;
		case 0x1: rX = &R1;  nX = &N1;  break;
		case 0x2: rX = &R2;  nX = &N2;  break;
		case 0x3: rX = &R3;  nX = &N3;  break;
	}

	switch(MM)
	{
		case 0x0:       break;
		case 0x1: (*rX)++;              break;
		case 0x2: (*rX)--;              break;
		case 0x3: (*rX) = (*rX)+(*nX);  break;
	}
}

static UINT16 execute_q_table(dsp56156_core* cpustate, int RR, UINT16 q)
{
	UINT16 *rX = NULL;
	UINT16 *nX = NULL;

	switch(RR)
	{
		case 0x0: rX = &R0;  nX = &N0;  break;
		case 0x1: rX = &R1;  nX = &N1;  break;
		case 0x2: rX = &R2;  nX = &N2;  break;
		case 0x3: rX = &R3;  nX = &N3;  break;
	}

	switch(q)
	{
		case 0x0: ; return (*rX)+(*nX);
		case 0x1: (*rX)--;                    return (*rX);
	}

	fatalerror("dsp56156: execute_q_table did something impossible!\n");
	return 0;
}

static void execute_z_table(dsp56156_core* cpustate, int RR, UINT16 z)
{
	UINT16 *rX = NULL;
	UINT16 *nX = NULL;

	switch(RR)
	{
		case 0x0: rX = &R0;  nX = &N0;  break;
		case 0x1: rX = &R1;  nX = &N1;  break;
		case 0x2: rX = &R2;  nX = &N2;  break;
		case 0x3: rX = &R3;  nX = &N3;  break;
	}

	switch(z)
	{
		case 0x0: (*rX)--;               break;
		case 0x1: (*rX) = (*rX) + (*nX); break;
	}
}

static UINT16 assemble_address_from_Pppppp_table(dsp56156_core* cpustate, UINT16 P, UINT16 ppppp)
{
	UINT16 destAddr = 0x00;

	switch (P)
	{
		case 0x0: destAddr = ppppp;  break;
		case 0x1: destAddr = assemble_address_from_IO_short_address(cpustate, ppppp);  break;
	}

	return destAddr;
}

static UINT16 assemble_address_from_IO_short_address(dsp56156_core* cpustate, UINT16 pp)
{
	UINT16 fullAddy = 0xffe0;
	fullAddy |= pp;
	return fullAddy;
}

static void dsp56156_process_loop(dsp56156_core* cpustate)
{

	if (LF_bit(cpustate) && FV_bit(cpustate))
	{

		if (PC == LA)
		{
			LC--;

			PC = SSH;
		}
	}
	else if (LF_bit(cpustate))
	{

		if (PC == LA)
		{
			if (LC == 1)
			{

				SR = SSL;
				SP--;

				LA = SSH;
				LC = SSL;
				SP--;
			}
			else
			{
				LC--;
				PC = SSH;
			}
		}
	}
}

static void dsp56156_process_rep(dsp56156_core* cpustate, size_t repSize)
{
	if (cpustate->repFlag)
	{
		if (PC == cpustate->repAddr)
		{
			if (LC == 1)
			{

				LC = TEMP;
				cpustate->repFlag = 0;
				cpustate->repAddr = 0x0000;
			}
			else
			{
				LC--;
				PC -= repSize;
			}
		}
	}
}

static int dsp56156_value_is_unnormalized(dsp56156_core* cpustate, const UINT64 value)
{
	UINT16 S = (cpustate->PCU.sr >> 10) & 3;
	if (S == 0 || S == 3)
		return BIT(value, 31) == BIT(value, 30);
	else if (S == 1)
		return BIT(value, 32) == BIT(value, 31);
	else
		return BIT(value, 30) == BIT(value, 29);
}

static int dsp56156_value_is_extended(dsp56156_core* cpustate, const UINT64 value)
{
	UINT16 S = (cpustate->PCU.sr >> 10) & 3;
	UINT64 extMask = 0x000000ffc0000000ULL;
	if (S == 0 || S == 3)
		extMask = 0x000000ff80000000ULL;
	else if (S == 1)
		extMask = 0x000000ff00000000ULL;
	UINT64 extVal = value & extMask;
	return extVal == extMask || extVal == 0;
}

static void execute_register_to_register_data_move(dsp56156_core* cpustate, const UINT16 op, typed_pointer* d_register, UINT64* prev_accum_value)
{
	typed_pointer S = {NULL, DT_BYTE};
	typed_pointer D = {NULL, DT_BYTE};

	decode_IIII_table(cpustate, BITS(op,0x0f00), &S, &D, d_register->addr);

	const int opdest_is_a = d_register->addr == &A0 || d_register->addr == &A1;
	const int opdest_is_b = d_register->addr == &B0 || d_register->addr == &B1;
	const int s_is_a = S.addr == &A0 || S.addr == &A1;
	const int s_is_b = S.addr == &B0 || S.addr == &B1;

	if ((opdest_is_a && s_is_a) || (opdest_is_b && s_is_b))
	{
		typed_pointer tempTP;
		UINT64 temp_long = 0ULL;
		UINT16 temp_word = 0U;
		if (S.addr == &A1 || S.addr == &B1)
		{
			temp_word = (UINT16)(*prev_accum_value >> 16);
			tempTP.addr = &temp_word;
			tempTP.data_type = DT_WORD;
		}
		else if (S.data_type == DT_LONG_WORD)
		{
			temp_long = dsp56156_limit_long_to_long(cpustate, *prev_accum_value);
			tempTP.addr = &temp_long;
			tempTP.data_type = DT_LONG_WORD;
		}
		else
		{
			temp_word = (UINT16)*prev_accum_value;
			tempTP.addr = &temp_word;
			tempTP.data_type = DT_WORD;
		}
		dsp56156_set_destination_value(cpustate, tempTP, D);
	}
	else
	{
		dsp56156_set_destination_value(cpustate, S, D);
	}
}

static void execute_address_register_update(dsp56156_core* cpustate, const UINT16 op, typed_pointer* d_register, UINT64* prev_accum_value)
{
	execute_z_table(cpustate, BITS(op,0x0300), BITS(op,0x0400));
}

static void execute_x_memory_data_move(dsp56156_core* cpustate, const UINT16 op, typed_pointer* d_register, UINT64* prev_accum_value)
{
	typed_pointer R = {NULL, DT_BYTE};
	typed_pointer SD = {NULL, DT_BYTE};

	const UINT16 W = BITS(op,0x0100);
	decode_HHH_table(cpustate, BITS(op,0x0e00), &SD);
	decode_RR_table(cpustate, BITS(op,0x3000),&R);

	if (W)
	{

		UINT16 data = dsp_data_read(*((UINT16*)R.addr));

		typed_pointer tempTP;
		tempTP.addr = &data;
		tempTP.data_type = DT_WORD;

		dsp56156_set_destination_value(cpustate, tempTP, SD);
	}
	else
	{

		if (d_register->addr == SD.addr)
		{
			typed_pointer tempTP;
			tempTP.addr = prev_accum_value;
			tempTP.data_type = DT_LONG_WORD;

			dsp56156_set_data_memory_value(cpustate, tempTP, *((UINT16*)R.addr));
		}
		else
		{
			dsp56156_set_data_memory_value(cpustate, SD, *((UINT16*)R.addr));
		}
	}

	execute_m_table(cpustate, BITS(op,0x3000), BITS(op,0x4000));
}

static void execute_x_memory_data_move2(dsp56156_core* cpustate, const UINT16 op, typed_pointer* d_register)
{
	typed_pointer SD = {NULL, DT_BYTE};

	const UINT16 W = BITS(op,0x0100);
	decode_HHH_table(cpustate, BITS(op,0x0e000), &SD);

	UINT16 *const mem_offset = (d_register->addr == &A) ? &B1 : &A1;

	if (W)
	{

		UINT16 value = dsp_data_read(*mem_offset);
		typed_pointer tempV = {&value, DT_WORD};
		dsp56156_set_destination_value(cpustate, tempV, SD);
	}
	else
	{

		dsp56156_set_data_memory_value(cpustate, SD, *mem_offset);
	}
}

static void execute_x_memory_data_move_with_short_displacement(dsp56156_core* cpustate, const UINT16 op, const UINT16 op2)
{
	typed_pointer SD = { NULL, DT_BYTE };

	const INT8 xx = (INT8)(op & 0x00ff);
	const UINT8 W = BITS(op2,0x0100);
	decode_HHH_table(cpustate, BITS(op2,0x0e00), &SD);

	const UINT16 memOffset = R2 + (INT16)xx;

	if (W)
	{

		UINT16 tempData = dsp_data_read(memOffset);
		typed_pointer temp_src = { (void*)&tempData, DT_WORD };
		dsp56156_set_destination_value(cpustate, temp_src, SD);
	}
	else
	{

		UINT16 tempData = *((UINT16*)SD.addr);
		typed_pointer temp_src = { (void*)&tempData, DT_WORD };
		dsp56156_set_data_memory_value(cpustate, temp_src, memOffset);
	}
}

static void execute_dual_x_memory_data_read(dsp56156_core* cpustate, const UINT16 op, typed_pointer* d_register)
{
	typed_pointer tempV;
	typed_pointer R = {NULL, DT_BYTE};
	typed_pointer D1 = {NULL, DT_BYTE};
	typed_pointer D2 = {NULL, DT_BYTE};

	decode_RR_table(cpustate, BITS(op,0x0060), &R);
	decode_KKK_table(cpustate, BITS(op,0x0700), &D1, &D2, d_register->addr);

	if (R.addr == &R3)
		fatalerror("Dsp56156: Error. Dual x memory data read specified R3 as its first source!\n");

	if (*((UINT16*)R.addr) >= 0xffc0 || R3 >= 0xffc0)
	{
		fatalerror("Unsupported DSP56156 operation at %04x", PC);
		LOGMASKED(LOG_UNIMPLEMENTED, "Dsp56156: Unimplemented access to external X Data Memory >= 0xffc0 in Dual X Memory Data Read at %04x.\n", cpustate->PCU.pc);
	}

	UINT16 srcVal1 = dsp_data_read(*((UINT16*)R.addr));
	tempV.addr = &srcVal1;
	tempV.data_type = DT_WORD;
	dsp56156_set_destination_value(cpustate, tempV, D1);

	UINT16 srcVal2 = dsp_data_read(R3);
	tempV.addr = &srcVal2;
	tempV.data_type = DT_WORD;
	dsp56156_set_destination_value(cpustate, tempV, D2);

	execute_mm_table(cpustate, BITS(op,0x0060), BITS(op,0x1800));
}

static UINT16 dsp56156_op_mask(UINT16 cur, UINT16 mask)
{
	UINT16 retVal = (cur & mask);
	UINT16 temp = 0x0000;
	int offsetCount = 0;

	int i;
	for (i = 0; i < 16; i++)
	{
		if (BIT(mask, i))
		{
			temp |= (BIT(retVal, i) << offsetCount);
			offsetCount++;
		}
	}

	return temp;
}

static UINT64 dsp56156_limit_long_to_long(dsp56156_core* cpustate, UINT64 val)
{
	INT32 signed_upper = (INT32)(val >> 8) >> 8;
	if (signed_upper < -32768)
		return 0x000000ff80000000ULL;
	else if (signed_upper > 32767)
		return 0x000000007fff0000ULL;
	return val & 0x000000ffffff0000ULL;
}

static void dsp56156_set_destination_value(dsp56156_core* cpustate, typed_pointer source, typed_pointer dest)
{
	UINT64 destinationValue = 0;

	switch(dest.data_type)
	{

		case DT_BYTE:
			switch(source.data_type)
			{

				case DT_BYTE:        *((UINT8*)dest.addr) = (*((UINT8*) source.addr)) & 0xff; break;
				case DT_WORD:        *((UINT8*)dest.addr) = (*((UINT16*)source.addr)) & 0x00ff; break;
				case DT_DOUBLE_WORD: *((UINT8*)dest.addr) = (*((UINT32*)source.addr)) & 0x000000ff; break;
				case DT_LONG_WORD:   *((UINT8*)dest.addr) = (*((UINT64*)source.addr)) & 0x00000000000000ffULL; break;
			}
			break;

		case DT_WORD:
			switch(source.data_type)
			{
				case DT_BYTE:        *((UINT16*)dest.addr) = (*((UINT8*) source.addr)) & 0xff; break;
				case DT_WORD:        *((UINT16*)dest.addr) = (*((UINT16*)source.addr)) & 0xffff; break;
				case DT_DOUBLE_WORD: *((UINT16*)dest.addr) = (*((UINT32*)source.addr)) & 0x0000ffff; break;
				case DT_LONG_WORD:   *((UINT16*)dest.addr) = (*((UINT64*)source.addr)) & 0x000000000000ffffULL;

					if (dest.addr == &A || dest.addr == &B)
					{
						LOGMASKED(LOG_CHECKS, "%04x: Double-check dsp56156_set_destination_value, writing A/B to a word.\n", cpustate->PCU.pc);
					}
					break;
			}
			break;

		case DT_DOUBLE_WORD:
			switch(source.data_type)
			{
				case DT_BYTE:        *((UINT32*)dest.addr) = (*((UINT8*) source.addr)) & 0xff; break;
				case DT_WORD:        *((UINT32*)dest.addr) = (*((UINT16*)source.addr)) & 0xffff; break;
				case DT_DOUBLE_WORD: *((UINT32*)dest.addr) = (*((UINT32*)source.addr)) & 0xffffffff; break;
				case DT_LONG_WORD:   *((UINT32*)dest.addr) = (*((UINT64*)source.addr)) & 0x00000000ffffffffULL; break;
			}
			break;

		case DT_LONG_WORD:
			switch(source.data_type)
			{
				case DT_BYTE:        *((UINT64*)dest.addr) = (*((UINT8*)source.addr)) & 0xff; break;

				case DT_WORD:        destinationValue = (*((UINT16*)source.addr)) << 16;
										if (destinationValue & 0x0000000080000000ULL)
											destinationValue |= 0x000000ff00000000ULL;
										*((UINT64*)dest.addr) = (UINT64)destinationValue; break;

				case DT_DOUBLE_WORD: *((UINT64*)dest.addr) = (*((UINT32*)source.addr)) & 0xffffffff; break;
				case DT_LONG_WORD:   *((UINT64*)dest.addr) = (*((UINT64*)source.addr)) & 0x000000ffffffffffULL; break;
			}
			break;
	}
}

static void dsp56156_set_data_memory_value(dsp56156_core* cpustate, typed_pointer source, UINT32 destinationAddr)
{
	switch(source.data_type)
	{
		case DT_BYTE:        dsp_data_write(destinationAddr, (UINT16)( (*((UINT8*) source.addr) & 0xff)));       break;
		case DT_WORD:        dsp_data_write(destinationAddr, (UINT16)( (*((UINT16*)source.addr) & 0xffff)));     break;
		case DT_DOUBLE_WORD: dsp_data_write(destinationAddr, (UINT16)( (*((UINT32*)source.addr) & 0x0000ffff))); break;

		case DT_LONG_WORD:   dsp_data_write(destinationAddr, (UINT16)(((*((UINT64*)source.addr)) & 0x00000000ffff0000ULL) >> 16)); break;
	}
}

static void dsp56156_set_program_memory_value(dsp56156_core* cpustate, typed_pointer source, UINT32 destinationAddr)
{
	switch(source.data_type)
	{
		case DT_BYTE:        dsp_program_write(destinationAddr, (UINT16)( (*((UINT8*) source.addr) & 0xff)));        break;
		case DT_WORD:        dsp_program_write(destinationAddr, (UINT16)( (*((UINT16*)source.addr) & 0xffff)));      break;
		case DT_DOUBLE_WORD: dsp_program_write(destinationAddr, (UINT16)( (*((UINT32*)source.addr) & 0x0000ffff)));  break;

		case DT_LONG_WORD:   dsp_program_write(destinationAddr, (UINT16)(((*((UINT64*)source.addr)) & 0x00000000ffff0000ULL) >> 16)); break;
	}
}

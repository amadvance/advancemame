// license:BSD-3-Clause
// copyright-holders:Ville Linde, Barry Rodewald
static UINT16 I386OP(shift_rotate16)(UINT8 modrm, UINT32 value, UINT8 shift)
{
	UINT16 src = value;
	UINT16 dst = value;

	if( shift == 0 ) {
		CYCLES_RM(modrm, 3, 7);
	} else if( shift == 1 ) {

		switch( (modrm >> 3) & 0x7 )
		{
			case 0:
				I.CF = (src & 0x8000) ? 1 : 0;
				dst = (src << 1) + I.CF;
				I.OF = ((src ^ dst) & 0x8000) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 1:
				I.CF = (src & 0x1) ? 1 : 0;
				dst = (I.CF << 15) | (src >> 1);
				I.OF = ((src ^ dst) & 0x8000) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 2:
				dst = (src << 1) + I.CF;
				I.CF = (src & 0x8000) ? 1 : 0;
				I.OF = ((src ^ dst) & 0x8000) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 3:
				dst = (I.CF << 15) | (src >> 1);
				I.CF = src & 0x1;
				I.OF = ((src ^ dst) & 0x8000) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 4:
			case 6:
				dst = src << 1;
				I.CF = (src & 0x8000) ? 1 : 0;
				I.OF = (((I.CF << 15) ^ dst) & 0x8000) ? 1 : 0;
				SetSZPF16(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 5:
				dst = src >> 1;
				I.CF = src & 0x1;
				I.OF = (dst & 0x8000) ? 1 : 0;
				SetSZPF16(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 7:
				dst = (INT16)(src) >> 1;
				I.CF = src & 0x1;
				I.OF = 0;
				SetSZPF16(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
		}
	} else {

		switch( (modrm >> 3) & 0x7 )
		{
			case 0:
				dst = ((src & ((UINT16)0xffff >> shift)) << shift) |
					  ((src & ((UINT16)0xffff << (16-shift))) >> (16-shift));
				I.CF = (src >> (16-shift)) & 0x1;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 1:
				dst = ((src & ((UINT16)0xffff << shift)) >> shift) |
					  ((src & ((UINT16)0xffff >> (16-shift))) << (16-shift));
				I.CF = (src >> (shift-1)) & 0x1;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 2:
				dst = ((src & ((UINT16)0xffff >> shift)) << shift) |
					  ((src & ((UINT16)0xffff << (17-shift))) >> (17-shift)) |
					  (I.CF << (shift-1));
				I.CF = (src >> (16-shift)) & 0x1;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 3:
				dst = ((src & ((UINT16)0xffff << shift)) >> shift) |
					  ((src & ((UINT16)0xffff >> (16-shift))) << (17-shift)) |
					  (I.CF << (16-shift));
				I.CF = (src >> (shift-1)) & 0x1;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 4:
			case 6:
				dst = src << shift;
				I.CF = (src & (1 << (16-shift))) ? 1 : 0;
				SetSZPF16(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 5:
				dst = src >> shift;
				I.CF = (src & (1 << (shift-1))) ? 1 : 0;
				SetSZPF16(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 7:
				dst = (INT16)src >> shift;
				I.CF = (src & (1 << (shift-1))) ? 1 : 0;
				SetSZPF16(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
		}

	}
	return dst;
}


static void I386OP(adc_rm16_r16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		dst = ADC16(dst, src, I.CF);
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = READ16(ea);
		dst = ADC16(dst, src, I.CF);
		WRITE16(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(adc_r16_rm16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM16(modrm);
		dst = LOAD_REG16(modrm);
		dst = ADC16(dst, src, I.CF);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ16(ea);
		dst = LOAD_REG16(modrm);
		dst = ADC16(dst, src, I.CF);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(adc_ax_i16)(void)
{
	UINT16 src, dst;
	src = FETCH16();
	dst = REG16(AX);
	dst = ADC16(dst, src, I.CF);
	REG16(AX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(add_rm16_r16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		dst = ADD16(dst, src);
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = READ16(ea);
		dst = ADD16(dst, src);
		WRITE16(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(add_r16_rm16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM16(modrm);
		dst = LOAD_REG16(modrm);
		dst = ADD16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ16(ea);
		dst = LOAD_REG16(modrm);
		dst = ADD16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(add_ax_i16)(void)
{
	UINT16 src, dst;
	src = FETCH16();
	dst = REG16(AX);
	dst = ADD16(dst, src);
	REG16(AX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(and_rm16_r16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		dst = AND16(dst, src);
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = READ16(ea);
		dst = AND16(dst, src);
		WRITE16(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(and_r16_rm16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM16(modrm);
		dst = LOAD_REG16(modrm);
		dst = AND16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ16(ea);
		dst = LOAD_REG16(modrm);
		dst = AND16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(and_ax_i16)(void)
{
	UINT16 src, dst;
	src = FETCH16();
	dst = REG16(AX);
	dst = AND16(dst, src);
	REG16(AX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(bsf_r16_rm16)(void)
{
	UINT16 src, dst, temp;
	UINT8 modrm = FETCH();

	if( modrm >= 0xc0 ) {
		src = LOAD_RM16(modrm);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ16(ea);
	}

	dst = 0;

	if( src == 0 ) {
		I.ZF = 1;
	} else {
		I.ZF = 0;
		temp = 0;
		while( (src & (1 << temp)) == 0 ) {
			temp++;
			dst = temp;
			CYCLES(CYCLES_BSF);
		}
	}
	CYCLES(CYCLES_BSF_BASE);
	STORE_REG16(modrm, dst);
}

static void I386OP(bsr_r16_rm16)(void)
{
	UINT16 src, dst, temp;
	UINT8 modrm = FETCH();

	if( modrm >= 0xc0 ) {
		src = LOAD_RM16(modrm);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ16(ea);
	}

	dst = 0;

	if( src == 0 ) {
		I.ZF = 1;
	} else {
		I.ZF = 0;
		dst = temp = 15;
		while( (src & (1 << temp)) == 0 ) {
			temp--;
			dst = temp;
			CYCLES(CYCLES_BSR);
		}
	}
	CYCLES(CYCLES_BSR_BASE);
	STORE_REG16(modrm, dst);
}

static void I386OP(bt_rm16_r16)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT16 dst = LOAD_RM16(modrm);
		UINT16 bit = LOAD_REG16(modrm);

		if( dst & (1 << bit) )
			I.CF = 1;
		else
			I.CF = 0;

		CYCLES(CYCLES_BT_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT16 dst = READ16(ea);
		UINT16 bit = LOAD_REG16(modrm);

		if( dst & (1 << bit) )
			I.CF = 1;
		else
			I.CF = 0;

		CYCLES(CYCLES_BT_REG_MEM);
	}
}

static void I386OP(btc_rm16_r16)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT16 dst = LOAD_RM16(modrm);
		UINT16 bit = LOAD_REG16(modrm);

		if( dst & (1 << bit) )
			I.CF = 1;
		else
			I.CF = 0;
		dst ^= (1 << bit);

		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_BTC_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT16 dst = READ16(ea);
		UINT16 bit = LOAD_REG16(modrm);

		if( dst & (1 << bit) )
			I.CF = 1;
		else
			I.CF = 0;
		dst ^= (1 << bit);

		WRITE16(ea, dst);
		CYCLES(CYCLES_BTC_REG_MEM);
	}
}

static void I386OP(btr_rm16_r16)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT16 dst = LOAD_RM16(modrm);
		UINT16 bit = LOAD_REG16(modrm);

		if( dst & (1 << bit) )
			I.CF = 1;
		else
			I.CF = 0;
		dst &= ~(1 << bit);

		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_BTR_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT16 dst = READ16(ea);
		UINT16 bit = LOAD_REG16(modrm);

		if( dst & (1 << bit) )
			I.CF = 1;
		else
			I.CF = 0;
		dst &= ~(1 << bit);

		WRITE16(ea, dst);
		CYCLES(CYCLES_BTR_REG_MEM);
	}
}

static void I386OP(bts_rm16_r16)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT16 dst = LOAD_RM16(modrm);
		UINT16 bit = LOAD_REG16(modrm);

		if( dst & (1 << bit) )
			I.CF = 1;
		else
			I.CF = 0;
		dst |= (1 << bit);

		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_BTS_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT16 dst = READ16(ea);
		UINT16 bit = LOAD_REG16(modrm);

		if( dst & (1 << bit) )
			I.CF = 1;
		else
			I.CF = 0;
		dst |= (1 << bit);

		WRITE16(ea, dst);
		CYCLES(CYCLES_BTS_REG_MEM);
	}
}

static void I386OP(call_abs16)(void)
{
	UINT16 offset = FETCH16();
	UINT16 ptr = FETCH16();

	if( PROTECTED_MODE && !V8086_MODE)
	{
		i386_protected_mode_call(ptr,offset,0,0);
	}
	else
	{
		PUSH16(I.sreg[CS].selector );
		PUSH16(I.eip );
		I.sreg[CS].selector = ptr;
		I.performed_intersegment_jump = 1;
		I.eip = offset;
		i386_load_segment_descriptor(CS);
	}
	CYCLES(CYCLES_CALL_INTERSEG);
	CHANGE_PC(I.eip);
}

static void I386OP(call_rel16)(void)
{
	INT16 disp = FETCH16();

	PUSH16(I.eip );
	if (I.sreg[CS].d)
	{
		I.eip += disp;
	}
	else
	{
		I.eip = (I.eip + disp) & 0xffff;
	}
	CHANGE_PC(I.eip);
	CYCLES(CYCLES_CALL);
}

static void I386OP(cbw)(void)
{
	REG16(AX) = (INT16)((INT8)REG8(AL));
	CYCLES(CYCLES_CBW);
}

static void I386OP(cmp_rm16_r16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		SUB16(dst, src);
		CYCLES(CYCLES_CMP_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = READ16(ea);
		SUB16(dst, src);
		CYCLES(CYCLES_CMP_REG_MEM);
	}
}

static void I386OP(cmp_r16_rm16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM16(modrm);
		dst = LOAD_REG16(modrm);
		SUB16(dst, src);
		CYCLES(CYCLES_CMP_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ16(ea);
		dst = LOAD_REG16(modrm);
		SUB16(dst, src);
		CYCLES(CYCLES_CMP_MEM_REG);
	}
}

static void I386OP(cmp_ax_i16)(void)
{
	UINT16 src, dst;
	src = FETCH16();
	dst = REG16(AX);
	SUB16(dst, src);
	CYCLES(CYCLES_CMP_IMM_ACC);
}

static void I386OP(cmpsw)(void)
{
	UINT32 eas, ead;
	UINT16 src, dst;
	if( I.segment_prefix ) {
		eas = i386_translate(I.segment_override, I.address_size ? REG32(ESI) : REG16(SI) );
	} else {
		eas = i386_translate(DS, I.address_size ? REG32(ESI) : REG16(SI) );
	}
	ead = i386_translate(ES, I.address_size ? REG32(EDI) : REG16(DI) );
	src = READ16(eas);
	dst = READ16(ead);
	SUB16(dst, src);
	BUMP_SI(2);
	BUMP_DI(2);
	CYCLES(CYCLES_CMPS);
}

static void I386OP(cwd)(void)
{
	if( REG16(AX) & 0x8000 ) {
		REG16(DX) = 0xffff;
	} else {
		REG16(DX) = 0x0000;
	}
	CYCLES(CYCLES_CWD);
}

static void I386OP(dec_ax)(void)
{
	REG16(AX) = DEC16(REG16(AX) );
	CYCLES(CYCLES_DEC_REG);
}

static void I386OP(dec_cx)(void)
{
	REG16(CX) = DEC16(REG16(CX) );
	CYCLES(CYCLES_DEC_REG);
}

static void I386OP(dec_dx)(void)
{
	REG16(DX) = DEC16(REG16(DX) );
	CYCLES(CYCLES_DEC_REG);
}

static void I386OP(dec_bx)(void)
{
	REG16(BX) = DEC16(REG16(BX) );
	CYCLES(CYCLES_DEC_REG);
}

static void I386OP(dec_sp)(void)
{
	REG16(SP) = DEC16(REG16(SP) );
	CYCLES(CYCLES_DEC_REG);
}

static void I386OP(dec_bp)(void)
{
	REG16(BP) = DEC16(REG16(BP) );
	CYCLES(CYCLES_DEC_REG);
}

static void I386OP(dec_si)(void)
{
	REG16(SI) = DEC16(REG16(SI) );
	CYCLES(CYCLES_DEC_REG);
}

static void I386OP(dec_di)(void)
{
	REG16(DI) = DEC16(REG16(DI) );
	CYCLES(CYCLES_DEC_REG);
}

static void I386OP(imul_r16_rm16)(void)
{
	UINT8 modrm = FETCH();
	INT32 result;
	INT32 src, dst;
	if( modrm >= 0xc0 ) {
		src = (INT32)(INT16)LOAD_RM16(modrm);
		CYCLES(CYCLES_IMUL16_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = (INT32)(INT16)READ16(ea);
		CYCLES(CYCLES_IMUL16_REG_MEM);
	}

	dst = (INT32)(INT16)LOAD_REG16(modrm);
	result = src * dst;

	STORE_REG16(modrm, (UINT16)result);

	I.CF = I.OF = !(result == (INT32)(INT16)result);
}

static void I386OP(imul_r16_rm16_i16)(void)
{
	UINT8 modrm = FETCH();
	INT32 result;
	INT32 src, dst;
	if( modrm >= 0xc0 ) {
		dst = (INT32)(INT16)LOAD_RM16(modrm);
		CYCLES(CYCLES_IMUL16_REG_IMM_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		dst = (INT32)(INT16)READ16(ea);
		CYCLES(CYCLES_IMUL16_MEM_IMM_REG);
	}

	src = (INT32)(INT16)FETCH16();
	result = src * dst;

	STORE_REG16(modrm, (UINT16)result);

	I.CF = I.OF = !(result == (INT32)(INT16)result);
}

static void I386OP(imul_r16_rm16_i8)(void)
{
	UINT8 modrm = FETCH();
	INT32 result;
	INT32 src, dst;
	if( modrm >= 0xc0 ) {
		dst = (INT32)(INT16)LOAD_RM16(modrm);
		CYCLES(CYCLES_IMUL16_REG_IMM_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		dst = (INT32)(INT16)READ16(ea);
		CYCLES(CYCLES_IMUL16_MEM_IMM_REG);
	}

	src = (INT32)(INT8)FETCH();
	result = src * dst;

	STORE_REG16(modrm, (UINT16)result);

	I.CF = I.OF = !(result == (INT32)(INT16)result);
}

static void I386OP(in_ax_i8)(void)
{
	UINT16 port = FETCH();
	UINT16 data = READPORT16(port);
	REG16(AX) = data;
	CYCLES(CYCLES_IN_VAR);
}

static void I386OP(in_ax_dx)(void)
{
	UINT16 port = REG16(DX);
	UINT16 data = READPORT16(port);
	REG16(AX) = data;
	CYCLES(CYCLES_IN);
}

static void I386OP(inc_ax)(void)
{
	REG16(AX) = INC16(REG16(AX) );
	CYCLES(CYCLES_INC_REG);
}

static void I386OP(inc_cx)(void)
{
	REG16(CX) = INC16(REG16(CX) );
	CYCLES(CYCLES_INC_REG);
}

static void I386OP(inc_dx)(void)
{
	REG16(DX) = INC16(REG16(DX) );
	CYCLES(CYCLES_INC_REG);
}

static void I386OP(inc_bx)(void)
{
	REG16(BX) = INC16(REG16(BX) );
	CYCLES(CYCLES_INC_REG);
}

static void I386OP(inc_sp)(void)
{
	REG16(SP) = INC16(REG16(SP) );
	CYCLES(CYCLES_INC_REG);
}

static void I386OP(inc_bp)(void)
{
	REG16(BP) = INC16(REG16(BP) );
	CYCLES(CYCLES_INC_REG);
}

static void I386OP(inc_si)(void)
{
	REG16(SI) = INC16(REG16(SI) );
	CYCLES(CYCLES_INC_REG);
}

static void I386OP(inc_di)(void)
{
	REG16(DI) = INC16(REG16(DI) );
	CYCLES(CYCLES_INC_REG);
}

static void I386OP(iret16)(void)
{
	if( PROTECTED_MODE )
	{
		i386_protected_mode_iret(0);
	}
	else
	{

		I.eip = POP16();
		I.sreg[CS].selector = POP16();
		set_flags(POP16() );
		i386_load_segment_descriptor(CS);
		CHANGE_PC(I.eip);
	}
	CYCLES(CYCLES_IRET);
}

static void I386OP(ja_rel16)(void)
{
	INT16 disp = FETCH16();
	if( I.CF == 0 && I.ZF == 0 ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jbe_rel16)(void)
{
	INT16 disp = FETCH16();
	if( I.CF != 0 || I.ZF != 0 ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jc_rel16)(void)
{
	INT16 disp = FETCH16();
	if( I.CF != 0 ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jg_rel16)(void)
{
	INT16 disp = FETCH16();
	if( I.ZF == 0 && (I.SF == I.OF) ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jge_rel16)(void)
{
	INT16 disp = FETCH16();
	if( (I.SF == I.OF) ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jl_rel16)(void)
{
	INT16 disp = FETCH16();
	if( (I.SF != I.OF) ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jle_rel16)(void)
{
	INT16 disp = FETCH16();
	if( I.ZF != 0 || (I.SF != I.OF) ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jnc_rel16)(void)
{
	INT16 disp = FETCH16();
	if( I.CF == 0 ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jno_rel16)(void)
{
	INT16 disp = FETCH16();
	if( I.OF == 0 ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jnp_rel16)(void)
{
	INT16 disp = FETCH16();
	if( I.PF == 0 ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jns_rel16)(void)
{
	INT16 disp = FETCH16();
	if( I.SF == 0 ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jnz_rel16)(void)
{
	INT16 disp = FETCH16();
	if( I.ZF == 0 ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jo_rel16)(void)
{
	INT16 disp = FETCH16();
	if( I.OF != 0 ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jp_rel16)(void)
{
	INT16 disp = FETCH16();
	if( I.PF != 0 ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(js_rel16)(void)
{
	INT16 disp = FETCH16();
	if( I.SF != 0 ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jz_rel16)(void)
{
	INT16 disp = FETCH16();
	if( I.ZF != 0 ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jcxz16)(void)
{
	INT8 disp = FETCH();
	if( REG16(CX) == 0 ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCXZ);
	} else {
		CYCLES(CYCLES_JCXZ_NOBRANCH);
	}
}

static void I386OP(jmp_rel16)(void)
{
	INT16 disp = FETCH16();

	if (I.sreg[CS].d)
	{
		I.eip += disp;
	}
	else
	{
		I.eip = (I.eip + disp) & 0xffff;
	}
	CHANGE_PC(I.eip);
	CYCLES(CYCLES_JMP);
}

static void I386OP(jmp_abs16)(void)
{
	UINT16 address = FETCH16();
	UINT16 segment = FETCH16();

	if( PROTECTED_MODE && !V8086_MODE)
	{
		i386_protected_mode_jump(segment,address,0,0);
	}
	else
	{
		I.eip = address;
		I.sreg[CS].selector = segment;
		I.performed_intersegment_jump = 1;
		i386_load_segment_descriptor(CS);
		CHANGE_PC(I.eip);
	}
	CYCLES(CYCLES_JMP_INTERSEG);
}

static void I386OP(lea16)(void)
{
	UINT8 modrm = FETCH();
	UINT32 ea = GetNonTranslatedEA(modrm);
	STORE_REG16(modrm, ea);
	CYCLES(CYCLES_LEA);
}

static void I386OP(enter16)(void)
{
	UINT16 framesize = FETCH16();
	UINT8 level = FETCH() % 32;
	UINT8 x;
	UINT16 frameptr;
	PUSH16(REG16(BP));

	if(!STACK_32BIT)
		frameptr = REG16(SP);
	else
		frameptr = REG32(ESP);

	if(level > 0)
	{
		for(x=1;x<level-1;x++)
		{
			REG16(BP) -= 2;
			PUSH16(READ16(REG16(BP)));
		}
		PUSH16(frameptr);
	}
	REG16(BP) = frameptr;
	if(!STACK_32BIT)
		REG16(SP) -= framesize;
	else
		REG32(ESP) -= framesize;
	CYCLES(CYCLES_ENTER);
}

static void I386OP(leave16)(void)
{
	if(!STACK_32BIT)
		REG16(SP) = REG16(BP);
	else
		REG32(ESP) = REG32(EBP);
	REG16(BP) = POP16();
	CYCLES(CYCLES_LEAVE);
}

static void I386OP(lodsw)(void)
{
	UINT32 eas;
	if( I.segment_prefix ) {
		eas = i386_translate(I.segment_override, I.address_size ? REG32(ESI) : REG16(SI) );
	} else {
		eas = i386_translate(DS, I.address_size ? REG32(ESI) : REG16(SI) );
	}
	REG16(AX) = READ16(eas);
	BUMP_SI(2);
	CYCLES(CYCLES_LODS);
}

static void I386OP(loop16)(void)
{
	INT8 disp = FETCH();
	REG16(CX)--;
	if( REG16(CX) != 0 ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
	}
	CYCLES(CYCLES_LOOP);
}

static void I386OP(loopne16)(void)
{
	INT8 disp = FETCH();
	REG16(CX)--;
	if( REG16(CX) != 0 && I.ZF == 0 ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
	}
	CYCLES(CYCLES_LOOPNZ);
}

static void I386OP(loopz16)(void)
{
	INT8 disp = FETCH();
	REG16(CX)--;
	if( REG16(CX) != 0 && I.ZF != 0 ) {
		if (I.sreg[CS].d)
		{
			I.eip += disp;
		}
		else
		{
			I.eip = (I.eip + disp) & 0xffff;
		}
		CHANGE_PC(I.eip);
	}
	CYCLES(CYCLES_LOOPZ);
}

static void I386OP(mov_rm16_r16)(void)
{
	UINT16 src;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG16(modrm);
		STORE_RM16(modrm, src);
		CYCLES(CYCLES_MOV_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		WRITE16(ea, src);
		CYCLES(CYCLES_MOV_REG_MEM);
	}
}

static void I386OP(mov_r16_rm16)(void)
{
	UINT16 src;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM16(modrm);
		STORE_REG16(modrm, src);
		CYCLES(CYCLES_MOV_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ16(ea);
		STORE_REG16(modrm, src);
		CYCLES(CYCLES_MOV_MEM_REG);
	}
}

static void I386OP(mov_rm16_i16)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT16 value = FETCH16();
		STORE_RM16(modrm, value);
		CYCLES(CYCLES_MOV_IMM_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT16 value = FETCH16();
		WRITE16(ea, value);
		CYCLES(CYCLES_MOV_IMM_MEM);
	}
}

static void I386OP(mov_ax_m16)(void)
{
	UINT32 offset, ea;
	if( I.address_size ) {
		offset = FETCH32();
	} else {
		offset = FETCH16();
	}

	if( I.segment_prefix ) {
		ea = i386_translate(I.segment_override, offset );
	} else {
		ea = i386_translate(DS, offset );
	}
	REG16(AX) = READ16(ea);
	CYCLES(CYCLES_MOV_MEM_ACC);
}

static void I386OP(mov_m16_ax)(void)
{
	UINT32 offset, ea;
	if( I.address_size ) {
		offset = FETCH32();
	} else {
		offset = FETCH16();
	}

	if( I.segment_prefix ) {
		ea = i386_translate(I.segment_override, offset );
	} else {
		ea = i386_translate(DS, offset );
	}
	WRITE16(ea, REG16(AX) );
	CYCLES(CYCLES_MOV_ACC_MEM);
}

static void I386OP(mov_ax_i16)(void)
{
	REG16(AX) = FETCH16();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_cx_i16)(void)
{
	REG16(CX) = FETCH16();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_dx_i16)(void)
{
	REG16(DX) = FETCH16();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_bx_i16)(void)
{
	REG16(BX) = FETCH16();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_sp_i16)(void)
{
	REG16(SP) = FETCH16();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_bp_i16)(void)
{
	REG16(BP) = FETCH16();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_si_i16)(void)
{
	REG16(SI) = FETCH16();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_di_i16)(void)
{
	REG16(DI) = FETCH16();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(movsw)(void)
{
	UINT32 eas, ead;
	UINT16 v;
	if( I.segment_prefix ) {
		eas = i386_translate(I.segment_override, I.address_size ? REG32(ESI) : REG16(SI) );
	} else {
		eas = i386_translate(DS, I.address_size ? REG32(ESI) : REG16(SI) );
	}
	ead = i386_translate(ES, I.address_size ? REG32(EDI) : REG16(DI) );
	v = READ16(eas);
	WRITE16(ead, v);
	BUMP_SI(2);
	BUMP_DI(2);
	CYCLES(CYCLES_MOVS);
}

static void I386OP(movsx_r16_rm8)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		INT16 src = (INT8)LOAD_RM8(modrm);
		STORE_REG16(modrm, src);
		CYCLES(CYCLES_MOVSX_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		INT16 src = (INT8)READ8(ea);
		STORE_REG16(modrm, src);
		CYCLES(CYCLES_MOVSX_MEM_REG);
	}
}

static void I386OP(movzx_r16_rm8)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT16 src = (UINT8)LOAD_RM8(modrm);
		STORE_REG16(modrm, src);
		CYCLES(CYCLES_MOVZX_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT16 src = (UINT8)READ8(ea);
		STORE_REG16(modrm, src);
		CYCLES(CYCLES_MOVZX_MEM_REG);
	}
}

static void I386OP(or_rm16_r16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		dst = OR16(dst, src);
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = READ16(ea);
		dst = OR16(dst, src);
		WRITE16(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(or_r16_rm16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM16(modrm);
		dst = LOAD_REG16(modrm);
		dst = OR16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ16(ea);
		dst = LOAD_REG16(modrm);
		dst = OR16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(or_ax_i16)(void)
{
	UINT16 src, dst;
	src = FETCH16();
	dst = REG16(AX);
	dst = OR16(dst, src);
	REG16(AX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(out_ax_i8)(void)
{
	UINT16 port = FETCH();
	UINT16 data = REG16(AX);
	WRITEPORT16(port, data);
	CYCLES(CYCLES_OUT_VAR);
}

static void I386OP(out_ax_dx)(void)
{
	UINT16 port = REG16(DX);
	UINT16 data = REG16(AX);
	WRITEPORT16(port, data);
	CYCLES(CYCLES_OUT);
}

static void I386OP(pop_ax)(void)
{
	REG16(AX) = POP16();
	CYCLES(CYCLES_POP_REG_SHORT);
}

static void I386OP(pop_cx)(void)
{
	REG16(CX) = POP16();
	CYCLES(CYCLES_POP_REG_SHORT);
}

static void I386OP(pop_dx)(void)
{
	REG16(DX) = POP16();
	CYCLES(CYCLES_POP_REG_SHORT);
}

static void I386OP(pop_bx)(void)
{
	REG16(BX) = POP16();
	CYCLES(CYCLES_POP_REG_SHORT);
}

static void I386OP(pop_sp)(void)
{
	REG16(SP) = POP16();
	CYCLES(CYCLES_POP_REG_SHORT);
}

static void I386OP(pop_bp)(void)
{
	REG16(BP) = POP16();
	CYCLES(CYCLES_POP_REG_SHORT);
}

static void I386OP(pop_si)(void)
{
	REG16(SI) = POP16();
	CYCLES(CYCLES_POP_REG_SHORT);
}

static void I386OP(pop_di)(void)
{
	REG16(DI) = POP16();
	CYCLES(CYCLES_POP_REG_SHORT);
}

static void I386OP(pop_ds16)(void)
{
	I.sreg[DS].selector = POP16();
	if( PROTECTED_MODE ) {
		i386_load_segment_descriptor(DS);
	} else {
		i386_load_segment_descriptor(DS);
	}
	CYCLES(CYCLES_POP_SREG);
}

static void I386OP(pop_es16)(void)
{
	I.sreg[ES].selector = POP16();
	if( PROTECTED_MODE ) {
		i386_load_segment_descriptor(ES);
	} else {
		i386_load_segment_descriptor(ES);
	}
	CYCLES(CYCLES_POP_SREG);
}

static void I386OP(pop_fs16)(void)
{
	I.sreg[FS].selector = POP16();
	if( PROTECTED_MODE ) {
		i386_load_segment_descriptor(FS);
	} else {
		i386_load_segment_descriptor(FS);
	}
	CYCLES(CYCLES_POP_SREG);
}

static void I386OP(pop_gs16)(void)
{
	I.sreg[GS].selector = POP16();
	if( PROTECTED_MODE ) {
		i386_load_segment_descriptor(GS);
	} else {
		i386_load_segment_descriptor(GS);
	}
	CYCLES(CYCLES_POP_SREG);
}

static void I386OP(pop_ss16)(void)
{
	if(I.IF != 0)
	{
		I.IF = 0;
		I.delayed_interrupt_enable = 1;
	}

	I.sreg[SS].selector = POP16();
	if( PROTECTED_MODE ) {
		i386_load_segment_descriptor(SS);
	} else {
		i386_load_segment_descriptor(SS);
	}
	CYCLES(CYCLES_POP_SREG);
}

static void I386OP(pop_rm16)(void)
{
	UINT8 modrm = FETCH();
	UINT16 value = POP16();

	if( modrm >= 0xc0 ) {
		STORE_RM16(modrm, value);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE16(ea, value);
	}
	CYCLES(CYCLES_POP_RM);
}

static void I386OP(popa)(void)
{
	REG16(DI) = POP16();
	REG16(SI) = POP16();
	REG16(BP) = POP16();
	REG16(SP) += 2;
	REG16(BX) = POP16();
	REG16(DX) = POP16();
	REG16(CX) = POP16();
	REG16(AX) = POP16();
	CYCLES(CYCLES_POPA);
}

static void I386OP(popf)(void)
{
	UINT16 value = POP16();
	set_flags(value);
	CYCLES(CYCLES_POPF);
}

static void I386OP(push_ax)(void)
{
	PUSH16(REG16(AX) );
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

static void I386OP(push_cx)(void)
{
	PUSH16(REG16(CX) );
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

static void I386OP(push_dx)(void)
{
	PUSH16(REG16(DX) );
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

static void I386OP(push_bx)(void)
{
	PUSH16(REG16(BX) );
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

static void I386OP(push_sp)(void)
{
	PUSH16(REG16(SP) );
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

static void I386OP(push_bp)(void)
{
	PUSH16(REG16(BP) );
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

static void I386OP(push_si)(void)
{
	PUSH16(REG16(SI) );
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

static void I386OP(push_di)(void)
{
	PUSH16(REG16(DI) );
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

static void I386OP(push_cs16)(void)
{
	PUSH16(I.sreg[CS].selector );
	CYCLES(CYCLES_PUSH_SREG);
}

static void I386OP(push_ds16)(void)
{
	PUSH16(I.sreg[DS].selector );
	CYCLES(CYCLES_PUSH_SREG);
}

static void I386OP(push_es16)(void)
{
	PUSH16(I.sreg[ES].selector );
	CYCLES(CYCLES_PUSH_SREG);
}

static void I386OP(push_fs16)(void)
{
	PUSH16(I.sreg[FS].selector );
	CYCLES(CYCLES_PUSH_SREG);
}

static void I386OP(push_gs16)(void)
{
	PUSH16(I.sreg[GS].selector );
	CYCLES(CYCLES_PUSH_SREG);
}

static void I386OP(push_ss16)(void)
{
	PUSH16(I.sreg[SS].selector );
	CYCLES(CYCLES_PUSH_SREG);
}

static void I386OP(push_i16)(void)
{
	UINT16 value = FETCH16();
	PUSH16(value);
	CYCLES(CYCLES_PUSH_IMM);
}

static void I386OP(pusha)(void)
{
	UINT16 temp = REG16(SP);
	PUSH16(REG16(AX) );
	PUSH16(REG16(CX) );
	PUSH16(REG16(DX) );
	PUSH16(REG16(BX) );
	PUSH16(temp );
	PUSH16(REG16(BP) );
	PUSH16(REG16(SI) );
	PUSH16(REG16(DI) );
	CYCLES(CYCLES_PUSHA);
}

static void I386OP(pushf)(void)
{
	PUSH16(get_flags() & 0xffff );
	CYCLES(CYCLES_PUSHF);
}

static void I386OP(ret_near16_i16)(void)
{
	INT16 disp = FETCH16();
	I.eip = POP16();
	REG16(SP) += disp;
	CHANGE_PC(I.eip);
	CYCLES(CYCLES_RET_IMM);
}

static void I386OP(ret_near16)(void)
{
	I.eip = POP16();
	CHANGE_PC(I.eip);
	CYCLES(CYCLES_RET);
}

static void I386OP(sbb_rm16_r16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		dst = SBB16(dst, src, I.CF);
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = READ16(ea);
		dst = SBB16(dst, src, I.CF);
		WRITE16(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(sbb_r16_rm16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM16(modrm);
		dst = LOAD_REG16(modrm);
		dst = SBB16(dst, src, I.CF);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ16(ea);
		dst = LOAD_REG16(modrm);
		dst = SBB16(dst, src, I.CF);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(sbb_ax_i16)(void)
{
	UINT16 src, dst;
	src = FETCH16();
	dst = REG16(AX);
	dst = SBB16(dst, src, I.CF);
	REG16(AX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(scasw)(void)
{
	UINT32 eas;
	UINT16 src, dst;
	eas = i386_translate(ES, I.address_size ? REG32(EDI) : REG16(DI) );
	src = READ16(eas);
	dst = REG16(AX);
	SUB16(dst, src);
	BUMP_DI(2);
	CYCLES(CYCLES_SCAS);
}

static void I386OP(shld16_i8)(void)
{

	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT16 dst = LOAD_RM16(modrm);
		UINT16 upper = LOAD_REG16(modrm);
		UINT8 shift = FETCH();
		if( shift > 31 || shift == 0 ) {

		} else if( shift > 15 ) {
			I.CF = (dst & (1 << (16-shift))) ? 1 : 0;
			dst = (upper << (shift-16)) | (upper >> (32-shift));
			SetSZPF16(dst);
		} else {
			I.CF = (dst & (1 << (16-shift))) ? 1 : 0;
			dst = (dst << shift) | (upper >> (16-shift));
			SetSZPF16(dst);
		}
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_SHLD_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT16 dst = READ16(ea);
		UINT16 upper = LOAD_REG16(modrm);
		UINT8 shift = FETCH();
		if( shift > 31 || shift == 0 ) {

		} else if( shift > 15 ) {
			I.CF = (dst & (1 << (16-shift))) ? 1 : 0;
			dst = (upper << (shift-16)) | (upper >> (32-shift));
			SetSZPF16(dst);
		} else {
			I.CF = (dst & (1 << (16-shift))) ? 1 : 0;
			dst = (dst << shift) | (upper >> (16-shift));
			SetSZPF16(dst);
		}
		WRITE16(ea, dst);
		CYCLES(CYCLES_SHLD_MEM);
	}
}

static void I386OP(shld16_cl)(void)
{

	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT16 dst = LOAD_RM16(modrm);
		UINT16 upper = LOAD_REG16(modrm);
		UINT8 shift = REG8(CL);
		if( shift > 31 || shift == 0 ) {

		} else if( shift > 15 ) {
			I.CF = (dst & (1 << (16-shift))) ? 1 : 0;
			dst = (upper << (shift-16)) | (upper >> (32-shift));
			SetSZPF16(dst);
		} else {
			I.CF = (dst & (1 << (16-shift))) ? 1 : 0;
			dst = (dst << shift) | (upper >> (16-shift));
			SetSZPF16(dst);
		}
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_SHLD_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT16 dst = READ16(ea);
		UINT16 upper = LOAD_REG16(modrm);
		UINT8 shift = REG8(CL);
		if( shift > 31 || shift == 0 ) {

		} else if( shift > 15 ) {
			I.CF = (dst & (1 << (16-shift))) ? 1 : 0;
			dst = (upper << (shift-16)) | (upper >> (32-shift));
			SetSZPF16(dst);
		} else {
			I.CF = (dst & (1 << (16-shift))) ? 1 : 0;
			dst = (dst << shift) | (upper >> (16-shift));
			SetSZPF16(dst);
		}
		WRITE16(ea, dst);
		CYCLES(CYCLES_SHLD_MEM);
	}
}

static void I386OP(shrd16_i8)(void)
{

	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT16 dst = LOAD_RM16(modrm);
		UINT16 upper = LOAD_REG16(modrm);
		UINT8 shift = FETCH();
		if( shift > 15 || shift == 0) {

		} else {
			I.CF = (dst & (1 << (shift-1))) ? 1 : 0;
			dst = (dst >> shift) | (upper << (16-shift));
			SetSZPF16(dst);
		}
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_SHRD_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT16 dst = READ16(ea);
		UINT16 upper = LOAD_REG16(modrm);
		UINT8 shift = FETCH();
		if( shift > 15 || shift == 0) {

		} else {
			I.CF = (dst & (1 << (shift-1))) ? 1 : 0;
			dst = (dst >> shift) | (upper << (16-shift));
			SetSZPF16(dst);
		}
		WRITE16(ea, dst);
		CYCLES(CYCLES_SHRD_MEM);
	}
}

static void I386OP(shrd16_cl)(void)
{

	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT16 dst = LOAD_RM16(modrm);
		UINT16 upper = LOAD_REG16(modrm);
		UINT8 shift = REG8(CL);
		if( shift > 15 || shift == 0) {

		} else {
			I.CF = (dst & (1 << (shift-1))) ? 1 : 0;
			dst = (dst >> shift) | (upper << (16-shift));
			SetSZPF16(dst);
		}
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_SHRD_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT16 dst = READ16(ea);
		UINT16 upper = LOAD_REG16(modrm);
		UINT8 shift = REG8(CL);
		if( shift > 15 || shift == 0) {

		} else {
			I.CF = (dst & (1 << (shift-1))) ? 1 : 0;
			dst = (dst >> shift) | (upper << (16-shift));
			SetSZPF16(dst);
		}
		WRITE16(ea, dst);
		CYCLES(CYCLES_SHRD_MEM);
	}
}

static void I386OP(stosw)(void)
{
	UINT32 ead;
	ead = i386_translate(ES, I.address_size ? REG32(EDI) : REG16(DI) );
	WRITE16(ead, REG16(AX));
	BUMP_DI(2);
	CYCLES(CYCLES_STOS);
}

static void I386OP(sub_rm16_r16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		dst = SUB16(dst, src);
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = READ16(ea);
		dst = SUB16(dst, src);
		WRITE16(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(sub_r16_rm16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM16(modrm);
		dst = LOAD_REG16(modrm);
		dst = SUB16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ16(ea);
		dst = LOAD_REG16(modrm);
		dst = SUB16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(sub_ax_i16)(void)
{
	UINT16 src, dst;
	src = FETCH16();
	dst = REG16(AX);
	dst = SUB16(dst, src);
	REG16(AX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(test_ax_i16)(void)
{
	UINT16 src = FETCH16();
	UINT16 dst = REG16(AX);
	dst = src & dst;
	SetSZPF16(dst);
	I.CF = 0;
	I.OF = 0;
	CYCLES(CYCLES_TEST_IMM_ACC);
}

static void I386OP(test_rm16_r16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		dst = src & dst;
		SetSZPF16(dst);
		I.CF = 0;
		I.OF = 0;
		CYCLES(CYCLES_TEST_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = READ16(ea);
		dst = src & dst;
		SetSZPF16(dst);
		I.CF = 0;
		I.OF = 0;
		CYCLES(CYCLES_TEST_REG_MEM);
	}
}

static void I386OP(xchg_ax_cx)(void)
{
	UINT16 temp;
	temp = REG16(AX);
	REG16(AX) = REG16(CX);
	REG16(CX) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

static void I386OP(xchg_ax_dx)(void)
{
	UINT16 temp;
	temp = REG16(AX);
	REG16(AX) = REG16(DX);
	REG16(DX) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

static void I386OP(xchg_ax_bx)(void)
{
	UINT16 temp;
	temp = REG16(AX);
	REG16(AX) = REG16(BX);
	REG16(BX) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

static void I386OP(xchg_ax_sp)(void)
{
	UINT16 temp;
	temp = REG16(AX);
	REG16(AX) = REG16(SP);
	REG16(SP) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

static void I386OP(xchg_ax_bp)(void)
{
	UINT16 temp;
	temp = REG16(AX);
	REG16(AX) = REG16(BP);
	REG16(BP) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

static void I386OP(xchg_ax_si)(void)
{
	UINT16 temp;
	temp = REG16(AX);
	REG16(AX) = REG16(SI);
	REG16(SI) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

static void I386OP(xchg_ax_di)(void)
{
	UINT16 temp;
	temp = REG16(AX);
	REG16(AX) = REG16(DI);
	REG16(DI) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

static void I386OP(xchg_r16_rm16)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT16 src = LOAD_RM16(modrm);
		UINT16 dst = LOAD_REG16(modrm);
		STORE_REG16(modrm, src);
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_XCHG_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT16 src = READ16(ea);
		UINT16 dst = LOAD_REG16(modrm);
		STORE_REG16(modrm, src);
		WRITE16(ea, dst);
		CYCLES(CYCLES_XCHG_REG_MEM);
	}
}

static void I386OP(xor_rm16_r16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		dst = XOR16(dst, src);
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = READ16(ea);
		dst = XOR16(dst, src);
		WRITE16(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(xor_r16_rm16)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM16(modrm);
		dst = LOAD_REG16(modrm);
		dst = XOR16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ16(ea);
		dst = LOAD_REG16(modrm);
		dst = XOR16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(xor_ax_i16)(void)
{
	UINT16 src, dst;
	src = FETCH16();
	dst = REG16(AX);
	dst = XOR16(dst, src);
	REG16(AX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}


static void I386OP(group81_16)(void)
{
	UINT32 ea;
	UINT16 src, dst;
	UINT8 modrm = FETCH();

	switch( (modrm >> 3) & 0x7 )
	{
		case 0:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM16(modrm);
				src = FETCH16();
				dst = ADD16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ16(ea);
				src = FETCH16();
				dst = ADD16(dst, src);
				WRITE16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 1:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM16(modrm);
				src = FETCH16();
				dst = OR16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ16(ea);
				src = FETCH16();
				dst = OR16(dst, src);
				WRITE16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 2:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM16(modrm);
				src = FETCH16();
				dst = ADC16(dst, src, I.CF);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ16(ea);
				src = FETCH16();
				dst = ADC16(dst, src, I.CF);
				WRITE16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 3:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM16(modrm);
				src = FETCH16();
				dst = SBB16(dst, src, I.CF);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ16(ea);
				src = FETCH16();
				dst = SBB16(dst, src, I.CF);
				WRITE16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 4:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM16(modrm);
				src = FETCH16();
				dst = AND16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ16(ea);
				src = FETCH16();
				dst = AND16(dst, src);
				WRITE16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 5:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM16(modrm);
				src = FETCH16();
				dst = SUB16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ16(ea);
				src = FETCH16();
				dst = SUB16(dst, src);
				WRITE16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 6:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM16(modrm);
				src = FETCH16();
				dst = XOR16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ16(ea);
				src = FETCH16();
				dst = XOR16(dst, src);
				WRITE16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 7:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM16(modrm);
				src = FETCH16();
				SUB16(dst, src);
				CYCLES(CYCLES_CMP_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ16(ea);
				src = FETCH16();
				SUB16(dst, src);
				CYCLES(CYCLES_CMP_REG_MEM);
			}
			break;
	}
}

static void I386OP(group83_16)(void)
{
	UINT32 ea;
	UINT16 src, dst;
	UINT8 modrm = FETCH();

	switch( (modrm >> 3) & 0x7 )
	{
		case 0:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM16(modrm);
				src = (UINT16)(INT16)(INT8)FETCH();
				dst = ADD16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ16(ea);
				src = (UINT16)(INT16)(INT8)FETCH();
				dst = ADD16(dst, src);
				WRITE16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 1:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM16(modrm);
				src = (UINT16)(INT16)(INT8)FETCH();
				dst = OR16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ16(ea);
				src = (UINT16)(INT16)(INT8)FETCH();
				dst = OR16(dst, src);
				WRITE16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 2:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM16(modrm);
				src = (UINT16)(INT16)(INT8)FETCH();
				dst = ADC16(dst, src, I.CF);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ16(ea);
				src = (UINT16)(INT16)(INT8)FETCH();
				dst = ADC16(dst, src, I.CF);
				WRITE16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 3:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM16(modrm);
				src = ((UINT16)(INT16)(INT8)FETCH());
				dst = SBB16(dst, src, I.CF);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ16(ea);
				src = ((UINT16)(INT16)(INT8)FETCH());
				dst = SBB16(dst, src, I.CF);
				WRITE16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 4:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM16(modrm);
				src = (UINT16)(INT16)(INT8)FETCH();
				dst = AND16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ16(ea);
				src = (UINT16)(INT16)(INT8)FETCH();
				dst = AND16(dst, src);
				WRITE16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 5:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM16(modrm);
				src = (UINT16)(INT16)(INT8)FETCH();
				dst = SUB16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ16(ea);
				src = (UINT16)(INT16)(INT8)FETCH();
				dst = SUB16(dst, src);
				WRITE16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 6:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM16(modrm);
				src = (UINT16)(INT16)(INT8)FETCH();
				dst = XOR16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ16(ea);
				src = (UINT16)(INT16)(INT8)FETCH();
				dst = XOR16(dst, src);
				WRITE16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 7:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM16(modrm);
				src = (UINT16)(INT16)(INT8)FETCH();
				SUB16(dst, src);
				CYCLES(CYCLES_CMP_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ16(ea);
				src = (UINT16)(INT16)(INT8)FETCH();
				SUB16(dst, src);
				CYCLES(CYCLES_CMP_REG_MEM);
			}
			break;
	}
}

static void I386OP(groupC1_16)(void)
{
	UINT16 dst;
	UINT8 modrm = FETCH();
	UINT8 shift;

	if( modrm >= 0xc0 ) {
		dst = LOAD_RM16(modrm);
		shift = FETCH() & 0x1f;
		dst = i386_shift_rotate16(modrm, dst, shift);
		STORE_RM16(modrm, dst);
	} else {
		UINT32 ea = GetEA(modrm);
		dst = READ16(ea);
		shift = FETCH() & 0x1f;
		dst = i386_shift_rotate16(modrm, dst, shift);
		WRITE16(ea, dst);
	}
}

static void I386OP(groupD1_16)(void)
{
	UINT16 dst;
	UINT8 modrm = FETCH();

	if( modrm >= 0xc0 ) {
		dst = LOAD_RM16(modrm);
		dst = i386_shift_rotate16(modrm, dst, 1);
		STORE_RM16(modrm, dst);
	} else {
		UINT32 ea = GetEA(modrm);
		dst = READ16(ea);
		dst = i386_shift_rotate16(modrm, dst, 1);
		WRITE16(ea, dst);
	}
}

static void I386OP(groupD3_16)(void)
{
	UINT16 dst;
	UINT8 modrm = FETCH();

	if( modrm >= 0xc0 ) {
		dst = LOAD_RM16(modrm);
		dst = i386_shift_rotate16(modrm, dst, REG8(CL));
		STORE_RM16(modrm, dst);
	} else {
		UINT32 ea = GetEA(modrm);
		dst = READ16(ea);
		dst = i386_shift_rotate16(modrm, dst, REG8(CL));
		WRITE16(ea, dst);
	}
}

static void I386OP(groupF7_16)(void)
{
	UINT8 modrm = FETCH();

	switch( (modrm >> 3) & 0x7 )
	{
		case 0:
			if( modrm >= 0xc0 ) {
				UINT16 dst = LOAD_RM16(modrm);
				UINT16 src = FETCH16();
				dst &= src;
				I.CF = I.OF = I.AF = 0;
				SetSZPF16(dst);
				CYCLES(CYCLES_TEST_IMM_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT16 dst = READ16(ea);
				UINT16 src = FETCH16();
				dst &= src;
				I.CF = I.OF = I.AF = 0;
				SetSZPF16(dst);
				CYCLES(CYCLES_TEST_IMM_MEM);
			}
			break;
		case 2:
			if( modrm >= 0xc0 ) {
				UINT16 dst = LOAD_RM16(modrm);
				dst = ~dst;
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_NOT_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT16 dst = READ16(ea);
				dst = ~dst;
				WRITE16(ea, dst);
				CYCLES(CYCLES_NOT_MEM);
			}
			break;
		case 3:
			if( modrm >= 0xc0 ) {
				UINT16 dst = LOAD_RM16(modrm);
				dst = SUB16(0, dst );
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_NEG_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT16 dst = READ16(ea);
				dst = SUB16(0, dst );
				WRITE16(ea, dst);
				CYCLES(CYCLES_NEG_MEM);
			}
			break;
		case 4:
			{
				UINT32 result;
				UINT16 src, dst;
				if( modrm >= 0xc0 ) {
					src = LOAD_RM16(modrm);
					CYCLES(CYCLES_MUL16_ACC_REG);
				} else {
					UINT32 ea = GetEA(modrm);
					src = READ16(ea);
					CYCLES(CYCLES_MUL16_ACC_MEM);
				}

				dst = REG16(AX);
				result = (UINT32)src * (UINT32)dst;
				REG16(DX) = (UINT16)(result >> 16);
				REG16(AX) = (UINT16)result;

				I.CF = I.OF = (REG16(DX) != 0);
			}
			break;
		case 5:
			{
				INT32 result;
				INT32 src, dst;
				if( modrm >= 0xc0 ) {
					src = (INT32)(INT16)LOAD_RM16(modrm);
					CYCLES(CYCLES_IMUL16_ACC_REG);
				} else {
					UINT32 ea = GetEA(modrm);
					src = (INT32)(INT16)READ16(ea);
					CYCLES(CYCLES_IMUL16_ACC_MEM);
				}

				dst = (INT32)(INT16)REG16(AX);
				result = src * dst;

				REG16(DX) = (UINT16)(result >> 16);
				REG16(AX) = (UINT16)result;

				I.CF = I.OF = !(result == (INT32)(INT16)result);
			}
			break;
		case 6:
			{
				UINT32 quotient, remainder, result;
				UINT16 src;
				if( modrm >= 0xc0 ) {
					src = LOAD_RM16(modrm);
					CYCLES(CYCLES_DIV16_ACC_REG);
				} else {
					UINT32 ea = GetEA(modrm);
					src = READ16(ea);
					CYCLES(CYCLES_DIV16_ACC_MEM);
				}

				quotient = ((UINT32)(REG16(DX)) << 16) | (UINT32)(REG16(AX));
				if( src ) {
					remainder = quotient % (UINT32)src;
					result = quotient / (UINT32)src;
					if( result > 0xffff ) {

					} else {
						REG16(DX) = (UINT16)remainder;
						REG16(AX) = (UINT16)result;

						if (I.cpuid_id0 != 0x69727943)
							I.CF = 1;
					}
				} else {

				}
			}
			break;
		case 7:
			{
				INT32 quotient, remainder, result;
				UINT16 src;
				if( modrm >= 0xc0 ) {
					src = LOAD_RM16(modrm);
					CYCLES(CYCLES_IDIV16_ACC_REG);
				} else {
					UINT32 ea = GetEA(modrm);
					src = READ16(ea);
					CYCLES(CYCLES_IDIV16_ACC_MEM);
				}

				quotient = (((INT32)REG16(DX)) << 16) | ((UINT32)REG16(AX));
				if( src ) {
					remainder = quotient % (INT32)(INT16)src;
					result = quotient / (INT32)(INT16)src;
					if( result > 0xffff ) {

					} else {
						REG16(DX) = (UINT16)remainder;
						REG16(AX) = (UINT16)result;

						if (I.cpuid_id0 != 0x69727943)
							I.CF = 1;
					}
				} else {

				}
			}
			break;
	}
}

static void I386OP(groupFF_16)(void)
{
	UINT8 modrm = FETCH();

	switch( (modrm >> 3) & 0x7 )
	{
		case 0:
			if( modrm >= 0xc0 ) {
				UINT16 dst = LOAD_RM16(modrm);
				dst = INC16(dst);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_INC_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT16 dst = READ16(ea);
				dst = INC16(dst);
				WRITE16(ea, dst);
				CYCLES(CYCLES_INC_MEM);
			}
			break;
		case 1:
			if( modrm >= 0xc0 ) {
				UINT16 dst = LOAD_RM16(modrm);
				dst = DEC16(dst);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_DEC_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT16 dst = READ16(ea);
				dst = DEC16(dst);
				WRITE16(ea, dst);
				CYCLES(CYCLES_DEC_MEM);
			}
			break;
		case 2:
			{
				UINT16 address;
				if( modrm >= 0xc0 ) {
					address = LOAD_RM16(modrm);
					CYCLES(CYCLES_CALL_REG);
				} else {
					UINT32 ea = GetEA(modrm);
					address = READ16(ea);
					CYCLES(CYCLES_CALL_MEM);
				}
				PUSH16(I.eip );
				I.eip = address;
				CHANGE_PC(I.eip);
			}
			break;
		case 3:
			{
				UINT16 address, selector;
				if( modrm >= 0xc0 )
				{
					fatalerror("i386: groupFF_16 /%d NYI", (modrm >> 3) & 0x7);
				}
				else
				{
					UINT32 ea = GetEA(modrm);
					address = READ16(ea + 0);
					selector = READ16(ea + 2);
					CYCLES(CYCLES_CALL_MEM_INTERSEG);

					if(PROTECTED_MODE && !V8086_MODE)
					{
						i386_protected_mode_call(selector,address,1,0);
					}
					else
					{
						PUSH16(I.sreg[CS].selector );
						PUSH16(I.eip );
						I.sreg[CS].selector = selector;
						I.performed_intersegment_jump = 1;
						i386_load_segment_descriptor(CS );
						I.eip = address;
						CHANGE_PC(I.eip);
					}
				}
			}
			break;
		case 4:
			{
				UINT16 address;
				if( modrm >= 0xc0 ) {
					address = LOAD_RM16(modrm);
					CYCLES(CYCLES_JMP_REG);
				} else {
					UINT32 ea = GetEA(modrm);
					address = READ16(ea);
					CYCLES(CYCLES_JMP_MEM);
				}
				I.eip = address;
				CHANGE_PC(I.eip);
			}
			break;
		case 5:
			{
				UINT16 address, selector;

				if( modrm >= 0xc0 )
				{
					fatalerror("i386: groupFF_16 /%d NYI", (modrm >> 3) & 0x7);
				}
				else
				{
					UINT32 ea = GetEA(modrm);
					address = READ16(ea + 0);
					selector = READ16(ea + 2);
					CYCLES(CYCLES_JMP_MEM_INTERSEG);
					if(PROTECTED_MODE && !V8086_MODE)
					{
						i386_protected_mode_jump(selector,address,1,0);
					}
					else
					{
						I.sreg[CS].selector = selector;
						I.performed_intersegment_jump = 1;
						i386_load_segment_descriptor(CS );
						I.eip = address;
						CHANGE_PC(I.eip);
					}
				}
			}
			break;
		case 6:
			{
				UINT16 value;
				if( modrm >= 0xc0 ) {
					value = LOAD_RM16(modrm);
				} else {
					UINT32 ea = GetEA(modrm);
					value = READ16(ea);
				}
				PUSH16(value);
				CYCLES(CYCLES_PUSH_RM);
			}
			break;
		case 7:
			I386OP(invalid)();
			break;
		default:
			fatalerror("i386: groupFF_16 /%d unimplemented", (modrm >> 3) & 0x7);
			break;
	}
}

static void I386OP(group0F00_16)(void)
{
	UINT32 address, ea;
	UINT8 modrm = FETCH();
	I386_SREG seg;
	UINT8 result;

	switch( (modrm >> 3) & 0x7 )
	{
		case 0:
			if ( PROTECTED_MODE && !V8086_MODE )
			{
				if( modrm >= 0xc0 ) {
					address = LOAD_RM16(modrm);
					STORE_RM16(address, I.ldtr.segment);
					CYCLES(CYCLES_SLDT_REG);
				} else {
					ea = GetEA(modrm);
					WRITE16(ea, I.ldtr.segment);
					CYCLES(CYCLES_SLDT_MEM);
				}
			}
			else
			{
				i386_trap(6, 0, 0);
			}
			break;
		case 1:
			if ( PROTECTED_MODE && !V8086_MODE )
			{
				if( modrm >= 0xc0 ) {
					address = LOAD_RM16(modrm);
					STORE_RM16(address, I.task.segment);
					CYCLES(CYCLES_STR_REG);
				} else {
					ea = GetEA(modrm);
					WRITE16(ea, I.task.segment);
					CYCLES(CYCLES_STR_MEM);
				}
			}
			else
			{
				i386_trap(6, 0, 0);
			}
			break;
		case 2:
			if ( PROTECTED_MODE && !V8086_MODE )
			{
				if( modrm >= 0xc0 ) {
					address = LOAD_RM16(modrm);
					I.ldtr.segment = address;
					CYCLES(CYCLES_LLDT_REG);
				} else {
					ea = GetEA(modrm);
					I.ldtr.segment = READ16(ea);
					CYCLES(CYCLES_LLDT_MEM);
				}
				memset(&seg, 0, sizeof(seg));
				seg.selector = I.ldtr.segment;
				i386_load_protected_mode_segment(&seg);
				I.ldtr.limit = seg.limit;
				I.ldtr.base = seg.base;
				I.ldtr.flags = seg.flags;
			}
			else
			{
				i386_trap(6, 0, 0);
			}
			break;

		case 3:
			if ( PROTECTED_MODE && !V8086_MODE )
			{
				if( modrm >= 0xc0 ) {
					address = LOAD_RM16(modrm);
					I.task.segment = address;
					CYCLES(CYCLES_LTR_REG);
				} else {
					ea = GetEA(modrm);
					I.task.segment = READ16(ea);
					CYCLES(CYCLES_LTR_MEM);
				}
				memset(&seg, 0, sizeof(seg));
				seg.selector = I.task.segment;
				i386_load_protected_mode_segment(&seg);
				I.task.limit = seg.limit;
				I.task.base = seg.base;
				I.task.flags = seg.flags;
			}
			else
			{
				i386_trap(6, 0, 0);
			}
			break;

		case 4:
			if ( PROTECTED_MODE && !V8086_MODE )
			{
				result = 1;
				if( modrm >= 0xc0 ) {
					address = LOAD_RM16(modrm);
					CYCLES(CYCLES_VERR_REG);
				} else {
					ea = GetEA(modrm);
					address = READ16(ea);
					CYCLES(CYCLES_VERR_MEM);
				}
				memset(&seg, 0, sizeof(seg));
				seg.selector = address;
				i386_load_protected_mode_segment(&seg);

				if(seg.selector & 0x04)
				{
					if((seg.selector & 0xf8) > I.ldtr.limit)
					{
						result = 0;
					}
				}
				else
				{
					if((seg.selector & 0xf8) > I.gdtr.limit)
					{
						result = 0;
					}
				}

				if(!(seg.flags & 0x10))
					result = 0;

				if(seg.flags & 0x10)
				{
					if(seg.flags & 0x08)
					{
						if(!(seg.flags & 0x02))
						{
							result = 0;
						}
						else
						{
							if(!(seg.flags & 0x04))
							{

								if(((seg.flags >> 5) & 0x03) < (address & 0x03))
									result = 0;
							}
						}
					}
				}

				SetZF(result);
			}
			else
			{
				i386_trap(6, 0, 0);
				logerror("i386: VERR: Exception - Running in real mode or virtual 8086 mode.\n");
			}
			break;

		case 5:
			if ( PROTECTED_MODE && !V8086_MODE )
			{
				result = 1;
				if( modrm >= 0xc0 ) {
					address = LOAD_RM16(modrm);
					CYCLES(CYCLES_VERW_REG);
				} else {
					ea = GetEA(modrm);
					address = READ16(ea);
					CYCLES(CYCLES_VERW_MEM);
				}
				memset(&seg, 0, sizeof(seg));
				seg.selector = address;
				i386_load_protected_mode_segment(&seg);

				if(seg.selector & 0x04)
				{
					if((seg.selector & 0xf8) > I.ldtr.limit)
					{
						result = 0;
					}
				}
				else
				{
					if((seg.selector & 0xf8) > I.gdtr.limit)
					{
						result = 0;
					}
				}

				if(!(seg.flags & 0x10))
					result = 0;

				if(seg.flags & 0x10)
				{
					if(seg.flags & 0x08)
					{
						result = 0;
					}
					else
					{
						if(!(seg.flags & 0x02))
							result = 0;
					}
				}

				if(((seg.flags >> 5) & 0x03) < (address & 0x03))
					result = 0;
				SetZF(result);
			}
			else
			{
				i386_trap(6, 0, 0);
				logerror("i386: VERW: Exception - Running in real mode or virtual 8086 mode.\n");
			}
			break;

		default:
			fatalerror("i386: group0F00_16 /%d unimplemented", (modrm >> 3) & 0x7);
			break;
	}
}

static void I386OP(group0F01_16)(void)
{
	UINT8 modrm = FETCH();
	UINT16 address;
	UINT32 ea;

	switch( (modrm >> 3) & 0x7 )
	{
		case 0:
			{
				if( modrm >= 0xc0 ) {
					address = LOAD_RM16(modrm);
					ea = i386_translate(CS, address );
				} else {
					ea = GetEA(modrm);
				}
				WRITE16(ea, I.gdtr.limit);
				WRITE32(ea + 2, I.gdtr.base & 0xffffff);
				CYCLES(CYCLES_SGDT);
				break;
			}
		case 1:
			{
				if (modrm >= 0xc0)
				{
					address = LOAD_RM16(modrm);
					ea = i386_translate(CS, address );
				}
				else
				{
					ea = GetEA(modrm);
				}
				WRITE16(ea, I.idtr.limit);
				WRITE32(ea + 2, I.idtr.base & 0xffffff);
				CYCLES(CYCLES_SIDT);
				break;
			}
		case 2:
			{
				if( modrm >= 0xc0 ) {
					address = LOAD_RM16(modrm);
					ea = i386_translate(CS, address );
				} else {
					ea = GetEA(modrm);
				}
				I.gdtr.limit = READ16(ea);
				I.gdtr.base = READ32(ea + 2) & 0xffffff;
				CYCLES(CYCLES_LGDT);
				break;
			}
		case 3:
			{
				if( modrm >= 0xc0 ) {
					address = LOAD_RM16(modrm);
					ea = i386_translate(CS, address );
				} else {
					ea = GetEA(modrm);
				}
				I.idtr.limit = READ16(ea);
				I.idtr.base = READ32(ea + 2) & 0xffffff;
				CYCLES(CYCLES_LIDT);
				break;
			}
		case 4:
			{
				if( modrm >= 0xc0 ) {
					STORE_RM16(modrm, I.cr[0]);
					CYCLES(CYCLES_SMSW_REG);
				} else {
					ea = GetEA(modrm);
					WRITE16(ea, I.cr[0]);
					CYCLES(CYCLES_SMSW_MEM);
				}
				break;
			}
		case 6:
			{

				UINT16 b;
				if( modrm >= 0xc0 ) {
					b = LOAD_RM16(modrm);
					CYCLES(CYCLES_LMSW_REG);
				} else {
					ea = GetEA(modrm);
					CYCLES(CYCLES_LMSW_MEM);
				b = READ16(ea);
				}
				if(PROTECTED_MODE)
					b |= 0x0001;
				I.cr[0] &= ~0x0000ffff;
				I.cr[0] |= b & 0x0000ffff;
				break;
			}
		default:
			fatalerror("i386: unimplemented opcode 0x0f 01 /%d at %08X", (modrm >> 3) & 0x7, I.eip - 2);
			break;
	}
}

static void I386OP(group0FBA_16)(void)
{
	UINT8 modrm = FETCH();

	switch( (modrm >> 3) & 0x7 )
	{
		case 4:
			if( modrm >= 0xc0 ) {
				UINT16 dst = LOAD_RM16(modrm);
				UINT8 bit = FETCH();

				if( dst & (1 << bit) )
					I.CF = 1;
				else
					I.CF = 0;

				CYCLES(CYCLES_BT_IMM_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT16 dst = READ16(ea);
				UINT8 bit = FETCH();

				if( dst & (1 << bit) )
					I.CF = 1;
				else
					I.CF = 0;

				CYCLES(CYCLES_BT_IMM_MEM);
			}
			break;
		case 5:
			if( modrm >= 0xc0 ) {
				UINT16 dst = LOAD_RM16(modrm);
				UINT8 bit = FETCH();

				if( dst & (1 << bit) )
					I.CF = 1;
				else
					I.CF = 0;
				dst |= (1 << bit);

				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_BTS_IMM_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT16 dst = READ16(ea);
				UINT8 bit = FETCH();

				if( dst & (1 << bit) )
					I.CF = 1;
				else
					I.CF = 0;
				dst |= (1 << bit);

				WRITE16(ea, dst);
				CYCLES(CYCLES_BTS_IMM_MEM);
			}
			break;
		case 6:
			if( modrm >= 0xc0 ) {
				UINT16 dst = LOAD_RM16(modrm);
				UINT8 bit = FETCH();

				if( dst & (1 << bit) )
					I.CF = 1;
				else
					I.CF = 0;
				dst &= ~(1 << bit);

				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_BTR_IMM_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT16 dst = READ16(ea);
				UINT8 bit = FETCH();

				if( dst & (1 << bit) )
					I.CF = 1;
				else
					I.CF = 0;
				dst &= ~(1 << bit);

				WRITE16(ea, dst);
				CYCLES(CYCLES_BTR_IMM_MEM);
			}
			break;
		case 7:
			if( modrm >= 0xc0 ) {
				UINT16 dst = LOAD_RM16(modrm);
				UINT8 bit = FETCH();

				if( dst & (1 << bit) )
					I.CF = 1;
				else
					I.CF = 0;
				dst ^= (1 << bit);

				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_BTC_IMM_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT16 dst = READ16(ea);
				UINT8 bit = FETCH();

				if( dst & (1 << bit) )
					I.CF = 1;
				else
					I.CF = 0;
				dst ^= (1 << bit);

				WRITE16(ea, dst);
				CYCLES(CYCLES_BTC_IMM_MEM);
			}
			break;
		default:
			fatalerror("i386: group0FBA_16 /%d unknown", (modrm >> 3) & 0x7);
			break;
	}
}

static void I386OP(lar_r16_rm16)(void)
{
	UINT8 modrm = FETCH();
	I386_SREG seg;
	UINT8 type;

	if(PROTECTED_MODE && !V8086_MODE)
	{
		memset(&seg,0,sizeof(seg));
		if(modrm >= 0xc0)
		{
			seg.selector = LOAD_RM16(modrm);
			CYCLES(CYCLES_LAR_REG);
		}
		else
		{
			UINT32 ea = GetEA(modrm);
			seg.selector = READ16(ea);
			CYCLES(CYCLES_LAR_MEM);
		}
		if(seg.selector == 0)
		{
			SetZF(0);
			logerror("i386: LAR: Selector %04x is invalid type.\n",seg.selector);
		}
		else
		{
			i386_load_protected_mode_segment(&seg);
			if(!(seg.flags & 0x10))
			{

				type = seg.flags & 0x000f;
				if(type == 0x00 || type == 0x08 || type == 0x0a || type == 0x0d)
				{
					SetZF(0);
				}
				else
				{

					STORE_REG16(modrm,(seg.flags << 8) & 0xff00);
					SetZF(1);
				}
			}
			else
			{

				STORE_REG16(modrm,(seg.flags << 8) & 0xff00);
				SetZF(1);
			}
		}
	}
	else
	{

		i386_trap(6,0, 0);
		logerror("i386: LAR: Exception - running in real mode or virtual 8086 mode.\n");
	}
}

static void I386OP(lsl_r16_rm16)(void)
{
	UINT8 modrm = FETCH();
	UINT32 limit;
	I386_SREG seg;

	if(PROTECTED_MODE)
	{
		memset(&seg, 0, sizeof(seg));
		if(modrm >= 0xc0)
		{
			seg.selector = LOAD_RM16(modrm);
		}
		else
		{
			UINT32 ea = GetEA(modrm);
			seg.selector = READ16(ea);
		}
		if(seg.selector == 0)
		{
			SetZF(0);
		}
		else
		{

			i386_load_protected_mode_segment(&seg);
			limit = seg.limit;
			STORE_REG16(modrm,limit & 0x0000ffff);
			SetZF(1);
		}
	}
	else
		i386_trap(6, 0, 0);
}

static void I386OP(bound_r16_m16_m16)(void)
{
	UINT8 modrm;
	INT16 val, low, high;

	modrm = FETCH();

	if (modrm >= 0xc0)
	{
		low = high = LOAD_RM16(modrm);
	}
	else
	{
		UINT32 ea = GetEA(modrm);
		low = READ16(ea + 0);
		high = READ16(ea + 2);
	}
	val = LOAD_REG16(modrm);

	if ((val < low) || (val > high))
	{
		CYCLES(CYCLES_BOUND_OUT_RANGE);
		i386_trap(5, 0, 0);
	}
	else
	{
		CYCLES(CYCLES_BOUND_IN_RANGE);
	}
}

static void I386OP(retf16)(void)
{
	if(PROTECTED_MODE && !V8086_MODE)
	{
		i386_protected_mode_retf(0,0);
	}
	else
	{
		I.eip = POP16();
		I.sreg[CS].selector = POP16();
		i386_load_segment_descriptor(CS );
		CHANGE_PC(I.eip);
	}

	CYCLES(CYCLES_RET_INTERSEG);
}

static void I386OP(retf_i16)(void)
{
	UINT16 count = FETCH16();

	if(PROTECTED_MODE && !V8086_MODE)
	{
		i386_protected_mode_retf(count,0);
	}
	else
	{
		I.eip = POP16();
		I.sreg[CS].selector = POP16();
		i386_load_segment_descriptor(CS );
		CHANGE_PC(I.eip);
		REG16(SP) += count;
	}

	CYCLES(CYCLES_RET_IMM_INTERSEG);
}

static void I386OP(xlat16)(void)
{
	UINT32 ea;
	if( I.segment_prefix ) {
		ea = i386_translate(I.segment_override, REG16(BX) + REG8(AL) );
	} else {
		ea = i386_translate(DS, REG16(BX) + REG8(AL) );
	}
	REG8(AL) = READ8(ea);
	CYCLES(CYCLES_XLAT);
}

static void I386OP(load_far_pointer16)(int s)
{
	UINT8 modrm = FETCH();

	if( modrm >= 0xc0 ) {
		fatalerror("i386: load_far_pointer16 NYI");
	} else {
		UINT32 ea = GetEA(modrm);
		STORE_REG16(modrm, READ16(ea + 0));
		I.sreg[s].selector = READ16(ea + 2);
		i386_load_segment_descriptor(s );
	}
}

static void I386OP(lds16)(void)
{
	I386OP(load_far_pointer16)(DS);
	CYCLES(CYCLES_LDS);
}

static void I386OP(lss16)(void)
{
	I386OP(load_far_pointer16)(SS);
	CYCLES(CYCLES_LSS);
}

static void I386OP(les16)(void)
{
	I386OP(load_far_pointer16)(ES);
	CYCLES(CYCLES_LES);
}

static void I386OP(lfs16)(void)
{
	I386OP(load_far_pointer16)(FS);
	CYCLES(CYCLES_LFS);
}

static void I386OP(lgs16)(void)
{
	I386OP(load_far_pointer16)(GS);
	CYCLES(CYCLES_LGS);
}

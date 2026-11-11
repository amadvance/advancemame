// license:BSD-3-Clause
// copyright-holders:Ville Linde, Barry Rodewald
static UINT32 I386OP(shift_rotate32)(UINT8 modrm, UINT32 value, UINT8 shift)
{
	UINT32 dst, src;
	dst = value;
	src = value;

	if( shift == 0 ) {
		CYCLES_RM(modrm, 3, 7);
	} else if( shift == 1 ) {

		switch( (modrm >> 3) & 0x7 )
		{
			case 0:
				I.CF = (src & 0x80000000) ? 1 : 0;
				dst = (src << 1) + I.CF;
				I.OF = ((src ^ dst) & 0x80000000) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 1:
				I.CF = (src & 0x1) ? 1 : 0;
				dst = (I.CF << 31) | (src >> 1);
				I.OF = ((src ^ dst) & 0x80000000) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 2:
				dst = (src << 1) + I.CF;
				I.CF = (src & 0x80000000) ? 1 : 0;
				I.OF = ((src ^ dst) & 0x80000000) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 3:
				dst = (I.CF << 31) | (src >> 1);
				I.CF = src & 0x1;
				I.OF = ((src ^ dst) & 0x80000000) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 4:
			case 6:
				dst = src << 1;
				I.CF = (src & 0x80000000) ? 1 : 0;
				I.OF = (((I.CF << 31) ^ dst) & 0x80000000) ? 1 : 0;
				SetSZPF32(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 5:
				dst = src >> 1;
				I.CF = src & 0x1;
				I.OF = (src & 0x80000000) ? 1 : 0;
				SetSZPF32(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 7:
				dst = (INT32)(src) >> 1;
				I.CF = src & 0x1;
				I.OF = 0;
				SetSZPF32(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
		}

	} else {

		switch( (modrm >> 3) & 0x7 )
		{
			case 0:
				dst = ((src & ((UINT32)0xffffffff >> shift)) << shift) |
					  ((src & ((UINT32)0xffffffff << (32-shift))) >> (32-shift));
				I.CF = (src >> (32-shift)) & 0x1;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 1:
				dst = ((src & ((UINT32)0xffffffff << shift)) >> shift) |
					  ((src & ((UINT32)0xffffffff >> (32-shift))) << (32-shift));
				I.CF = (src >> (shift-1)) & 0x1;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 2:
				dst = ((src & ((UINT32)0xffffffff >> shift)) << shift) |
					  ((src & ((UINT32)0xffffffff << (33-shift))) >> (33-shift)) |
					  (I.CF << (shift-1));
				I.CF = (src >> (32-shift)) & 0x1;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 3:
				dst = ((src & ((UINT32)0xffffffff << shift)) >> shift) |
					  ((src & ((UINT32)0xffffffff >> (32-shift))) << (33-shift)) |
					  (I.CF << (32-shift));
				I.CF = (src >> (shift-1)) & 0x1;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 4:
			case 6:
				dst = src << shift;
				I.CF = (src & (1 << (32-shift))) ? 1 : 0;
				SetSZPF32(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 5:
				dst = src >> shift;
				I.CF = (src & (1 << (shift-1))) ? 1 : 0;
				SetSZPF32(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 7:
				dst = (INT32)src >> shift;
				I.CF = (src & (1 << (shift-1))) ? 1 : 0;
				SetSZPF32(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
		}

	}
	return dst;
}


static void I386OP(adc_rm32_r32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		dst = ADC32(dst, src, I.CF);
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = READ32(ea);
		dst = ADC32(dst, src, I.CF);
		WRITE32(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(adc_r32_rm32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM32(modrm);
		dst = LOAD_REG32(modrm);
		dst = ADC32(dst, src, I.CF);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ32(ea);
		dst = LOAD_REG32(modrm);
		dst = ADC32(dst, src, I.CF);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(adc_eax_i32)(void)
{
	UINT32 src, dst;
	src = FETCH32();
	dst = REG32(EAX);
	dst = ADC32(dst, src, I.CF);
	REG32(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(add_rm32_r32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		dst = ADD32(dst, src);
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = READ32(ea);
		dst = ADD32(dst, src);
		WRITE32(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(add_r32_rm32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM32(modrm);
		dst = LOAD_REG32(modrm);
		dst = ADD32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ32(ea);
		dst = LOAD_REG32(modrm);
		dst = ADD32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(add_eax_i32)(void)
{
	UINT32 src, dst;
	src = FETCH32();
	dst = REG32(EAX);
	dst = ADD32(dst, src);
	REG32(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(and_rm32_r32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		dst = AND32(dst, src);
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = READ32(ea);
		dst = AND32(dst, src);
		WRITE32(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(and_r32_rm32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM32(modrm);
		dst = LOAD_REG32(modrm);
		dst = AND32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ32(ea);
		dst = LOAD_REG32(modrm);
		dst = AND32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(and_eax_i32)(void)
{
	UINT32 src, dst;
	src = FETCH32();
	dst = REG32(EAX);
	dst = AND32(dst, src);
	REG32(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(bsf_r32_rm32)(void)
{
	UINT32 src, dst, temp;
	UINT8 modrm = FETCH();

	if( modrm >= 0xc0 ) {
		src = LOAD_RM32(modrm);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ32(ea);
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
	STORE_REG32(modrm, dst);
}

static void I386OP(bsr_r32_rm32)(void)
{
	UINT32 src, dst, temp;
	UINT8 modrm = FETCH();

	if( modrm >= 0xc0 ) {
		src = LOAD_RM32(modrm);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ32(ea);
	}

	dst = 0;

	if( src == 0 ) {
		I.ZF = 1;
	} else {
		I.ZF = 0;
		dst = temp = 31;
		while( (src & (1 << temp)) == 0 ) {
			temp--;
			dst = temp;
			CYCLES(CYCLES_BSR);
		}
	}
	CYCLES(CYCLES_BSR_BASE);
	STORE_REG32(modrm, dst);
}

static void I386OP(bt_rm32_r32)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT32 dst = LOAD_RM32(modrm);
		UINT32 bit = LOAD_REG32(modrm);

		if( dst & (1 << bit) )
			I.CF = 1;
		else
			I.CF = 0;

		CYCLES(CYCLES_BT_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT32 dst = READ32(ea);
		UINT32 bit = LOAD_REG32(modrm);

		if( dst & (1 << bit) )
			I.CF = 1;
		else
			I.CF = 0;

		CYCLES(CYCLES_BT_REG_MEM);
	}
}

static void I386OP(btc_rm32_r32)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT32 dst = LOAD_RM32(modrm);
		UINT32 bit = LOAD_REG32(modrm);

		if( dst & (1 << bit) )
			I.CF = 1;
		else
			I.CF = 0;
		dst ^= (1 << bit);

		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_BTC_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT32 dst = READ32(ea);
		UINT32 bit = LOAD_REG32(modrm);

		if( dst & (1 << bit) )
			I.CF = 1;
		else
			I.CF = 0;
		dst ^= (1 << bit);

		WRITE32(ea, dst);
		CYCLES(CYCLES_BTC_REG_MEM);
	}
}

static void I386OP(btr_rm32_r32)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT32 dst = LOAD_RM32(modrm);
		UINT32 bit = LOAD_REG32(modrm);

		if( dst & (1 << bit) )
			I.CF = 1;
		else
			I.CF = 0;
		dst &= ~(1 << bit);

		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_BTR_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT32 dst = READ32(ea);
		UINT32 bit = LOAD_REG32(modrm);

		if( dst & (1 << bit) )
			I.CF = 1;
		else
			I.CF = 0;
		dst &= ~(1 << bit);

		WRITE32(ea, dst);
		CYCLES(CYCLES_BTR_REG_MEM);
	}
}

static void I386OP(bts_rm32_r32)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT32 dst = LOAD_RM32(modrm);
		UINT32 bit = LOAD_REG32(modrm);

		if( dst & (1 << bit) )
			I.CF = 1;
		else
			I.CF = 0;
		dst |= (1 << bit);

		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_BTS_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT32 dst = READ32(ea);
		UINT32 bit = LOAD_REG32(modrm);

		if( dst & (1 << bit) )
			I.CF = 1;
		else
			I.CF = 0;
		dst |= (1 << bit);

		WRITE32(ea, dst);
		CYCLES(CYCLES_BTS_REG_MEM);
	}
}

static void I386OP(call_abs32)(void)
{
	UINT32 offset = FETCH32();
	UINT16 ptr = FETCH16();

	if(PROTECTED_MODE && !V8086_MODE)
	{
		i386_protected_mode_call(ptr,offset,0,1);
	}
	else
	{
		PUSH32(I.sreg[CS].selector );
		PUSH32(I.eip );
		I.sreg[CS].selector = ptr;
		I.performed_intersegment_jump = 1;
		I.eip = offset;
		i386_load_segment_descriptor(CS);
	}
	CYCLES(CYCLES_CALL_INTERSEG);
	CHANGE_PC(I.eip);
}

static void I386OP(call_rel32)(void)
{
	INT32 disp = FETCH32();

	PUSH32(I.eip );
	I.eip += disp;
	CHANGE_PC(I.eip);
	CYCLES(CYCLES_CALL);
}

static void I386OP(cdq)(void)
{
	if( REG32(EAX) & 0x80000000 ) {
		REG32(EDX) = 0xffffffff;
	} else {
		REG32(EDX) = 0x00000000;
	}
	CYCLES(CYCLES_CWD);
}

static void I386OP(cmp_rm32_r32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		SUB32(dst, src);
		CYCLES(CYCLES_CMP_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = READ32(ea);
		SUB32(dst, src);
		CYCLES(CYCLES_CMP_REG_MEM);
	}
}

static void I386OP(cmp_r32_rm32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM32(modrm);
		dst = LOAD_REG32(modrm);
		SUB32(dst, src);
		CYCLES(CYCLES_CMP_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ32(ea);
		dst = LOAD_REG32(modrm);
		SUB32(dst, src);
		CYCLES(CYCLES_CMP_MEM_REG);
	}
}

static void I386OP(cmp_eax_i32)(void)
{
	UINT32 src, dst;
	src = FETCH32();
	dst = REG32(EAX);
	SUB32(dst, src);
	CYCLES(CYCLES_CMP_IMM_ACC);
}

static void I386OP(cmpsd)(void)
{
	UINT32 eas, ead, src, dst;
	if( I.segment_prefix ) {
		eas = i386_translate(I.segment_override, I.address_size ? REG32(ESI) : REG16(SI) );
	} else {
		eas = i386_translate(DS, I.address_size ? REG32(ESI) : REG16(SI) );
	}
	ead = i386_translate(ES, I.address_size ? REG32(EDI) : REG16(DI) );
	src = READ32(eas);
	dst = READ32(ead);
	SUB32(dst, src);
	BUMP_SI(4);
	BUMP_DI(4);
	CYCLES(CYCLES_CMPS);
}

static void I386OP(cwde)(void)
{
	REG32(EAX) = (INT32)((INT16)REG16(AX));
	CYCLES(CYCLES_CBW);
}

static void I386OP(dec_eax)(void)
{
	REG32(EAX) = DEC32(REG32(EAX) );
	CYCLES(CYCLES_DEC_REG);
}

static void I386OP(dec_ecx)(void)
{
	REG32(ECX) = DEC32(REG32(ECX) );
	CYCLES(CYCLES_DEC_REG);
}

static void I386OP(dec_edx)(void)
{
	REG32(EDX) = DEC32(REG32(EDX) );
	CYCLES(CYCLES_DEC_REG);
}

static void I386OP(dec_ebx)(void)
{
	REG32(EBX) = DEC32(REG32(EBX) );
	CYCLES(CYCLES_DEC_REG);
}

static void I386OP(dec_esp)(void)
{
	REG32(ESP) = DEC32(REG32(ESP) );
	CYCLES(CYCLES_DEC_REG);
}

static void I386OP(dec_ebp)(void)
{
	REG32(EBP) = DEC32(REG32(EBP) );
	CYCLES(CYCLES_DEC_REG);
}

static void I386OP(dec_esi)(void)
{
	REG32(ESI) = DEC32(REG32(ESI) );
	CYCLES(CYCLES_DEC_REG);
}

static void I386OP(dec_edi)(void)
{
	REG32(EDI) = DEC32(REG32(EDI) );
	CYCLES(CYCLES_DEC_REG);
}

static void I386OP(imul_r32_rm32)(void)
{
	UINT8 modrm = FETCH();
	INT64 result;
	INT64 src, dst;
	if( modrm >= 0xc0 ) {
		src = (INT64)(INT32)LOAD_RM32(modrm);
		CYCLES(CYCLES_IMUL32_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = (INT64)(INT32)READ32(ea);
		CYCLES(CYCLES_IMUL32_REG_REG);
	}

	dst = (INT64)(INT32)LOAD_REG32(modrm);
	result = src * dst;

	STORE_REG32(modrm, (UINT32)result);

	I.CF = I.OF = !(result == (INT64)(INT32)result);
}

static void I386OP(imul_r32_rm32_i32)(void)
{
	UINT8 modrm = FETCH();
	INT64 result;
	INT64 src, dst;
	if( modrm >= 0xc0 ) {
		dst = (INT64)(INT32)LOAD_RM32(modrm);
		CYCLES(CYCLES_IMUL32_REG_IMM_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		dst = (INT64)(INT32)READ32(ea);
		CYCLES(CYCLES_IMUL32_MEM_IMM_REG);
	}

	src = (INT64)(INT32)FETCH32();
	result = src * dst;

	STORE_REG32(modrm, (UINT32)result);

	I.CF = I.OF = !(result == (INT64)(INT32)result);
}

static void I386OP(imul_r32_rm32_i8)(void)
{
	UINT8 modrm = FETCH();
	INT64 result;
	INT64 src, dst;
	if( modrm >= 0xc0 ) {
		dst = (INT64)(INT32)LOAD_RM32(modrm);
		CYCLES(CYCLES_IMUL32_REG_IMM_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		dst = (INT64)(INT32)READ32(ea);
		CYCLES(CYCLES_IMUL32_MEM_IMM_REG);
	}

	src = (INT64)(INT8)FETCH();
	result = src * dst;

	STORE_REG32(modrm, (UINT32)result);

	I.CF = I.OF = !(result == (INT64)(INT32)result);
}

static void I386OP(in_eax_i8)(void)
{
	UINT16 port = FETCH();
	UINT32 data = READPORT32(port);
	REG32(EAX) = data;
	CYCLES(CYCLES_IN_VAR);
}

static void I386OP(in_eax_dx)(void)
{
	UINT16 port = REG16(DX);
	UINT32 data = READPORT32(port);
	REG32(EAX) = data;
	CYCLES(CYCLES_IN);
}

static void I386OP(inc_eax)(void)
{
	REG32(EAX) = INC32(REG32(EAX) );
	CYCLES(CYCLES_INC_REG);
}

static void I386OP(inc_ecx)(void)
{
	REG32(ECX) = INC32(REG32(ECX) );
	CYCLES(CYCLES_INC_REG);
}

static void I386OP(inc_edx)(void)
{
	REG32(EDX) = INC32(REG32(EDX) );
	CYCLES(CYCLES_INC_REG);
}

static void I386OP(inc_ebx)(void)
{
	REG32(EBX) = INC32(REG32(EBX) );
	CYCLES(CYCLES_INC_REG);
}

static void I386OP(inc_esp)(void)
{
	REG32(ESP) = INC32(REG32(ESP) );
	CYCLES(CYCLES_INC_REG);
}

static void I386OP(inc_ebp)(void)
{
	REG32(EBP) = INC32(REG32(EBP) );
	CYCLES(CYCLES_INC_REG);
}

static void I386OP(inc_esi)(void)
{
	REG32(ESI) = INC32(REG32(ESI) );
	CYCLES(CYCLES_INC_REG);
}

static void I386OP(inc_edi)(void)
{
	REG32(EDI) = INC32(REG32(EDI) );
	CYCLES(CYCLES_INC_REG);
}

static void I386OP(iret32)(void)
{
	if( PROTECTED_MODE )
	{
		i386_protected_mode_iret(1);
	}
	else
	{

		I.eip = POP32();
		I.sreg[CS].selector = POP32() & 0xffff;
		set_flags(POP32() );
		i386_load_segment_descriptor(CS);
		CHANGE_PC(I.eip);
	}
	CYCLES(CYCLES_IRET);
}

static void I386OP(ja_rel32)(void)
{
	INT32 disp = FETCH32();
	if( I.CF == 0 && I.ZF == 0 ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jbe_rel32)(void)
{
	INT32 disp = FETCH32();
	if( I.CF != 0 || I.ZF != 0 ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jc_rel32)(void)
{
	INT32 disp = FETCH32();
	if( I.CF != 0 ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jg_rel32)(void)
{
	INT32 disp = FETCH32();
	if( I.ZF == 0 && (I.SF == I.OF) ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jge_rel32)(void)
{
	INT32 disp = FETCH32();
	if( (I.SF == I.OF) ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jl_rel32)(void)
{
	INT32 disp = FETCH32();
	if( (I.SF != I.OF) ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jle_rel32)(void)
{
	INT32 disp = FETCH32();
	if( I.ZF != 0 || (I.SF != I.OF) ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jnc_rel32)(void)
{
	INT32 disp = FETCH32();
	if( I.CF == 0 ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jno_rel32)(void)
{
	INT32 disp = FETCH32();
	if( I.OF == 0 ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jnp_rel32)(void)
{
	INT32 disp = FETCH32();
	if( I.PF == 0 ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jns_rel32)(void)
{
	INT32 disp = FETCH32();
	if( I.SF == 0 ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jnz_rel32)(void)
{
	INT32 disp = FETCH32();
	if( I.ZF == 0 ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jo_rel32)(void)
{
	INT32 disp = FETCH32();
	if( I.OF != 0 ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jp_rel32)(void)
{
	INT32 disp = FETCH32();
	if( I.PF != 0 ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(js_rel32)(void)
{
	INT32 disp = FETCH32();
	if( I.SF != 0 ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jz_rel32)(void)
{
	INT32 disp = FETCH32();
	if( I.ZF != 0 ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCC_FULL_DISP);
	} else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

static void I386OP(jcxz32)(void)
{
	INT8 disp = FETCH();
	if( REG32(ECX) == 0 ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
		CYCLES(CYCLES_JCXZ);
	} else {
		CYCLES(CYCLES_JCXZ_NOBRANCH);
	}
}

static void I386OP(jmp_rel32)(void)
{
	UINT32 disp = FETCH32();

	I.eip += disp;
	CHANGE_PC(I.eip);
	CYCLES(CYCLES_JMP);
}

static void I386OP(jmp_abs32)(void)
{
	UINT32 address = FETCH32();
	UINT16 segment = FETCH16();

	if( PROTECTED_MODE && !V8086_MODE)
	{
		i386_protected_mode_jump(segment,address,0,1);
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

static void I386OP(lea32)(void)
{
	UINT8 modrm = FETCH();
	UINT32 ea = GetNonTranslatedEA(modrm);
	if (!I.address_size)
	{
		ea &= 0xffff;
	}
	STORE_REG32(modrm, ea);
	CYCLES(CYCLES_LEA);
}

static void I386OP(enter32)(void)
{
	UINT16 framesize = FETCH16();
	UINT8 level = FETCH() % 32;
	UINT8 x;
	UINT32 frameptr;
	PUSH32(REG32(EBP));
	if(!STACK_32BIT)
		frameptr = REG16(SP);
	else
		frameptr = REG32(ESP);

	if(level > 0)
	{
		for(x=1;x<level-1;x++)
		{
			REG32(EBP) -= 4;
			PUSH32(READ32(REG32(EBP)));
		}
		PUSH32(frameptr);
	}
	REG32(EBP) = frameptr;
	if(!STACK_32BIT)
		REG16(SP) -= framesize;
	else
		REG32(ESP) -= framesize;
	CYCLES(CYCLES_ENTER);
}

static void I386OP(leave32)(void)
{
	if(!STACK_32BIT)
		REG16(SP) = REG16(BP);
	else
		REG32(ESP) = REG32(EBP);
	REG32(EBP) = POP32();
	CYCLES(CYCLES_LEAVE);
}

static void I386OP(lodsd)(void)
{
	UINT32 eas;
	if( I.segment_prefix ) {
		eas = i386_translate(I.segment_override, I.address_size ? REG32(ESI) : REG16(SI) );
	} else {
		eas = i386_translate(DS, I.address_size ? REG32(ESI) : REG16(SI) );
	}
	REG32(EAX) = READ32(eas);
	BUMP_SI(4);
	CYCLES(CYCLES_LODS);
}

static void I386OP(loop32)(void)
{
	INT8 disp = FETCH();
	INT32 reg = (I.address_size)?--REG32(ECX):--REG16(CX);
	if( reg != 0 ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
	}
	CYCLES(CYCLES_LOOP);
}

static void I386OP(loopne32)(void)
{
	INT8 disp = FETCH();
	INT32 reg = (I.address_size)?--REG32(ECX):--REG16(CX);
	if( reg != 0 && I.ZF == 0 ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
	}
	CYCLES(CYCLES_LOOPNZ);
}

static void I386OP(loopz32)(void)
{
	INT8 disp = FETCH();
	INT32 reg = (I.address_size)?--REG32(ECX):--REG16(CX);
	if( reg != 0 && I.ZF != 0 ) {
		I.eip += disp;
		CHANGE_PC(I.eip);
	}
	CYCLES(CYCLES_LOOPZ);
}

static void I386OP(mov_rm32_r32)(void)
{
	UINT32 src;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG32(modrm);
		STORE_RM32(modrm, src);
		CYCLES(CYCLES_MOV_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		WRITE32(ea, src);
		CYCLES(CYCLES_MOV_REG_MEM);
	}
}

static void I386OP(mov_r32_rm32)(void)
{
	UINT32 src;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM32(modrm);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOV_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ32(ea);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOV_MEM_REG);
	}
}

static void I386OP(mov_rm32_i32)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT32 value = FETCH32();
		STORE_RM32(modrm, value);
		CYCLES(CYCLES_MOV_IMM_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT32 value = FETCH32();
		WRITE32(ea, value);
		CYCLES(CYCLES_MOV_IMM_MEM);
	}
}

static void I386OP(mov_eax_m32)(void)
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
	REG32(EAX) = READ32(ea);
	CYCLES(CYCLES_MOV_MEM_ACC);
}

static void I386OP(mov_m32_eax)(void)
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
	WRITE32(ea, REG32(EAX) );
	CYCLES(CYCLES_MOV_ACC_MEM);
}

static void I386OP(mov_eax_i32)(void)
{
	REG32(EAX) = FETCH32();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_ecx_i32)(void)
{
	REG32(ECX) = FETCH32();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_edx_i32)(void)
{
	REG32(EDX) = FETCH32();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_ebx_i32)(void)
{
	REG32(EBX) = FETCH32();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_esp_i32)(void)
{
	REG32(ESP) = FETCH32();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_ebp_i32)(void)
{
	REG32(EBP) = FETCH32();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_esi_i32)(void)
{
	REG32(ESI) = FETCH32();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_edi_i32)(void)
{
	REG32(EDI) = FETCH32();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(movsd)(void)
{
	UINT32 eas, ead, v;
	if( I.segment_prefix ) {
		eas = i386_translate(I.segment_override, I.address_size ? REG32(ESI) : REG16(SI) );
	} else {
		eas = i386_translate(DS, I.address_size ? REG32(ESI) : REG16(SI) );
	}
	ead = i386_translate(ES, I.address_size ? REG32(EDI) : REG16(DI) );
	v = READ32(eas);
	WRITE32(ead, v);
	BUMP_SI(4);
	BUMP_DI(4);
	CYCLES(CYCLES_MOVS);
}

static void I386OP(movsx_r32_rm8)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		INT32 src = (INT8)LOAD_RM8(modrm);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOVSX_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		INT32 src = (INT8)READ8(ea);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOVSX_MEM_REG);
	}
}

static void I386OP(movsx_r32_rm16)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		INT32 src = (INT16)LOAD_RM16(modrm);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOVSX_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		INT32 src = (INT16)READ16(ea);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOVSX_MEM_REG);
	}
}

static void I386OP(movzx_r32_rm8)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT32 src = (UINT8)LOAD_RM8(modrm);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOVZX_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT32 src = (UINT8)READ8(ea);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOVZX_MEM_REG);
	}
}

static void I386OP(movzx_r32_rm16)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT32 src = (UINT16)LOAD_RM16(modrm);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOVZX_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT32 src = (UINT16)READ16(ea);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOVZX_MEM_REG);
	}
}

static void I386OP(or_rm32_r32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		dst = OR32(dst, src);
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = READ32(ea);
		dst = OR32(dst, src);
		WRITE32(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(or_r32_rm32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM32(modrm);
		dst = LOAD_REG32(modrm);
		dst = OR32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ32(ea);
		dst = LOAD_REG32(modrm);
		dst = OR32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(or_eax_i32)(void)
{
	UINT32 src, dst;
	src = FETCH32();
	dst = REG32(EAX);
	dst = OR32(dst, src);
	REG32(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(out_eax_i8)(void)
{
	UINT16 port = FETCH();
	UINT32 data = REG32(EAX);
	WRITEPORT32(port, data);
	CYCLES(CYCLES_OUT_VAR);
}

static void I386OP(out_eax_dx)(void)
{
	UINT16 port = REG16(DX);
	UINT32 data = REG32(EAX);
	WRITEPORT32(port, data);
	CYCLES(CYCLES_OUT);
}

static void I386OP(pop_eax)(void)
{
	REG32(EAX) = POP32();
	CYCLES(CYCLES_POP_REG_SHORT);
}

static void I386OP(pop_ecx)(void)
{
	REG32(ECX) = POP32();
	CYCLES(CYCLES_POP_REG_SHORT);
}

static void I386OP(pop_edx)(void)
{
	REG32(EDX) = POP32();
	CYCLES(CYCLES_POP_REG_SHORT);
}

static void I386OP(pop_ebx)(void)
{
	REG32(EBX) = POP32();
	CYCLES(CYCLES_POP_REG_SHORT);
}

static void I386OP(pop_esp)(void)
{
	REG32(ESP) = POP32();
	CYCLES(CYCLES_POP_REG_SHORT);
}

static void I386OP(pop_ebp)(void)
{
	REG32(EBP) = POP32();
	CYCLES(CYCLES_POP_REG_SHORT);
}

static void I386OP(pop_esi)(void)
{
	REG32(ESI) = POP32();
	CYCLES(CYCLES_POP_REG_SHORT);
}

static void I386OP(pop_edi)(void)
{
	REG32(EDI) = POP32();
	CYCLES(CYCLES_POP_REG_SHORT);
}

static void I386OP(pop_ds32)(void)
{
	I.sreg[DS].selector = POP32();
	if( PROTECTED_MODE ) {
		i386_load_segment_descriptor(DS);
	} else {
		i386_load_segment_descriptor(DS);
	}
	CYCLES(CYCLES_POP_SREG);
}

static void I386OP(pop_es32)(void)
{
	I.sreg[ES].selector = POP32();
	if( PROTECTED_MODE ) {
		i386_load_segment_descriptor(ES);
	} else {
		i386_load_segment_descriptor(ES);
	}
	CYCLES(CYCLES_POP_SREG);
}

static void I386OP(pop_fs32)(void)
{
	I.sreg[FS].selector = POP32();
	if( PROTECTED_MODE ) {
		i386_load_segment_descriptor(FS);
	} else {
		i386_load_segment_descriptor(FS);
	}
	CYCLES(CYCLES_POP_SREG);
}

static void I386OP(pop_gs32)(void)
{
	I.sreg[GS].selector = POP32();
	if( PROTECTED_MODE ) {
		i386_load_segment_descriptor(GS);
	} else {
		i386_load_segment_descriptor(GS);
	}
	CYCLES(CYCLES_POP_SREG);
}

static void I386OP(pop_ss32)(void)
{
	if(I.IF != 0)
	{
		I.IF = 0;
		I.delayed_interrupt_enable = 1;
	}

	I.sreg[SS].selector = POP32();
	if( PROTECTED_MODE ) {
		i386_load_segment_descriptor(SS);
	} else {
		i386_load_segment_descriptor(SS);
	}
	CYCLES(CYCLES_POP_SREG);
}

static void I386OP(pop_rm32)(void)
{
	UINT8 modrm = FETCH();
	UINT32 value = POP32();

	if( modrm >= 0xc0 ) {
		STORE_RM32(modrm, value);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE32(ea, value);
	}
	CYCLES(CYCLES_POP_RM);
}

static void I386OP(popad)(void)
{
	REG32(EDI) = POP32();
	REG32(ESI) = POP32();
	REG32(EBP) = POP32();
	REG32(ESP) += 4;
	REG32(EBX) = POP32();
	REG32(EDX) = POP32();
	REG32(ECX) = POP32();
	REG32(EAX) = POP32();
	CYCLES(CYCLES_POPA);
}

static void I386OP(popfd)(void)
{
	UINT32 value = POP32();
	set_flags(value);
	CYCLES(CYCLES_POPF);
}

static void I386OP(push_eax)(void)
{
	PUSH32(REG32(EAX) );
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

static void I386OP(push_ecx)(void)
{
	PUSH32(REG32(ECX) );
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

static void I386OP(push_edx)(void)
{
	PUSH32(REG32(EDX) );
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

static void I386OP(push_ebx)(void)
{
	PUSH32(REG32(EBX) );
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

static void I386OP(push_esp)(void)
{
	PUSH32(REG32(ESP) );
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

static void I386OP(push_ebp)(void)
{
	PUSH32(REG32(EBP) );
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

static void I386OP(push_esi)(void)
{
	PUSH32(REG32(ESI) );
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

static void I386OP(push_edi)(void)
{
	PUSH32(REG32(EDI) );
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

static void I386OP(push_cs32)(void)
{
	PUSH32(I.sreg[CS].selector );
	CYCLES(CYCLES_PUSH_SREG);
}

static void I386OP(push_ds32)(void)
{
	PUSH32(I.sreg[DS].selector );
	CYCLES(CYCLES_PUSH_SREG);
}

static void I386OP(push_es32)(void)
{
	PUSH32(I.sreg[ES].selector );
	CYCLES(CYCLES_PUSH_SREG);
}

static void I386OP(push_fs32)(void)
{
	PUSH32(I.sreg[FS].selector );
	CYCLES(CYCLES_PUSH_SREG);
}

static void I386OP(push_gs32)(void)
{
	PUSH32(I.sreg[GS].selector );
	CYCLES(CYCLES_PUSH_SREG);
}

static void I386OP(push_ss32)(void)
{
	PUSH32(I.sreg[SS].selector );
	CYCLES(CYCLES_PUSH_SREG);
}

static void I386OP(push_i32)(void)
{
	UINT32 value = FETCH32();
	PUSH32(value);
	CYCLES(CYCLES_PUSH_IMM);
}

static void I386OP(pushad)(void)
{
	UINT32 temp = REG32(ESP);
	PUSH32(REG32(EAX) );
	PUSH32(REG32(ECX) );
	PUSH32(REG32(EDX) );
	PUSH32(REG32(EBX) );
	PUSH32(temp );
	PUSH32(REG32(EBP) );
	PUSH32(REG32(ESI) );
	PUSH32(REG32(EDI) );
	CYCLES(CYCLES_PUSHA);
}

static void I386OP(pushfd)(void)
{
	PUSH32(get_flags() & 0x00fcffff );
	CYCLES(CYCLES_PUSHF);
}

static void I386OP(ret_near32_i16)(void)
{
	INT16 disp = FETCH16();
	I.eip = POP32();
	REG32(ESP) += disp;
	CHANGE_PC(I.eip);
	CYCLES(CYCLES_RET_IMM);
}

static void I386OP(ret_near32)(void)
{
	I.eip = POP32();
	CHANGE_PC(I.eip);
	CYCLES(CYCLES_RET);
}

static void I386OP(sbb_rm32_r32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		dst = SBB32(dst, src, I.CF);
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = READ32(ea);
		dst = SBB32(dst, src, I.CF);
		WRITE32(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(sbb_r32_rm32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM32(modrm);
		dst = LOAD_REG32(modrm);
		dst = SBB32(dst, src, I.CF);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ32(ea);
		dst = LOAD_REG32(modrm);
		dst = SBB32(dst, src, I.CF);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(sbb_eax_i32)(void)
{
	UINT32 src, dst;
	src = FETCH32();
	dst = REG32(EAX);
	dst = SBB32(dst, src, I.CF);
	REG32(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(scasd)(void)
{
	UINT32 eas, src, dst;
	eas = i386_translate(ES, I.address_size ? REG32(EDI) : REG16(DI) );
	src = READ32(eas);
	dst = REG32(EAX);
	SUB32(dst, src);
	BUMP_DI(4);
	CYCLES(CYCLES_SCAS);
}

static void I386OP(shld32_i8)(void)
{

	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT32 dst = LOAD_RM32(modrm);
		UINT32 upper = LOAD_REG32(modrm);
		UINT8 shift = FETCH();
		if( shift > 31 || shift == 0 ) {

		} else {
			I.CF = (dst & (1 << (32-shift))) ? 1 : 0;
			dst = (dst << shift) | (upper >> (32-shift));
			SetSZPF32(dst);
		}
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_SHLD_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT32 dst = READ32(ea);
		UINT32 upper = LOAD_REG32(modrm);
		UINT8 shift = FETCH();
		if( shift > 31 || shift == 0 ) {

		} else {
			I.CF = (dst & (1 << (32-shift))) ? 1 : 0;
			dst = (dst << shift) | (upper >> (32-shift));
			SetSZPF32(dst);
		}
		WRITE32(ea, dst);
		CYCLES(CYCLES_SHLD_MEM);
	}
}

static void I386OP(shld32_cl)(void)
{

	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT32 dst = LOAD_RM32(modrm);
		UINT32 upper = LOAD_REG32(modrm);
		UINT8 shift = REG8(CL);
		if( shift > 31 || shift == 0 ) {

		} else {
			I.CF = (dst & (1 << (32-shift))) ? 1 : 0;
			dst = (dst << shift) | (upper >> (32-shift));
			SetSZPF32(dst);
		}
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_SHLD_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT32 dst = READ32(ea);
		UINT32 upper = LOAD_REG32(modrm);
		UINT8 shift = REG8(CL);
		if( shift > 31 || shift == 0 ) {

		} else {
			I.CF = (dst & (1 << (32-shift))) ? 1 : 0;
			dst = (dst << shift) | (upper >> (32-shift));
			SetSZPF32(dst);
		}
		WRITE32(ea, dst);
		CYCLES(CYCLES_SHLD_MEM);
	}
}

static void I386OP(shrd32_i8)(void)
{

	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT32 dst = LOAD_RM32(modrm);
		UINT32 upper = LOAD_REG32(modrm);
		UINT8 shift = FETCH();
		if( shift > 31 || shift == 0 ) {

		} else {
			I.CF = (dst & (1 << (shift-1))) ? 1 : 0;
			dst = (dst >> shift) | (upper << (32-shift));
			SetSZPF32(dst);
		}
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_SHRD_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT32 dst = READ32(ea);
		UINT32 upper = LOAD_REG32(modrm);
		UINT8 shift = FETCH();
		if( shift > 31 || shift == 0 ) {

		} else {
			I.CF = (dst & (1 << (shift-1))) ? 1 : 0;
			dst = (dst >> shift) | (upper << (32-shift));
			SetSZPF32(dst);
		}
		WRITE32(ea, dst);
		CYCLES(CYCLES_SHRD_MEM);
	}
}

static void I386OP(shrd32_cl)(void)
{

	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT32 dst = LOAD_RM32(modrm);
		UINT32 upper = LOAD_REG32(modrm);
		UINT8 shift = REG8(CL);
		if( shift > 31 || shift == 0 ) {

		} else {
			I.CF = (dst & (1 << (shift-1))) ? 1 : 0;
			dst = (dst >> shift) | (upper << (32-shift));
			SetSZPF32(dst);
		}
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_SHRD_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT32 dst = READ32(ea);
		UINT32 upper = LOAD_REG32(modrm);
		UINT8 shift = REG8(CL);
		if( shift > 31 || shift == 0 ) {

		} else {
			I.CF = (dst & (1 << (shift-1))) ? 1 : 0;
			dst = (dst >> shift) | (upper << (32-shift));
			SetSZPF32(dst);
		}
		WRITE32(ea, dst);
		CYCLES(CYCLES_SHRD_MEM);
	}
}

static void I386OP(stosd)(void)
{
	UINT32 eas = i386_translate(ES, I.address_size ? REG32(EDI) : REG16(DI) );
	WRITE32(eas, REG32(EAX));
	BUMP_DI(4);
	CYCLES(CYCLES_STOS);
}

static void I386OP(sub_rm32_r32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		dst = SUB32(dst, src);
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = READ32(ea);
		dst = SUB32(dst, src);
		WRITE32(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(sub_r32_rm32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM32(modrm);
		dst = LOAD_REG32(modrm);
		dst = SUB32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ32(ea);
		dst = LOAD_REG32(modrm);
		dst = SUB32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(sub_eax_i32)(void)
{
	UINT32 src, dst;
	src = FETCH32();
	dst = REG32(EAX);
	dst = SUB32(dst, src);
	REG32(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(test_eax_i32)(void)
{
	UINT32 src = FETCH32();
	UINT32 dst = REG32(EAX);
	dst = src & dst;
	SetSZPF32(dst);
	I.CF = 0;
	I.OF = 0;
	CYCLES(CYCLES_TEST_IMM_ACC);
}

static void I386OP(test_rm32_r32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		dst = src & dst;
		SetSZPF32(dst);
		I.CF = 0;
		I.OF = 0;
		CYCLES(CYCLES_TEST_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = READ32(ea);
		dst = src & dst;
		SetSZPF32(dst);
		I.CF = 0;
		I.OF = 0;
		CYCLES(CYCLES_TEST_REG_MEM);
	}
}

static void I386OP(xchg_eax_ecx)(void)
{
	UINT32 temp;
	temp = REG32(EAX);
	REG32(EAX) = REG32(ECX);
	REG32(ECX) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

static void I386OP(xchg_eax_edx)(void)
{
	UINT32 temp;
	temp = REG32(EAX);
	REG32(EAX) = REG32(EDX);
	REG32(EDX) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

static void I386OP(xchg_eax_ebx)(void)
{
	UINT32 temp;
	temp = REG32(EAX);
	REG32(EAX) = REG32(EBX);
	REG32(EBX) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

static void I386OP(xchg_eax_esp)(void)
{
	UINT32 temp;
	temp = REG32(EAX);
	REG32(EAX) = REG32(ESP);
	REG32(ESP) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

static void I386OP(xchg_eax_ebp)(void)
{
	UINT32 temp;
	temp = REG32(EAX);
	REG32(EAX) = REG32(EBP);
	REG32(EBP) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

static void I386OP(xchg_eax_esi)(void)
{
	UINT32 temp;
	temp = REG32(EAX);
	REG32(EAX) = REG32(ESI);
	REG32(ESI) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

static void I386OP(xchg_eax_edi)(void)
{
	UINT32 temp;
	temp = REG32(EAX);
	REG32(EAX) = REG32(EDI);
	REG32(EDI) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

static void I386OP(xchg_r32_rm32)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT32 src = LOAD_RM32(modrm);
		UINT32 dst = LOAD_REG32(modrm);
		STORE_REG32(modrm, src);
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_XCHG_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT32 src = READ32(ea);
		UINT32 dst = LOAD_REG32(modrm);
		STORE_REG32(modrm, src);
		WRITE32(ea, dst);
		CYCLES(CYCLES_XCHG_REG_MEM);
	}
}

static void I386OP(xor_rm32_r32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		dst = XOR32(dst, src);
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = READ32(ea);
		dst = XOR32(dst, src);
		WRITE32(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(xor_r32_rm32)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM32(modrm);
		dst = LOAD_REG32(modrm);
		dst = XOR32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ32(ea);
		dst = LOAD_REG32(modrm);
		dst = XOR32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(xor_eax_i32)(void)
{
	UINT32 src, dst;
	src = FETCH32();
	dst = REG32(EAX);
	dst = XOR32(dst, src);
	REG32(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}


static void I386OP(group81_32)(void)
{
	UINT32 ea;
	UINT32 src, dst;
	UINT8 modrm = FETCH();

	switch( (modrm >> 3) & 0x7 )
	{
		case 0:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM32(modrm);
				src = FETCH32();
				dst = ADD32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ32(ea);
				src = FETCH32();
				dst = ADD32(dst, src);
				WRITE32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 1:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM32(modrm);
				src = FETCH32();
				dst = OR32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ32(ea);
				src = FETCH32();
				dst = OR32(dst, src);
				WRITE32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 2:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM32(modrm);
				src = FETCH32();
				dst = ADC32(dst, src, I.CF);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ32(ea);
				src = FETCH32();
				dst = ADC32(dst, src, I.CF);
				WRITE32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 3:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM32(modrm);
				src = FETCH32();
				dst = SBB32(dst, src, I.CF);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ32(ea);
				src = FETCH32();
				dst = SBB32(dst, src, I.CF);
				WRITE32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 4:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM32(modrm);
				src = FETCH32();
				dst = AND32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ32(ea);
				src = FETCH32();
				dst = AND32(dst, src);
				WRITE32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 5:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM32(modrm);
				src = FETCH32();
				dst = SUB32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ32(ea);
				src = FETCH32();
				dst = SUB32(dst, src);
				WRITE32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 6:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM32(modrm);
				src = FETCH32();
				dst = XOR32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ32(ea);
				src = FETCH32();
				dst = XOR32(dst, src);
				WRITE32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 7:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM32(modrm);
				src = FETCH32();
				SUB32(dst, src);
				CYCLES(CYCLES_CMP_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ32(ea);
				src = FETCH32();
				SUB32(dst, src);
				CYCLES(CYCLES_CMP_REG_MEM);
			}
			break;
	}
}

static void I386OP(group83_32)(void)
{
	UINT32 ea;
	UINT32 src, dst;
	UINT8 modrm = FETCH();

	switch( (modrm >> 3) & 0x7 )
	{
		case 0:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM32(modrm);
				src = (UINT32)(INT32)(INT8)FETCH();
				dst = ADD32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ32(ea);
				src = (UINT32)(INT32)(INT8)FETCH();
				dst = ADD32(dst, src);
				WRITE32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 1:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM32(modrm);
				src = (UINT32)(INT32)(INT8)FETCH();
				dst = OR32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ32(ea);
				src = (UINT32)(INT32)(INT8)FETCH();
				dst = OR32(dst, src);
				WRITE32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 2:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM32(modrm);
				src = (UINT32)(INT32)(INT8)FETCH();
				dst = ADC32(dst, src, I.CF);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ32(ea);
				src = (UINT32)(INT32)(INT8)FETCH();
				dst = ADC32(dst, src, I.CF);
				WRITE32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 3:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM32(modrm);
				src = ((UINT32)(INT32)(INT8)FETCH());
				dst = SBB32(dst, src, I.CF);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ32(ea);
				src = ((UINT32)(INT32)(INT8)FETCH());
				dst = SBB32(dst, src, I.CF);
				WRITE32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 4:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM32(modrm);
				src = (UINT32)(INT32)(INT8)FETCH();
				dst = AND32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ32(ea);
				src = (UINT32)(INT32)(INT8)FETCH();
				dst = AND32(dst, src);
				WRITE32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 5:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM32(modrm);
				src = (UINT32)(INT32)(INT8)FETCH();
				dst = SUB32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ32(ea);
				src = (UINT32)(INT32)(INT8)FETCH();
				dst = SUB32(dst, src);
				WRITE32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 6:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM32(modrm);
				src = (UINT32)(INT32)(INT8)FETCH();
				dst = XOR32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ32(ea);
				src = (UINT32)(INT32)(INT8)FETCH();
				dst = XOR32(dst, src);
				WRITE32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 7:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM32(modrm);
				src = (UINT32)(INT32)(INT8)FETCH();
				SUB32(dst, src);
				CYCLES(CYCLES_CMP_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ32(ea);
				src = (UINT32)(INT32)(INT8)FETCH();
				SUB32(dst, src);
				CYCLES(CYCLES_CMP_REG_MEM);
			}
			break;
	}
}

static void I386OP(groupC1_32)(void)
{
	UINT32 dst;
	UINT8 modrm = FETCH();
	UINT8 shift;

	if( modrm >= 0xc0 ) {
		dst = LOAD_RM32(modrm);
		shift = FETCH() & 0x1f;
		dst = i386_shift_rotate32(modrm, dst, shift);
		STORE_RM32(modrm, dst);
	} else {
		UINT32 ea = GetEA(modrm);
		dst = READ32(ea);
		shift = FETCH() & 0x1f;
		dst = i386_shift_rotate32(modrm, dst, shift);
		WRITE32(ea, dst);
	}
}

static void I386OP(groupD1_32)(void)
{
	UINT32 dst;
	UINT8 modrm = FETCH();

	if( modrm >= 0xc0 ) {
		dst = LOAD_RM32(modrm);
		dst = i386_shift_rotate32(modrm, dst, 1);
		STORE_RM32(modrm, dst);
	} else {
		UINT32 ea = GetEA(modrm);
		dst = READ32(ea);
		dst = i386_shift_rotate32(modrm, dst, 1);
		WRITE32(ea, dst);
	}
}

static void I386OP(groupD3_32)(void)
{
	UINT32 dst;
	UINT8 modrm = FETCH();

	if( modrm >= 0xc0 ) {
		dst = LOAD_RM32(modrm);
		dst = i386_shift_rotate32(modrm, dst, REG8(CL));
		STORE_RM32(modrm, dst);
	} else {
		UINT32 ea = GetEA(modrm);
		dst = READ32(ea);
		dst = i386_shift_rotate32(modrm, dst, REG8(CL));
		WRITE32(ea, dst);
	}
}

static void I386OP(groupF7_32)(void)
{
	UINT8 modrm = FETCH();

	switch( (modrm >> 3) & 0x7 )
	{
		case 0:
			if( modrm >= 0xc0 ) {
				UINT32 dst = LOAD_RM32(modrm);
				UINT32 src = FETCH32();
				dst &= src;
				I.CF = I.OF = I.AF = 0;
				SetSZPF32(dst);
				CYCLES(CYCLES_TEST_IMM_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT32 dst = READ32(ea);
				UINT32 src = FETCH32();
				dst &= src;
				I.CF = I.OF = I.AF = 0;
				SetSZPF32(dst);
				CYCLES(CYCLES_TEST_IMM_MEM);
			}
			break;
		case 2:
			if( modrm >= 0xc0 ) {
				UINT32 dst = LOAD_RM32(modrm);
				dst = ~dst;
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_NOT_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT32 dst = READ32(ea);
				dst = ~dst;
				WRITE32(ea, dst);
				CYCLES(CYCLES_NOT_MEM);
			}
			break;
		case 3:
			if( modrm >= 0xc0 ) {
				UINT32 dst = LOAD_RM32(modrm);
				dst = SUB32(0, dst );
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_NEG_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT32 dst = READ32(ea);
				dst = SUB32(0, dst );
				WRITE32(ea, dst);
				CYCLES(CYCLES_NEG_MEM);
			}
			break;
		case 4:
			{
				UINT64 result;
				UINT32 src, dst;
				if( modrm >= 0xc0 ) {
					src = LOAD_RM32(modrm);
					CYCLES(CYCLES_MUL32_ACC_REG);
				} else {
					UINT32 ea = GetEA(modrm);
					src = READ32(ea);
					CYCLES(CYCLES_MUL32_ACC_MEM);
				}

				dst = REG32(EAX);
				result = (UINT64)src * (UINT64)dst;
				REG32(EDX) = (UINT32)(result >> 32);
				REG32(EAX) = (UINT32)result;

				I.CF = I.OF = (REG32(EDX) != 0);
			}
			break;
		case 5:
			{
				INT64 result;
				INT64 src, dst;
				if( modrm >= 0xc0 ) {
					src = (INT64)(INT32)LOAD_RM32(modrm);
					CYCLES(CYCLES_IMUL32_ACC_REG);
				} else {
					UINT32 ea = GetEA(modrm);
					src = (INT64)(INT32)READ32(ea);
					CYCLES(CYCLES_IMUL32_ACC_MEM);
				}

				dst = (INT64)(INT32)REG32(EAX);
				result = src * dst;

				REG32(EDX) = (UINT32)(result >> 32);
				REG32(EAX) = (UINT32)result;

				I.CF = I.OF = !(result == (INT64)(INT32)result);
			}
			break;
		case 6:
			{
				UINT64 quotient, remainder, result;
				UINT32 src;
				if( modrm >= 0xc0 ) {
					src = LOAD_RM32(modrm);
					CYCLES(CYCLES_DIV32_ACC_REG);
				} else {
					UINT32 ea = GetEA(modrm);
					src = READ32(ea);
					CYCLES(CYCLES_DIV32_ACC_MEM);
				}

				quotient = ((UINT64)(REG32(EDX)) << 32) | (UINT64)(REG32(EAX));
				if( src ) {
					remainder = quotient % (UINT64)src;
					result = quotient / (UINT64)src;
					if( result > 0xffffffff ) {

					} else {
						REG32(EDX) = (UINT32)remainder;
						REG32(EAX) = (UINT32)result;
					}
				} else {

				}
			}
			break;
		case 7:
			{
				INT64 quotient, remainder, result;
				UINT32 src;
				if( modrm >= 0xc0 ) {
					src = LOAD_RM32(modrm);
					CYCLES(CYCLES_IDIV32_ACC_REG);
				} else {
					UINT32 ea = GetEA(modrm);
					src = READ32(ea);
					CYCLES(CYCLES_IDIV32_ACC_MEM);
				}

				quotient = (((INT64)REG32(EDX)) << 32) | ((UINT64)REG32(EAX));
				if( src ) {
					remainder = quotient % (INT64)(INT32)src;
					result = quotient / (INT64)(INT32)src;
					if( result > 0xffffffff ) {

					} else {
						REG32(EDX) = (UINT32)remainder;
						REG32(EAX) = (UINT32)result;
					}
				} else {

				}
			}
			break;
	}
}

static void I386OP(groupFF_32)(void)
{
	UINT8 modrm = FETCH();

	switch( (modrm >> 3) & 0x7 )
	{
		case 0:
			if( modrm >= 0xc0 ) {
				UINT32 dst = LOAD_RM32(modrm);
				dst = INC32(dst);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_INC_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT32 dst = READ32(ea);
				dst = INC32(dst);
				WRITE32(ea, dst);
				CYCLES(CYCLES_INC_MEM);
			}
			break;
		case 1:
			if( modrm >= 0xc0 ) {
				UINT32 dst = LOAD_RM32(modrm);
				dst = DEC32(dst);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_DEC_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT32 dst = READ32(ea);
				dst = DEC32(dst);
				WRITE32(ea, dst);
				CYCLES(CYCLES_DEC_MEM);
			}
			break;
		case 2:
			{
				UINT32 address;
				if( modrm >= 0xc0 ) {
					address = LOAD_RM32(modrm);
					CYCLES(CYCLES_CALL_REG);
				} else {
					UINT32 ea = GetEA(modrm);
					address = READ32(ea);
					CYCLES(CYCLES_CALL_MEM);
				}
				PUSH32(I.eip );
				I.eip = address;
				CHANGE_PC(I.eip);
			}
			break;
		case 3:
			{
				UINT16 selector;
				UINT32 address;

				if( modrm >= 0xc0 )
				{
					fatalerror("i386: groupFF_32 /%d: NYI", (modrm >> 3) & 0x7);
				}
				else
				{
					UINT32 ea = GetEA(modrm);
					address = READ32(ea + 0);
					selector = READ16(ea + 4);
					CYCLES(CYCLES_CALL_MEM_INTERSEG);
					if(PROTECTED_MODE && !V8086_MODE)
					{
						i386_protected_mode_call(selector,address,1,1);
					}
					else
					{
						PUSH32(I.sreg[CS].selector );
						PUSH32(I.eip );
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
				UINT32 address;
				if( modrm >= 0xc0 ) {
					address = LOAD_RM32(modrm);
					CYCLES(CYCLES_JMP_REG);
				} else {
					UINT32 ea = GetEA(modrm);
					address = READ32(ea);
					CYCLES(CYCLES_JMP_MEM);
				}
				I.eip = address;
				CHANGE_PC(I.eip);
			}
			break;
		case 5:
			{
				UINT16 selector;
				UINT32 address;

				if( modrm >= 0xc0 )
				{
					fatalerror("i386: groupFF_32 /%d: NYI", (modrm >> 3) & 0x7);
				}
				else
				{
					UINT32 ea = GetEA(modrm);
					address = READ32(ea + 0);
					selector = READ16(ea + 4);
					CYCLES(CYCLES_JMP_MEM_INTERSEG);
					if(PROTECTED_MODE && !V8086_MODE)
					{
						i386_protected_mode_jump(selector,address,1,1);
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
				UINT32 value;
				if( modrm >= 0xc0 ) {
					value = LOAD_RM32(modrm);
				} else {
					UINT32 ea = GetEA(modrm);
					value = READ32(ea);
				}
				PUSH32(value);
				CYCLES(CYCLES_PUSH_RM);
			}
			break;
		default:
			fatalerror("i386: groupFF_32 /%d unimplemented at %08X", (modrm >> 3) & 0x7, I.pc-2);
			break;
	}
}

static void I386OP(group0F00_32)(void)
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
					address = LOAD_RM32(modrm);
					STORE_RM32(address, I.ldtr.segment);
					CYCLES(CYCLES_SLDT_REG);
				} else {
					ea = GetEA(modrm);
					WRITE32(ea, I.ldtr.segment);
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
					address = LOAD_RM32(modrm);
					STORE_RM32(address, I.task.segment);
					CYCLES(CYCLES_STR_REG);
				} else {
					ea = GetEA(modrm);
					WRITE32(ea, I.task.segment);
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
					address = LOAD_RM32(modrm);
					I.ldtr.segment = address;
					CYCLES(CYCLES_LLDT_REG);
				} else {
					ea = GetEA(modrm);
					I.ldtr.segment = READ32(ea);
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
					address = LOAD_RM32(modrm);
					I.task.segment = address;
					CYCLES(CYCLES_LTR_REG);
				} else {
					ea = GetEA(modrm);
					I.task.segment = READ32(ea);
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
					address = LOAD_RM32(modrm);
					CYCLES(CYCLES_VERR_REG);
				} else {
					ea = GetEA(modrm);
					address = READ32(ea);
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
			fatalerror("i386: group0F00_32 /%d unimplemented", (modrm >> 3) & 0x7);
			break;
	}
}

static void I386OP(group0F01_32)(void)
{
	UINT8 modrm = FETCH();
	UINT32 address, ea;

	switch( (modrm >> 3) & 0x7 )
	{
		case 0:
			{
				if( modrm >= 0xc0 ) {
					address = LOAD_RM32(modrm);
					ea = i386_translate(CS, address );
				} else {
					ea = GetEA(modrm);
				}
				WRITE16(ea, I.gdtr.limit);
				WRITE32(ea + 2, I.gdtr.base);
				CYCLES(CYCLES_SGDT);
				break;
			}
		case 1:
			{
				if (modrm >= 0xc0)
				{
					address = LOAD_RM32(modrm);
					ea = i386_translate(CS, address );
				}
				else
				{
					ea = GetEA(modrm);
				}
				WRITE16(ea, I.idtr.limit);
				WRITE32(ea + 2, I.idtr.base);
				CYCLES(CYCLES_SIDT);
				break;
			}
		case 2:
			{
				if( modrm >= 0xc0 ) {
					address = LOAD_RM32(modrm);
					ea = i386_translate(CS, address );
				} else {
					ea = GetEA(modrm);
				}
				I.gdtr.limit = READ16(ea);
				I.gdtr.base = READ32(ea + 2);
				CYCLES(CYCLES_LGDT);
				break;
			}
		case 3:
			{
				if( modrm >= 0xc0 ) {
					address = LOAD_RM32(modrm);
					ea = i386_translate(CS, address );
				} else {
					ea = GetEA(modrm);
				}
				I.idtr.limit = READ16(ea);
				I.idtr.base = READ32(ea + 2);
				CYCLES(CYCLES_LIDT);
				break;
			}
		default:
			fatalerror("i386: unimplemented opcode 0x0f 01 /%d at %08X", (modrm >> 3) & 0x7, I.eip - 2);
			break;
	}
}

static void I386OP(group0FBA_32)(void)
{
	UINT8 modrm = FETCH();

	switch( (modrm >> 3) & 0x7 )
	{
		case 4:
			if( modrm >= 0xc0 ) {
				UINT32 dst = LOAD_RM32(modrm);
				UINT8 bit = FETCH();

				if( dst & (1 << bit) )
					I.CF = 1;
				else
					I.CF = 0;

				CYCLES(CYCLES_BT_IMM_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT32 dst = READ32(ea);
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
				UINT32 dst = LOAD_RM32(modrm);
				UINT8 bit = FETCH();

				if( dst & (1 << bit) )
					I.CF = 1;
				else
					I.CF = 0;
				dst |= (1 << bit);

				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_BTS_IMM_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT32 dst = READ32(ea);
				UINT8 bit = FETCH();

				if( dst & (1 << bit) )
					I.CF = 1;
				else
					I.CF = 0;
				dst |= (1 << bit);

				WRITE32(ea, dst);
				CYCLES(CYCLES_BTS_IMM_MEM);
			}
			break;
		case 6:
			if( modrm >= 0xc0 ) {
				UINT32 dst = LOAD_RM32(modrm);
				UINT8 bit = FETCH();

				if( dst & (1 << bit) )
					I.CF = 1;
				else
					I.CF = 0;
				dst &= ~(1 << bit);

				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_BTR_IMM_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT32 dst = READ32(ea);
				UINT8 bit = FETCH();

				if( dst & (1 << bit) )
					I.CF = 1;
				else
					I.CF = 0;
				dst &= ~(1 << bit);

				WRITE32(ea, dst);
				CYCLES(CYCLES_BTR_IMM_MEM);
			}
			break;
		case 7:
			if( modrm >= 0xc0 ) {
				UINT32 dst = LOAD_RM32(modrm);
				UINT8 bit = FETCH();

				if( dst & (1 << bit) )
					I.CF = 1;
				else
					I.CF = 0;
				dst ^= (1 << bit);

				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_BTC_IMM_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT32 dst = READ32(ea);
				UINT8 bit = FETCH();

				if( dst & (1 << bit) )
					I.CF = 1;
				else
					I.CF = 0;
				dst ^= (1 << bit);

				WRITE32(ea, dst);
				CYCLES(CYCLES_BTC_IMM_MEM);
			}
			break;
		default:
			fatalerror("i386: group0FBA_32 /%d unknown", (modrm >> 3) & 0x7);
			break;
	}
}

static void I386OP(lar_r32_rm32)(void)
{
	UINT8 modrm = FETCH();
	I386_SREG seg;
	UINT8 type;

	if(PROTECTED_MODE && !V8086_MODE)
	{
		memset(&seg,0,sizeof(seg));
		if(modrm >= 0xc0)
		{
			seg.selector = LOAD_RM32(modrm);
			CYCLES(CYCLES_LAR_REG);
		}
		else
		{
			UINT32 ea = GetEA(modrm);
			seg.selector = READ32(ea);
			CYCLES(CYCLES_LAR_MEM);
		}
		if(seg.selector == 0)
		{
			SetZF(0);
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

					STORE_REG32(modrm,(seg.flags << 8) & 0x00ffff00);
					SetZF(1);
				}
			}
			else
			{

				STORE_REG32(modrm,(seg.flags << 8) & 0x00ffff00);
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

static void I386OP(lsl_r32_rm32)(void)
{
	UINT8 modrm = FETCH();
	UINT32 limit;
	I386_SREG seg;

	if(PROTECTED_MODE)
	{
		memset(&seg, 0, sizeof(seg));
		if(modrm >= 0xc0)
		{
			seg.selector = LOAD_RM32(modrm);
		}
		else
		{
			UINT32 ea = GetEA(modrm);
			seg.selector = READ32(ea);
		}
		if(seg.selector == 0)
		{
			SetZF(0);
		}
		else
		{

			i386_load_protected_mode_segment(&seg);
			limit = seg.limit;
			STORE_REG32(modrm,limit);
			SetZF(1);
		}
	}
	else
		i386_trap(6, 0, 0);
}

static void I386OP(bound_r32_m32_m32)(void)
{
	UINT8 modrm;
	INT32 val, low, high;

	modrm = FETCH();

	if (modrm >= 0xc0)
	{
		low = high = LOAD_RM32(modrm);
	}
	else
	{
		UINT32 ea = GetEA(modrm);
		low = READ32(ea + 0);
		high = READ32(ea + 4);
	}
	val = LOAD_REG32(modrm);

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

static void I386OP(retf32)(void)
{
	if(PROTECTED_MODE && !V8086_MODE)
	{
		i386_protected_mode_retf(0,1);
	}
	else
	{
		I.eip = POP32();
		I.sreg[CS].selector = POP32();
		i386_load_segment_descriptor(CS );
		CHANGE_PC(I.eip);
	}

	CYCLES(CYCLES_RET_INTERSEG);
}

static void I386OP(retf_i32)(void)
{
	UINT16 count = FETCH16();

	if(PROTECTED_MODE && !V8086_MODE)
	{
		i386_protected_mode_retf(count,1);
	}
	else
	{
		I.eip = POP32();
		I.sreg[CS].selector = POP32();
		i386_load_segment_descriptor(CS );
		CHANGE_PC(I.eip);
		REG32(ESP) += count;
	}

	CYCLES(CYCLES_RET_IMM_INTERSEG);
}

static void I386OP(xlat32)(void)
{
	UINT32 ea;
	if( I.segment_prefix ) {
		ea = i386_translate(I.segment_override, REG32(EBX) + REG8(AL) );
	} else {
		ea = i386_translate(DS, REG32(EBX) + REG8(AL) );
	}
	REG8(AL) = READ8(ea);
	CYCLES(CYCLES_XLAT);
}

static void I386OP(load_far_pointer32)(int s)
{
	UINT8 modrm = FETCH();

	if( modrm >= 0xc0 ) {
		fatalerror("i386: load_far_pointer32 NYI");
	} else {
		UINT32 ea = GetEA(modrm);
		STORE_REG32(modrm, READ32(ea + 0));
		I.sreg[s].selector = READ16(ea + 4);
		i386_load_segment_descriptor(s );
	}
}

static void I386OP(lds32)(void)
{
	I386OP(load_far_pointer32)(DS);
	CYCLES(CYCLES_LDS);
}

static void I386OP(lss32)(void)
{
	I386OP(load_far_pointer32)(SS);
	CYCLES(CYCLES_LSS);
}

static void I386OP(les32)(void)
{
	I386OP(load_far_pointer32)(ES);
	CYCLES(CYCLES_LES);
}

static void I386OP(lfs32)(void)
{
	I386OP(load_far_pointer32)(FS);
	CYCLES(CYCLES_LFS);
}

static void I386OP(lgs32)(void)
{
	I386OP(load_far_pointer32)(GS);
	CYCLES(CYCLES_LGS);
}

// license:BSD-3-Clause
// copyright-holders:Ville Linde, Barry Rodewald
static UINT8 I386OP(shift_rotate8)(UINT8 modrm, UINT32 value, UINT8 shift)
{
	UINT8 src = value;
	UINT8 dst = value;

	if( shift == 0 ) {
		CYCLES_RM(modrm, 3, 7);
	} else if( shift == 1 ) {

		switch( (modrm >> 3) & 0x7 )
		{
			case 0:
				I.CF = (src & 0x80) ? 1 : 0;
				dst = (src << 1) + I.CF;
				I.OF = ((src ^ dst) & 0x80) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 1:
				I.CF = (src & 0x1) ? 1 : 0;
				dst = (I.CF << 7) | (src >> 1);
				I.OF = ((src ^ dst) & 0x80) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 2:
				dst = (src << 1) + I.CF;
				I.CF = (src & 0x80) ? 1 : 0;
				I.OF = ((src ^ dst) & 0x80) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 3:
				dst = (I.CF << 7) | (src >> 1);
				I.CF = src & 0x1;
				I.OF = ((src ^ dst) & 0x80) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 4:
			case 6:
				dst = src << 1;
				I.CF = (src & 0x80) ? 1 : 0;
				I.OF = (((I.CF << 7) ^ dst) & 0x80) ? 1 : 0;
				SetSZPF8(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 5:
				dst = src >> 1;
				I.CF = src & 0x1;
				I.OF = (dst & 0x80) ? 1 : 0;
				SetSZPF8(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 7:
				dst = (INT8)(src) >> 1;
				I.CF = src & 0x1;
				I.OF = 0;
				SetSZPF8(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
		}

	} else {

		switch( (modrm >> 3) & 0x7 )
		{
			case 0:
				dst = ((src & ((UINT8)0xff >> shift)) << shift) |
					  ((src & ((UINT8)0xff << (8-shift))) >> (8-shift));
				I.CF = (src >> (8-shift)) & 0x1;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 1:
				dst = ((src & ((UINT8)0xff << shift)) >> shift) |
					  ((src & ((UINT8)0xff >> (8-shift))) << (8-shift));
				I.CF = (src >> (shift-1)) & 0x1;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 2:
				dst = ((src & ((UINT8)0xff >> shift)) << shift) |
					  ((src & ((UINT8)0xff << (9-shift))) >> (9-shift)) |
					  (I.CF << (shift-1));
				I.CF = (src >> (8-shift)) & 0x1;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 3:
				dst = ((src & ((UINT8)0xff << shift)) >> shift) |
					  ((src & ((UINT8)0xff >> (8-shift))) << (9-shift)) |
					  (I.CF << (8-shift));
				I.CF = (src >> (shift-1)) & 0x1;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 4:
			case 6:
				dst = src << shift;
				I.CF = (src & (1 << (8-shift))) ? 1 : 0;
				SetSZPF8(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 5:
				dst = src >> shift;
				I.CF = (src & (1 << (shift-1))) ? 1 : 0;
				SetSZPF8(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 7:
				dst = (INT8)src >> shift;
				I.CF = (src & (1 << (shift-1))) ? 1 : 0;
				SetSZPF8(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
		}
	}

	return dst;
}


static void I386OP(adc_rm8_r8)(void)
{
	UINT8 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		dst = ADC8(dst, src, I.CF);
		STORE_RM8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = READ8(ea);
		dst = ADC8(dst, src, I.CF);
		WRITE8(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(adc_r8_rm8)(void)
{
	UINT8 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM8(modrm);
		dst = LOAD_REG8(modrm);
		dst = ADC8(dst, src, I.CF);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ8(ea);
		dst = LOAD_REG8(modrm);
		dst = ADC8(dst, src, I.CF);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(adc_al_i8)(void)
{
	UINT8 src, dst;
	src = FETCH();
	dst = REG8(AL);
	dst = ADC8(dst, src, I.CF);
	REG8(AL) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(add_rm8_r8)(void)
{
	UINT8 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		dst = ADD8(dst, src);
		STORE_RM8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = READ8(ea);
		dst = ADD8(dst, src);
		WRITE8(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(add_r8_rm8)(void)
{
	UINT8 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM8(modrm);
		dst = LOAD_REG8(modrm);
		dst = ADD8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ8(ea);
		dst = LOAD_REG8(modrm);
		dst = ADD8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(add_al_i8)(void)
{
	UINT8 src, dst;
	src = FETCH();
	dst = REG8(AL);
	dst = ADD8(dst, src);
	REG8(AL) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(and_rm8_r8)(void)
{
	UINT8 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		dst = AND8(dst, src);
		STORE_RM8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = READ8(ea);
		dst = AND8(dst, src);
		WRITE8(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(and_r8_rm8)(void)
{
	UINT8 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM8(modrm);
		dst = LOAD_REG8(modrm);
		dst = AND8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ8(ea);
		dst = LOAD_REG8(modrm);
		dst = AND8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(and_al_i8)(void)
{
	UINT8 src, dst;
	src = FETCH();
	dst = REG8(AL);
	dst = AND8(dst, src);
	REG8(AL) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(clc)(void)
{
	I.CF = 0;
	CYCLES(CYCLES_CLC);
}

static void I386OP(cld)(void)
{
	I.DF = 0;
	CYCLES(CYCLES_CLD);
}

static void I386OP(cli)(void)
{
	I.IF = 0;
	CYCLES(CYCLES_CLI);
}

static void I386OP(cmc)(void)
{
	I.CF ^= 1;
	CYCLES(CYCLES_CMC);
}

static void I386OP(cmp_rm8_r8)(void)
{
	UINT8 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		SUB8(dst, src);
		CYCLES(CYCLES_CMP_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = READ8(ea);
		SUB8(dst, src);
		CYCLES(CYCLES_CMP_REG_MEM);
	}
}

static void I386OP(cmp_r8_rm8)(void)
{
	UINT8 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM8(modrm);
		dst = LOAD_REG8(modrm);
		SUB8(dst, src);
		CYCLES(CYCLES_CMP_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ8(ea);
		dst = LOAD_REG8(modrm);
		SUB8(dst, src);
		CYCLES(CYCLES_CMP_MEM_REG);
	}
}

static void I386OP(cmp_al_i8)(void)
{
	UINT8 src, dst;
	src = FETCH();
	dst = REG8(AL);
	SUB8(dst, src);
	CYCLES(CYCLES_CMP_IMM_ACC);
}

static void I386OP(cmpsb)(void)
{
	UINT32 eas, ead;
	UINT8 src, dst;
	if( I.segment_prefix ) {
		eas = i386_translate(I.segment_override, I.address_size ? REG32(ESI) : REG16(SI) );
	} else {
		eas = i386_translate(DS, I.address_size ? REG32(ESI) : REG16(SI) );
	}
	ead = i386_translate(ES, I.address_size ? REG32(EDI) : REG16(DI) );
	src = READ8(eas);
	dst = READ8(ead);
	SUB8(dst, src);
	BUMP_SI(1);
	BUMP_DI(1);
	CYCLES(CYCLES_CMPS);
}

static void I386OP(in_al_i8)(void)
{
	UINT16 port = FETCH();
	UINT8 data = READPORT8(port);
	REG8(AL) = data;
	CYCLES(CYCLES_IN_VAR);
}

static void I386OP(in_al_dx)(void)
{
	UINT16 port = REG16(DX);
	UINT8 data = READPORT8(port);
	REG8(AL) = data;
	CYCLES(CYCLES_IN);
}

static void I386OP(ja_rel8)(void)
{
	INT8 disp = FETCH();
	if( I.CF == 0 && I.ZF == 0 ) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);
	} else {
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
	}
}

static void I386OP(jbe_rel8)(void)
{
	INT8 disp = FETCH();
	if( I.CF != 0 || I.ZF != 0 ) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);
	} else {
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
	}
}

static void I386OP(jc_rel8)(void)
{
	INT8 disp = FETCH();
	if( I.CF != 0 ) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);
	} else {
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
	}
}

static void I386OP(jg_rel8)(void)
{
	INT8 disp = FETCH();
	if( I.ZF == 0 && (I.SF == I.OF) ) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);
	} else {
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
	}
}

static void I386OP(jge_rel8)(void)
{
	INT8 disp = FETCH();
	if( (I.SF == I.OF) ) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);
	} else {
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
	}
}

static void I386OP(jl_rel8)(void)
{
	INT8 disp = FETCH();
	if( (I.SF != I.OF) ) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);
	} else {
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
	}
}

static void I386OP(jle_rel8)(void)
{
	INT8 disp = FETCH();
	if( I.ZF != 0 || (I.SF != I.OF) ) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);
	} else {
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
	}
}

static void I386OP(jnc_rel8)(void)
{
	INT8 disp = FETCH();
	if( I.CF == 0 ) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);
	} else {
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
	}
}

static void I386OP(jno_rel8)(void)
{
	INT8 disp = FETCH();
	if( I.OF == 0 ) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);
	} else {
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
	}
}

static void I386OP(jnp_rel8)(void)
{
	INT8 disp = FETCH();
	if( I.PF == 0 ) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);
	} else {
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
	}
}

static void I386OP(jns_rel8)(void)
{
	INT8 disp = FETCH();
	if( I.SF == 0 ) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);
	} else {
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
	}
}

static void I386OP(jnz_rel8)(void)
{
	INT8 disp = FETCH();
	if( I.ZF == 0 ) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);
	} else {
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
	}
}

static void I386OP(jo_rel8)(void)
{
	INT8 disp = FETCH();
	if( I.OF != 0 ) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);
	} else {
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
	}
}

static void I386OP(jp_rel8)(void)
{
	INT8 disp = FETCH();
	if( I.PF != 0 ) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);
	} else {
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
	}
}

static void I386OP(js_rel8)(void)
{
	INT8 disp = FETCH();
	if( I.SF != 0 ) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);
	} else {
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
	}
}

static void I386OP(jz_rel8)(void)
{
	INT8 disp = FETCH();
	if( I.ZF != 0 ) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);
	} else {
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
	}
}

static void I386OP(jmp_rel8)(void)
{
	INT8 disp = FETCH();
	NEAR_BRANCH(disp);
	CYCLES(CYCLES_JMP_SHORT);
}

static void I386OP(lahf)(void)
{
	REG8(AH) = get_flags() & 0xd7;
	CYCLES(CYCLES_LAHF);
}

static void I386OP(lodsb)(void)
{
	UINT32 eas;
	if( I.segment_prefix ) {
		eas = i386_translate(I.segment_override, I.address_size ? REG32(ESI) : REG16(SI) );
	} else {
		eas = i386_translate(DS, I.address_size ? REG32(ESI) : REG16(SI) );
	}
	REG8(AL) = READ8(eas);
	BUMP_SI(1);
	CYCLES(CYCLES_LODS);
}

static void I386OP(mov_rm8_r8)(void)
{
	UINT8 src;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG8(modrm);
		STORE_RM8(modrm, src);
		CYCLES(CYCLES_MOV_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		WRITE8(ea, src);
		CYCLES(CYCLES_MOV_REG_MEM);
	}
}

static void I386OP(mov_r8_rm8)(void)
{
	UINT8 src;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM8(modrm);
		STORE_REG8(modrm, src);
		CYCLES(CYCLES_MOV_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ8(ea);
		STORE_REG8(modrm, src);
		CYCLES(CYCLES_MOV_MEM_REG);
	}
}

static void I386OP(mov_rm8_i8)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT8 value = FETCH();
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_MOV_IMM_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT8 value = FETCH();
		WRITE8(ea, value);
		CYCLES(CYCLES_MOV_IMM_MEM);
	}
}

static void I386OP(mov_r32_cr)(void)
{
	UINT8 modrm = FETCH();
	UINT8 cr = (modrm >> 3) & 0x7;

	STORE_RM32(modrm, I.cr[cr]);
	CYCLES(CYCLES_MOV_CR_REG);
}

static void I386OP(mov_r32_dr)(void)
{
	UINT8 modrm = FETCH();
	UINT8 dr = (modrm >> 3) & 0x7;

	STORE_RM32(modrm, I.dr[dr]);
	switch(dr)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			CYCLES(CYCLES_MOV_REG_DR0_3);
			break;
		case 6:
		case 7:
			CYCLES(CYCLES_MOV_REG_DR6_7);
			break;
	}
}

static void I386OP(mov_cr_r32)(void)
{
	UINT8 modrm = FETCH();
	UINT8 cr = (modrm >> 3) & 0x7;

	I.cr[cr] = LOAD_RM32(modrm);
	switch(cr)
	{
		case 0: CYCLES(CYCLES_MOV_REG_CR0); break;
		case 2: CYCLES(CYCLES_MOV_REG_CR2); break;
		case 3: CYCLES(CYCLES_MOV_REG_CR3); break;
		case 4: CYCLES(1); break;
		default:
			fatalerror("i386: mov_cr_r32 CR%d !", cr);
			break;
	}
}

static void I386OP(mov_dr_r32)(void)
{
	UINT8 modrm = FETCH();
	UINT8 dr = (modrm >> 3) & 0x7;

	I.dr[dr] = LOAD_RM32(modrm);
	switch(dr)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			CYCLES(CYCLES_MOV_DR0_3_REG);
			break;
		case 6:
		case 7:
			CYCLES(CYCLES_MOV_DR6_7_REG);
			break;
		default:
			fatalerror("i386: mov_dr_r32 DR%d !", dr);
			break;
	}
}

static void I386OP(mov_al_m8)(void)
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
	REG8(AL) = READ8(ea);
	CYCLES(CYCLES_MOV_IMM_MEM);
}

static void I386OP(mov_m8_al)(void)
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
	WRITE8(ea, REG8(AL) );
	CYCLES(CYCLES_MOV_MEM_ACC);
}

static void I386OP(mov_rm16_sreg)(void)
{
	UINT8 modrm = FETCH();
	int s = (modrm >> 3) & 0x7;

	if( modrm >= 0xc0 ) {
		STORE_RM16(modrm, I.sreg[s].selector);
		CYCLES(CYCLES_MOV_SREG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE16(ea, I.sreg[s].selector);
		CYCLES(CYCLES_MOV_SREG_MEM);
	}
}

static void I386OP(mov_sreg_rm16)(void)
{
	UINT16 selector;
	UINT8 modrm = FETCH();
	int s = (modrm >> 3) & 0x7;

	if( modrm >= 0xc0 ) {
		selector = LOAD_RM16(modrm);
		CYCLES(CYCLES_MOV_REG_SREG);
	} else {
		UINT32 ea = GetEA(modrm);
		selector = READ16(ea);
		CYCLES(CYCLES_MOV_MEM_SREG);
	}

	if(s == SS)
	{
		if(I.IF != 0)
		{
			I.IF = 0;
			I.delayed_interrupt_enable = 1;
		}
	}

	I.sreg[s].selector = selector;
	i386_load_segment_descriptor(s );
}

static void I386OP(mov_al_i8)(void)
{
	REG8(AL) = FETCH();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_cl_i8)(void)
{
	REG8(CL) = FETCH();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_dl_i8)(void)
{
	REG8(DL) = FETCH();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_bl_i8)(void)
{
	REG8(BL) = FETCH();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_ah_i8)(void)
{
	REG8(AH) = FETCH();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_ch_i8)(void)
{
	REG8(CH) = FETCH();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_dh_i8)(void)
{
	REG8(DH) = FETCH();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(mov_bh_i8)(void)
{
	REG8(BH) = FETCH();
	CYCLES(CYCLES_MOV_IMM_REG);
}

static void I386OP(movsb)(void)
{
	UINT32 eas, ead;
	UINT8 v;
	if( I.segment_prefix ) {
		eas = i386_translate(I.segment_override, I.address_size ? REG32(ESI) : REG16(SI) );
	} else {
		eas = i386_translate(DS, I.address_size ? REG32(ESI) : REG16(SI) );
	}
	ead = i386_translate(ES, I.address_size ? REG32(EDI) : REG16(DI) );
	v = READ8(eas);
	WRITE8(ead, v);
	BUMP_SI(1);
	BUMP_DI(1);
	CYCLES(CYCLES_MOVS);
}

static void I386OP(or_rm8_r8)(void)
{
	UINT8 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		dst = OR8(dst, src);
		STORE_RM8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = READ8(ea);
		dst = OR8(dst, src);
		WRITE8(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(or_r8_rm8)(void)
{
	UINT8 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM8(modrm);
		dst = LOAD_REG8(modrm);
		dst = OR8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ8(ea);
		dst = LOAD_REG8(modrm);
		dst = OR8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(or_al_i8)(void)
{
	UINT8 src, dst;
	src = FETCH();
	dst = REG8(AL);
	dst = OR8(dst, src);
	REG8(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(out_al_i8)(void)
{
	UINT16 port = FETCH();
	UINT8 data = REG8(AL);
	WRITEPORT8(port, data);
	CYCLES(CYCLES_OUT_VAR);
}

static void I386OP(out_al_dx)(void)
{
	UINT16 port = REG16(DX);
	UINT8 data = REG8(AL);
	WRITEPORT8(port, data);
	CYCLES(CYCLES_OUT);
}

static void I386OP(arpl)(void)
{
	UINT16 src, dst;
	UINT8 modrm = FETCH();
	UINT8 flag = 0;

     if( modrm >= 0xc0 ) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		if( (dst&0x3) < (src&0x3) ) {
			dst = (dst&0xfffc) | (src&0x3);
			flag = 1;
			STORE_RM16(modrm, dst);
		}
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = READ16(ea);
		if( (dst&0x3) < (src&0x3) ) {
			dst = (dst&0xfffc) | (src&0x3);
			flag = 1;
			WRITE16(ea, dst);
		}
	}
	SetZF(flag);
}

static void I386OP(push_i8)(void)
{
	UINT8 value = FETCH();
	PUSH8(value);
	CYCLES(CYCLES_PUSH_IMM);
}

static void I386OP(ins_generic)(int size)
{
	UINT32 ead;
	UINT8 vb;
	UINT16 vw;
	UINT32 vd;

	ead = i386_translate(ES, I.address_size ? REG32(EDI) : REG16(DI) );

	switch(size) {
	case 1:
		vb = READPORT8(REG16(DX));
		WRITE8(ead, vb);
		break;
	case 2:
		vw = READPORT16(REG16(DX));
		WRITE16(ead, vw);
		break;
	case 4:
		vd = READPORT32(REG16(DX));
		WRITE32(ead, vd);
		break;
	}

	if(I.address_size)
		REG32(EDI) += ((I.DF) ? -1 : 1) * size;
	else
		REG16(DI) += ((I.DF) ? -1 : 1) * size;
	CYCLES(CYCLES_INS);
}

static void I386OP(insb)(void)
{
	I386OP(ins_generic)(1);
}

static void I386OP(insw)(void)
{
	I386OP(ins_generic)(2);
}

static void I386OP(insd)(void)
{
	I386OP(ins_generic)(4);
}

static void I386OP(outs_generic)(int size)
{
	UINT32 eas;
	UINT8 vb;
	UINT16 vw;
	UINT32 vd;

	if( I.segment_prefix ) {
		eas = i386_translate(I.segment_override, I.address_size ? REG32(ESI) : REG16(SI) );
	} else {
		eas = i386_translate(DS, I.address_size ? REG32(ESI) : REG16(SI) );
	}

	switch(size) {
	case 1:
		vb = READ8(eas);
		WRITEPORT8(REG16(DX), vb);
		break;
	case 2:
		vw = READ16(eas);
		WRITEPORT16(REG16(DX), vw);
		break;
	case 4:
		vd = READ32(eas);
		WRITEPORT32(REG16(DX), vd);
		break;
	}

	if(I.address_size)
		REG32(ESI) += ((I.DF) ? -1 : 1) * size;
	else
		REG16(SI) += ((I.DF) ? -1 : 1) * size;
	CYCLES(CYCLES_OUTS);
}

static void I386OP(outsb)(void)
{
	I386OP(outs_generic)(1);
}

static void I386OP(outsw)(void)
{
	I386OP(outs_generic)(2);
}

static void I386OP(outsd)(void)
{
	I386OP(outs_generic)(4);
}

static void I386OP(repeat)(int invert_flag)
{
	UINT32 repeated_eip = I.eip;
	UINT32 repeated_pc = I.pc;
	UINT8 opcode;

	UINT32 count;
	INT32 cycle_base = 0, cycle_adjustment = 0;
	UINT8 prefix_flag=1;
	UINT8 *flag = NULL;

	do {
	repeated_eip = I.eip;
	repeated_pc = I.pc;
	opcode = FETCH();
	switch(opcode) {
		case 0x26:
	    I.segment_override=ES;
		I.segment_prefix=1;
		break;
		case 0x2e:
	    I.segment_override=CS;
		I.segment_prefix=1;
		break;
		case 0x36:
	    I.segment_override=SS;
		I.segment_prefix=1;
		break;
		case 0x3e:
	    I.segment_override=DS;
		I.segment_prefix=1;
		break;
		case 0x64:
	    I.segment_override=FS;
		I.segment_prefix=1;
		break;
		case 0x65:
	    I.segment_override=GS;
		I.segment_prefix=1;
		break;
		case 0x66:
		I.operand_size ^= 1;
		break;
		case 0x67:
		I.address_size ^= 1;
		break;
        default:
		prefix_flag=0;
	}
	} while (prefix_flag);

	if( I.segment_prefix ) {

		i386_translate(I.segment_override, I.sreg[I.segment_prefix].d ? REG32(ESI) : REG16(SI) );
	} else {

		i386_translate(DS, I.address_size ? REG32(ESI) : REG16(SI) );
	}
	i386_translate(ES, I.address_size ? REG32(EDI) : REG16(DI) );

	switch(opcode)
	{
		case 0x6c:
		case 0x6d:

			cycle_base = 8;
			cycle_adjustment = -4;
			flag = NULL;
			break;

		case 0x6e:
		case 0x6f:

			cycle_base = 8;
			cycle_adjustment = -4;
			flag = NULL;
			break;

		case 0xa4:
		case 0xa5:

			cycle_base = 8;
			cycle_adjustment = -4;
			flag = NULL;
			break;

		case 0xa6:
		case 0xa7:

			cycle_base = 5;
			cycle_adjustment = -1;
			flag = &I.ZF;
			break;

		case 0xac:
		case 0xad:

			cycle_base = 5;
			cycle_adjustment = 1;
			flag = NULL;
			break;

		case 0xaa:
		case 0xab:

			cycle_base = 5;
			cycle_adjustment = 0;
			flag = NULL;
			break;

		case 0xae:
		case 0xaf:

			cycle_base = 5;
			cycle_adjustment = 0;
			flag = &I.ZF;
			break;

		default:
			fatalerror("i386: Invalid REP/opcode %02X combination",opcode);
			break;
	}

	if( I.address_size ) {
		if( REG32(ECX) == 0 )
			return;
	} else {
		if( REG16(CX) == 0 )
			return;
	}

	CYCLES_NUM(cycle_base);
	do
	{
		I.eip = repeated_eip;
		I.pc = repeated_pc;
		I386OP(decode_opcode)();
		CYCLES_NUM(cycle_adjustment);

		if (I.address_size)
			count = --REG32(ECX);
		else
			count = --REG16(CX);
		if (I.cycles <= 0)
			goto outofcycles;
	}
	while( count && (!flag || (invert_flag ? !*flag : *flag)) );
	return;

outofcycles:

	I.eip = I.prev_eip;
	CHANGE_PC(I.eip);
	CYCLES_NUM(-cycle_base);
}

static void I386OP(rep)(void)
{
	I386OP(repeat)(0);
}

static void I386OP(repne)(void)
{
	I386OP(repeat)(1);
}

static void I386OP(sahf)(void)
{
	set_flags((get_flags() & 0xffffff00) | (REG8(AH) & 0xd7) );
	CYCLES(CYCLES_SAHF);
}

static void I386OP(sbb_rm8_r8)(void)
{
	UINT8 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		dst = SBB8(dst, src, I.CF);
		STORE_RM8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = READ8(ea);
		dst = SBB8(dst, src, I.CF);
		WRITE8(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(sbb_r8_rm8)(void)
{
	UINT8 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM8(modrm);
		dst = LOAD_REG8(modrm);
		dst = SBB8(dst, src, I.CF);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ8(ea);
		dst = LOAD_REG8(modrm);
		dst = SBB8(dst, src, I.CF);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(sbb_al_i8)(void)
{
	UINT8 src, dst;
	src = FETCH();
	dst = REG8(AL);
	dst = SBB8(dst, src, I.CF);
	REG8(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(scasb)(void)
{
	UINT32 eas;
	UINT8 src, dst;
	eas = i386_translate(ES, I.address_size ? REG32(EDI) : REG16(DI) );
	src = READ8(eas);
	dst = REG8(AL);
	SUB8(dst, src);
	BUMP_DI(1);
	CYCLES(CYCLES_SCAS);
}

static void I386OP(setalc)(void)
{
	if( I.CF ) {
		REG8(AL) = 0xff;
	} else {
		REG8(AL) = 0;
	}
	CYCLES(3);
}

static void I386OP(seta_rm8)(void)
{
	UINT8 modrm = FETCH();
	UINT8 value = 0;
	if( I.CF == 0 && I.ZF == 0 ) {
		value = 1;
	}
	if( modrm >= 0xc0 ) {
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_SETCC_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE8(ea, value);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

static void I386OP(setbe_rm8)(void)
{
	UINT8 modrm = FETCH();
	UINT8 value = 0;
	if( I.CF != 0 || I.ZF != 0 ) {
		value = 1;
	}
	if( modrm >= 0xc0 ) {
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_SETCC_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE8(ea, value);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

static void I386OP(setc_rm8)(void)
{
	UINT8 modrm = FETCH();
	UINT8 value = 0;
	if( I.CF != 0 ) {
		value = 1;
	}
	if( modrm >= 0xc0 ) {
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_SETCC_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE8(ea, value);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

static void I386OP(setg_rm8)(void)
{
	UINT8 modrm = FETCH();
	UINT8 value = 0;
	if( I.ZF == 0 && (I.SF == I.OF) ) {
		value = 1;
	}
	if( modrm >= 0xc0 ) {
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_SETCC_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE8(ea, value);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

static void I386OP(setge_rm8)(void)
{
	UINT8 modrm = FETCH();
	UINT8 value = 0;
	if( (I.SF == I.OF) ) {
		value = 1;
	}
	if( modrm >= 0xc0 ) {
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_SETCC_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE8(ea, value);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

static void I386OP(setl_rm8)(void)
{
	UINT8 modrm = FETCH();
	UINT8 value = 0;
	if( I.SF != I.OF ) {
		value = 1;
	}
	if( modrm >= 0xc0 ) {
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_SETCC_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE8(ea, value);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

static void I386OP(setle_rm8)(void)
{
	UINT8 modrm = FETCH();
	UINT8 value = 0;
	if( I.ZF != 0 || (I.SF != I.OF) ) {
		value = 1;
	}
	if( modrm >= 0xc0 ) {
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_SETCC_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE8(ea, value);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

static void I386OP(setnc_rm8)(void)
{
	UINT8 modrm = FETCH();
	UINT8 value = 0;
	if( I.CF == 0 ) {
		value = 1;
	}
	if( modrm >= 0xc0 ) {
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_SETCC_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE8(ea, value);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

static void I386OP(setno_rm8)(void)
{
	UINT8 modrm = FETCH();
	UINT8 value = 0;
	if( I.OF == 0 ) {
		value = 1;
	}
	if( modrm >= 0xc0 ) {
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_SETCC_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE8(ea, value);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

static void I386OP(setnp_rm8)(void)
{
	UINT8 modrm = FETCH();
	UINT8 value = 0;
	if( I.PF == 0 ) {
		value = 1;
	}
	if( modrm >= 0xc0 ) {
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_SETCC_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE8(ea, value);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

static void I386OP(setns_rm8)(void)
{
	UINT8 modrm = FETCH();
	UINT8 value = 0;
	if( I.SF == 0 ) {
		value = 1;
	}
	if( modrm >= 0xc0 ) {
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_SETCC_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE8(ea, value);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

static void I386OP(setnz_rm8)(void)
{
	UINT8 modrm = FETCH();
	UINT8 value = 0;
	if( I.ZF == 0 ) {
		value = 1;
	}
	if( modrm >= 0xc0 ) {
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_SETCC_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE8(ea, value);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

static void I386OP(seto_rm8)(void)
{
	UINT8 modrm = FETCH();
	UINT8 value = 0;
	if( I.OF != 0 ) {
		value = 1;
	}
	if( modrm >= 0xc0 ) {
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_SETCC_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE8(ea, value);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

static void I386OP(setp_rm8)(void)
{
	UINT8 modrm = FETCH();
	UINT8 value = 0;
	if( I.PF != 0 ) {
		value = 1;
	}
	if( modrm >= 0xc0 ) {
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_SETCC_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE8(ea, value);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

static void I386OP(sets_rm8)(void)
{
	UINT8 modrm = FETCH();
	UINT8 value = 0;
	if( I.SF != 0 ) {
		value = 1;
	}
	if( modrm >= 0xc0 ) {
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_SETCC_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE8(ea, value);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

static void I386OP(setz_rm8)(void)
{
	UINT8 modrm = FETCH();
	UINT8 value = 0;
	if( I.ZF != 0 ) {
		value = 1;
	}
	if( modrm >= 0xc0 ) {
		STORE_RM8(modrm, value);
		CYCLES(CYCLES_SETCC_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		WRITE8(ea, value);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

static void I386OP(stc)(void)
{
	I.CF = 1;
	CYCLES(CYCLES_STC);
}

static void I386OP(std)(void)
{
	I.DF = 1;
	CYCLES(CYCLES_STD);
}

static void I386OP(sti)(void)
{
	I.delayed_interrupt_enable = 1;
	CYCLES(CYCLES_STI);
}

static void I386OP(stosb)(void)
{
	UINT32 ead;
	ead = i386_translate(ES, I.address_size ? REG32(EDI) : REG16(DI) );
	WRITE8(ead, REG8(AL));
	BUMP_DI(1);
	CYCLES(CYCLES_STOS);
}

static void I386OP(sub_rm8_r8)(void)
{
	UINT8 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		dst = SUB8(dst, src);
		STORE_RM8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = READ8(ea);
		dst = SUB8(dst, src);
		WRITE8(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(sub_r8_rm8)(void)
{
	UINT8 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM8(modrm);
		dst = LOAD_REG8(modrm);
		dst = SUB8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ8(ea);
		dst = LOAD_REG8(modrm);
		dst = SUB8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(sub_al_i8)(void)
{
	UINT8 src, dst;
	src = FETCH();
	dst = REG8(EAX);
	dst = SUB8(dst, src);
	REG8(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(test_al_i8)(void)
{
	UINT8 src = FETCH();
	UINT8 dst = REG8(AL);
	dst = src & dst;
	SetSZPF8(dst);
	I.CF = 0;
	I.OF = 0;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

static void I386OP(test_rm8_r8)(void)
{
	UINT8 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		dst = src & dst;
		SetSZPF8(dst);
		I.CF = 0;
		I.OF = 0;
		CYCLES(CYCLES_TEST_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = READ8(ea);
		dst = src & dst;
		SetSZPF8(dst);
		I.CF = 0;
		I.OF = 0;
		CYCLES(CYCLES_TEST_REG_MEM);
	}
}

static void I386OP(xchg_r8_rm8)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT8 src = LOAD_RM8(modrm);
		UINT8 dst = LOAD_REG8(modrm);
		STORE_REG8(modrm, src);
		STORE_RM8(modrm, dst);
		CYCLES(CYCLES_XCHG_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT8 src = READ8(ea);
		UINT8 dst = LOAD_REG8(modrm);
		STORE_REG8(modrm, src);
		WRITE8(ea, dst);
		CYCLES(CYCLES_XCHG_REG_MEM);
	}
}

static void I386OP(xor_rm8_r8)(void)
{
	UINT8 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		dst = XOR8(dst, src);
		STORE_RM8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = READ8(ea);
		dst = XOR8(dst, src);
		WRITE8(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

static void I386OP(xor_r8_rm8)(void)
{
	UINT32 src, dst;
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		src = LOAD_RM8(modrm);
		dst = LOAD_REG8(modrm);
		dst = XOR8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		src = READ8(ea);
		dst = LOAD_REG8(modrm);
		dst = XOR8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

static void I386OP(xor_al_i8)(void)
{
	UINT8 src, dst;
	src = FETCH();
	dst = REG8(AL);
	dst = XOR8(dst, src);
	REG8(AL) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}


static void I386OP(group80_8)(void)
{
	UINT32 ea;
	UINT8 src, dst;
	UINT8 modrm = FETCH();

	switch( (modrm >> 3) & 0x7 )
	{
		case 0:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM8(modrm);
				src = FETCH();
				dst = ADD8(dst, src);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ8(ea);
				src = FETCH();
				dst = ADD8(dst, src);
				WRITE8(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 1:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM8(modrm);
				src = FETCH();
				dst = OR8(dst, src);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ8(ea);
				src = FETCH();
				dst = OR8(dst, src);
				WRITE8(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 2:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM8(modrm);
				src = FETCH();
				dst = ADC8(dst, src, I.CF);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ8(ea);
				src = FETCH();
				dst = ADC8(dst, src, I.CF);
				WRITE8(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 3:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM8(modrm);
				src = FETCH();
				dst = SBB8(dst, src, I.CF);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ8(ea);
				src = FETCH();
				dst = SBB8(dst, src, I.CF);
				WRITE8(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 4:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM8(modrm);
				src = FETCH();
				dst = AND8(dst, src);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ8(ea);
				src = FETCH();
				dst = AND8(dst, src);
				WRITE8(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 5:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM8(modrm);
				src = FETCH();
				dst = SUB8(dst, src);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ8(ea);
				src = FETCH();
				dst = SUB8(dst, src);
				WRITE8(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 6:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM8(modrm);
				src = FETCH();
				dst = XOR8(dst, src);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ8(ea);
				src = FETCH();
				dst = XOR8(dst, src);
				WRITE8(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 7:
			if( modrm >= 0xc0 ) {
				dst = LOAD_RM8(modrm);
				src = FETCH();
				SUB8(dst, src);
				CYCLES(CYCLES_CMP_REG_REG);
			} else {
				ea = GetEA(modrm);
				dst = READ8(ea);
				src = FETCH();
				SUB8(dst, src);
				CYCLES(CYCLES_CMP_REG_MEM);
			}
			break;
	}
}

static void I386OP(groupC0_8)(void)
{
	UINT8 dst;
	UINT8 modrm = FETCH();
	UINT8 shift;

	if( modrm >= 0xc0 ) {
		dst = LOAD_RM8(modrm);
		shift = FETCH() & 0x1f;
		dst = i386_shift_rotate8(modrm, dst, shift);
		STORE_RM8(modrm, dst);
	} else {
		UINT32 ea = GetEA(modrm);
		dst = READ8(ea);
		shift = FETCH() & 0x1f;
		dst = i386_shift_rotate8(modrm, dst, shift);
		WRITE8(ea, dst);
	}
}

static void I386OP(groupD0_8)(void)
{
	UINT8 dst;
	UINT8 modrm = FETCH();

	if( modrm >= 0xc0 ) {
		dst = LOAD_RM8(modrm);
		dst = i386_shift_rotate8(modrm, dst, 1);
		STORE_RM8(modrm, dst);
	} else {
		UINT32 ea = GetEA(modrm);
		dst = READ8(ea);
		dst = i386_shift_rotate8(modrm, dst, 1);
		WRITE8(ea, dst);
	}
}

static void I386OP(groupD2_8)(void)
{
	UINT8 dst;
	UINT8 modrm = FETCH();

	if( modrm >= 0xc0 ) {
		dst = LOAD_RM8(modrm);
		dst = i386_shift_rotate8(modrm, dst, REG8(CL));
		STORE_RM8(modrm, dst);
	} else {
		UINT32 ea = GetEA(modrm);
		dst = READ8(ea);
		dst = i386_shift_rotate8(modrm, dst, REG8(CL));
		WRITE8(ea, dst);
	}
}

static void I386OP(groupF6_8)(void)
{
	UINT8 modrm = FETCH();

	switch( (modrm >> 3) & 0x7 )
	{
		case 0:
			if( modrm >= 0xc0 ) {
				UINT8 dst = LOAD_RM8(modrm);
				UINT8 src = FETCH();
				dst &= src;
				I.CF = I.OF = I.AF = 0;
				SetSZPF8(dst);
				CYCLES(CYCLES_TEST_IMM_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT8 dst = READ8(ea);
				UINT8 src = FETCH();
				dst &= src;
				I.CF = I.OF = I.AF = 0;
				SetSZPF8(dst);
				CYCLES(CYCLES_TEST_IMM_MEM);
			}
			break;
		case 2:
			if( modrm >= 0xc0 ) {
				UINT8 dst = LOAD_RM8(modrm);
				dst = ~dst;
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_NOT_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT8 dst = READ8(ea);
				dst = ~dst;
				WRITE8(ea, dst);
				CYCLES(CYCLES_NOT_MEM);
			}
			break;
		case 3:
			if( modrm >= 0xc0 ) {
				UINT8 dst = LOAD_RM8(modrm);
				dst = SUB8(0, dst );
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_NEG_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT8 dst = READ8(ea);
				dst = SUB8(0, dst );
				WRITE8(ea, dst);
				CYCLES(CYCLES_NEG_MEM);
			}
			break;
		case 4:
			{
				UINT16 result;
				UINT8 src, dst;
				if( modrm >= 0xc0 ) {
					src = LOAD_RM8(modrm);
					CYCLES(CYCLES_MUL8_ACC_REG);
				} else {
					UINT32 ea = GetEA(modrm);
					src = READ8(ea);
					CYCLES(CYCLES_MUL8_ACC_MEM);
				}

				dst = REG8(AL);
				result = (UINT16)src * (UINT16)dst;
				REG16(AX) = (UINT16)result;

				I.CF = I.OF = (REG16(AX) > 0xff);
			}
			break;
		case 5:
			{
				INT16 result;
				INT16 src, dst;
				if( modrm >= 0xc0 ) {
					src = (INT16)(INT8)LOAD_RM8(modrm);
					CYCLES(CYCLES_IMUL8_ACC_REG);
				} else {
					UINT32 ea = GetEA(modrm);
					src = (INT16)(INT8)READ8(ea);
					CYCLES(CYCLES_IMUL8_ACC_MEM);
				}

				dst = (INT16)(INT8)REG8(AL);
				result = src * dst;

				REG16(AX) = (UINT16)result;

				I.CF = I.OF = !(result == (INT16)(INT8)result);
			}
			break;
		case 6:
			{
				UINT16 quotient, remainder, result;
				UINT8 src;
				if( modrm >= 0xc0 ) {
					src = LOAD_RM8(modrm);
					CYCLES(CYCLES_DIV8_ACC_REG);
				} else {
					UINT32 ea = GetEA(modrm);
					src = READ8(ea);
					CYCLES(CYCLES_DIV8_ACC_MEM);
				}

				quotient = (UINT16)REG16(AX);
				if( src ) {
					remainder = quotient % (UINT16)src;
					result = quotient / (UINT16)src;
					if( result > 0xff ) {

					} else {
						REG8(AH) = (UINT8)remainder & 0xff;
						REG8(AL) = (UINT8)result & 0xff;

						if (I.cpuid_id0 != 0x69727943)
							I.CF = 1;
					}
				} else {

				}
			}
			break;
		case 7:
			{
				INT16 quotient, remainder, result;
				UINT8 src;
				if( modrm >= 0xc0 ) {
					src = LOAD_RM8(modrm);
					CYCLES(CYCLES_IDIV8_ACC_REG);
				} else {
					UINT32 ea = GetEA(modrm);
					src = READ8(ea);
					CYCLES(CYCLES_IDIV8_ACC_MEM);
				}

				quotient = (INT16)REG16(AX);
				if( src ) {
					remainder = quotient % (INT16)(INT8)src;
					result = quotient / (INT16)(INT8)src;
					if( result > 0xff ) {

					} else {
						REG8(AH) = (UINT8)remainder & 0xff;
						REG8(AL) = (UINT8)result & 0xff;

						if (I.cpuid_id0 != 0x69727943)
							I.CF = 1;
					}
				} else {

				}
			}
			break;
	}
}

static void I386OP(groupFE_8)(void)
{
	UINT8 modrm = FETCH();

	switch( (modrm >> 3) & 0x7 )
	{
		case 0:
			if( modrm >= 0xc0 ) {
				UINT8 dst = LOAD_RM8(modrm);
				dst = INC8(dst);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_INC_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT8 dst = READ8(ea);
				dst = INC8(dst);
				WRITE8(ea, dst);
				CYCLES(CYCLES_INC_MEM);
			}
			break;
		case 1:
			if( modrm >= 0xc0 ) {
				UINT8 dst = LOAD_RM8(modrm);
				dst = DEC8(dst);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_DEC_REG);
			} else {
				UINT32 ea = GetEA(modrm);
				UINT8 dst = READ8(ea);
				dst = DEC8(dst);
				WRITE8(ea, dst);
				CYCLES(CYCLES_DEC_MEM);
			}
			break;
		case 6:
			{
				UINT8 value;
				if( modrm >= 0xc0 ) {
					value = LOAD_RM8(modrm);
				} else {
					UINT32 ea = GetEA(modrm);
					value = READ8(ea);
				}
				if( I.operand_size ) {
					PUSH32(value);
				} else {
					PUSH16(value);
				}
				CYCLES(CYCLES_PUSH_RM);
			}
			break;
		default:
			fatalerror("i386: groupFE_8 /%d unimplemented", (modrm >> 3) & 0x7);
			break;
	}
}


static void I386OP(segment_CS)(void)
{
	I.segment_prefix = 1;
	I.segment_override = CS;

	I386OP(decode_opcode)();
}

static void I386OP(segment_DS)(void)
{
	I.segment_prefix = 1;
	I.segment_override = DS;
	CYCLES(0);
	I386OP(decode_opcode)();
}

static void I386OP(segment_ES)(void)
{
	I.segment_prefix = 1;
	I.segment_override = ES;
	CYCLES(0);
	I386OP(decode_opcode)();
}

static void I386OP(segment_FS)(void)
{
	I.segment_prefix = 1;
	I.segment_override = FS;
	CYCLES(1);
	I386OP(decode_opcode)();
}

static void I386OP(segment_GS)(void)
{
	I.segment_prefix = 1;
	I.segment_override = GS;
	CYCLES(1);
	I386OP(decode_opcode)();
}

static void I386OP(segment_SS)(void)
{
	I.segment_prefix = 1;
	I.segment_override = SS;
	CYCLES(0);
	I386OP(decode_opcode)();
}

static void I386OP(operand_size)(void)
{
	I.operand_size ^= 1;
	I386OP(decode_opcode)();
}

static void I386OP(address_size)(void)
{
	I.address_size ^= 1;
	I386OP(decode_opcode)();
}

static void I386OP(nop)(void)
{
	CYCLES(CYCLES_NOP);
}

static void I386OP(int3)(void)
{
	CYCLES(CYCLES_INT3);
	i386_trap(3, 1, 0);
}

static void I386OP(int)(void)
{
	int interrupt = FETCH();
	CYCLES(CYCLES_INT);
	i386_trap(interrupt, 1, 0);
}

static void I386OP(into)(void)
{
	if( I.OF ) {
		i386_trap(4, 1, 0);
		CYCLES(CYCLES_INTO_OF1);
	}
	else
	{
		CYCLES(CYCLES_INTO_OF0);
	}
}

static UINT32 i386_escape_ea;
static void I386OP(escape)(void)
{
	UINT8 modrm = FETCH();
	if(modrm < 0xc0)
	{
		i386_escape_ea = GetEA(modrm);
	}
	CYCLES(3);
	(void) LOAD_RM8(modrm);
}

static void I386OP(hlt)(void)
{

	I.halted = 1;
	CYCLES(CYCLES_HLT);
	if (I.cycles > 0)
		I.cycles = 0;
}

static void I386OP(decimal_adjust)(int direction)
{
	UINT8 tmpAL = REG8(AL);
	UINT8 tmpCF = I.CF;

	if (I.AF || ((REG8(AL) & 0xf) > 9))
	{
		UINT16 t= (UINT16)REG8(AL) + (direction * 0x06);
		REG8(AL) = (UINT8)t&0xff;
		I.AF = 1;
		if (t & 0x100)
			I.CF = 1;
		if (direction > 0)
			tmpAL = REG8(AL);
	}

	if (tmpCF || (tmpAL > 0x99))
	{
		REG8(AL) += (direction * 0x60);
		I.CF = 1;
	}

	SetSZPF8(REG8(AL));
}

static void I386OP(daa)(void)
{
	I386OP(decimal_adjust)(+1);
	CYCLES(CYCLES_DAA);
}

static void I386OP(das)(void)
{
	I386OP(decimal_adjust)(-1);
	CYCLES(CYCLES_DAS);
}

static void I386OP(aaa)(void)
{
	if( ( (REG8(AL) & 0x0f) > 9) || (I.AF != 0) ) {
		REG16(AX) = REG16(AX) + 6;
		REG8(AH) = REG8(AH) + 1;
		I.AF = 1;
		I.CF = 1;
	} else {
		I.AF = 0;
		I.CF = 0;
	}
	REG8(AL) = REG8(AL) & 0x0f;
	CYCLES(CYCLES_AAA);
}

static void I386OP(aas)(void)
{
	if (I.AF || ((REG8(AL) & 0xf) > 9))
    {
		REG16(AX) -= 6;
		REG8(AH) -= 1;
		I.AF = 1;
		I.CF = 1;
    }
	else
	{
		I.AF = 0;
		I.CF = 0;
    }
	REG8(AL) &= 0x0f;
	CYCLES(CYCLES_AAS);
}

static void I386OP(aad)(void)
{
	UINT8 tempAL = REG8(AL);
	UINT8 tempAH = REG8(AH);
	UINT8 i = FETCH();

	REG8(AL) = (tempAL + (tempAH * i)) & 0xff;
	REG8(AH) = 0;
	SetSZPF8( REG8(AL) );
	CYCLES(CYCLES_AAD);
}

static void I386OP(aam)(void)
{
	UINT8 tempAL = REG8(AL);
	UINT8 i = FETCH();

	REG8(AH) = tempAL / i;
	REG8(AL) = tempAL % i;
	SetSZPF8( REG8(AL) );
	CYCLES(CYCLES_AAM);
}

static void I386OP(clts)(void)
{

	if(PROTECTED_MODE && I.CPL != 0)
		FAULT(FAULT_GP,0)
	I.cr[0] &= ~0x08;
	CYCLES(CYCLES_CLTS);
}

static void I386OP(wait)(void)
{

}

static void I386OP(lock)(void)
{
	CYCLES(CYCLES_LOCK);
	I386OP(decode_opcode)();
}

static void I386OP(mov_r32_tr)(void)
{
	FETCH();
	CYCLES(1);
}

static void I386OP(mov_tr_r32)(void)
{
	FETCH();
	CYCLES(1);
}

static void I386OP(loadall)(void)
{
	ui_popup("LOADALL instruction hit!");
	CYCLES(1);
}

static void I386OP(unimplemented)(void)
{
	fatalerror("i386: Unimplemented opcode %02X at %08X", I.opcode, I.pc - 1 );
}

static void I386OP(invalid)(void)
{
	logerror("i386: Invalid opcode %02X at %08X\n", I.opcode, I.pc - 1);
	i386_trap(6, 0, 0);
}

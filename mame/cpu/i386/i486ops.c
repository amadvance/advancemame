// license:BSD-3-Clause
// copyright-holders:Ville Linde, Barry Rodewald
static void I486OP(cpuid)(void)
{
	if (I.cpuid_id0 == 0)
	{

		logerror("CPUID not supported at %08x!\n", I.eip);
		i386_trap(6, 0, 0);
	}
	else
	{
		switch (REG32(EAX))
		{
			case 0:
			{
				REG32(EAX) = I.cpuid_max_input_value_eax;
				REG32(EBX) = I.cpuid_id0;
				REG32(ECX) = I.cpuid_id2;
				REG32(EDX) = I.cpuid_id1;
				CYCLES(CYCLES_CPUID);
				break;
			}

			case 1:
			{
				REG32(EAX) = I.cpu_version;
				REG32(EDX) = I.feature_flags;
				CYCLES(CYCLES_CPUID_EAX1);
				break;
			}
		}
	}
}

static void I486OP(invd)(void)
{

	CYCLES(CYCLES_INVD);
}

static void I486OP(wbinvd)(void)
{

}

static void I486OP(cmpxchg_rm8_r8)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT8 dst = LOAD_RM8(modrm);
		UINT8 src = LOAD_REG8(modrm);

		if( REG8(AL) == dst ) {
			STORE_RM8(modrm, src);
			I.ZF = 1;
			CYCLES(CYCLES_CMPXCHG_REG_REG_T);
		} else {
			REG8(AL) = dst;
			I.ZF = 0;
			CYCLES(CYCLES_CMPXCHG_REG_REG_F);
		}
	} else {
		UINT32 ea = GetEA(modrm);
		UINT8 dst = READ8(ea);
		UINT8 src = LOAD_REG8(modrm);

		if( REG8(AL) == dst ) {
			WRITE8(modrm, src);
			I.ZF = 1;
			CYCLES(CYCLES_CMPXCHG_REG_MEM_T);
		} else {
			REG8(AL) = dst;
			I.ZF = 0;
			CYCLES(CYCLES_CMPXCHG_REG_MEM_F);
		}
	}
}

static void I486OP(cmpxchg_rm16_r16)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT16 dst = LOAD_RM16(modrm);
		UINT16 src = LOAD_REG16(modrm);

		if( REG16(AX) == dst ) {
			STORE_RM16(modrm, src);
			I.ZF = 1;
			CYCLES(CYCLES_CMPXCHG_REG_REG_T);
		} else {
			REG16(AX) = dst;
			I.ZF = 0;
			CYCLES(CYCLES_CMPXCHG_REG_REG_F);
		}
	} else {
		UINT32 ea = GetEA(modrm);
		UINT16 dst = READ16(ea);
		UINT16 src = LOAD_REG16(modrm);

		if( REG16(AX) == dst ) {
			WRITE16(modrm, src);
			I.ZF = 1;
			CYCLES(CYCLES_CMPXCHG_REG_MEM_T);
		} else {
			REG16(AX) = dst;
			I.ZF = 0;
			CYCLES(CYCLES_CMPXCHG_REG_MEM_F);
		}
	}
}

static void I486OP(cmpxchg_rm32_r32)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT32 dst = LOAD_RM32(modrm);
		UINT32 src = LOAD_REG32(modrm);

		if( REG32(EAX) == dst ) {
			STORE_RM32(modrm, src);
			I.ZF = 1;
			CYCLES(CYCLES_CMPXCHG_REG_REG_T);
		} else {
			REG32(EAX) = dst;
			I.ZF = 0;
			CYCLES(CYCLES_CMPXCHG_REG_REG_F);
		}
	} else {
		UINT32 ea = GetEA(modrm);
		UINT32 dst = READ32(ea);
		UINT32 src = LOAD_REG32(modrm);

		if( REG32(EAX) == dst ) {
			WRITE32(ea, src);
			I.ZF = 1;
			CYCLES(CYCLES_CMPXCHG_REG_MEM_T);
		} else {
			REG32(EAX) = dst;
			I.ZF = 0;
			CYCLES(CYCLES_CMPXCHG_REG_MEM_F);
		}
	}
}

static void I486OP(xadd_rm8_r8)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT8 dst = LOAD_RM8(modrm);
		UINT8 src = LOAD_REG8(modrm);
		STORE_RM16(modrm, dst + src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_XADD_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT8 dst = READ8(ea);
		UINT8 src = LOAD_REG8(modrm);
		WRITE8(ea, dst + src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_XADD_REG_MEM);
	}
}

static void I486OP(xadd_rm16_r16)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT16 dst = LOAD_RM16(modrm);
		UINT16 src = LOAD_REG16(modrm);
		STORE_RM16(modrm, dst + src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_XADD_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT16 dst = READ16(ea);
		UINT16 src = LOAD_REG16(modrm);
		WRITE16(ea, dst + src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_XADD_REG_MEM);
	}
}

static void I486OP(xadd_rm32_r32)(void)
{
	UINT8 modrm = FETCH();
	if( modrm >= 0xc0 ) {
		UINT32 dst = LOAD_RM32(modrm);
		UINT32 src = LOAD_REG32(modrm);
		STORE_RM32(modrm, dst + src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_XADD_REG_REG);
	} else {
		UINT32 ea = GetEA(modrm);
		UINT32 dst = READ32(ea);
		UINT32 src = LOAD_REG32(modrm);
		WRITE32(ea, dst + src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_XADD_REG_MEM);
	}
}

static void I486OP(group0F01_16)(void)
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
					ea = i386_translate( CS, address );
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
					ea = i386_translate( CS, address );
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
					ea = i386_translate( CS, address );
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
					ea = i386_translate( CS, address );
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

				UINT8 b;
				if( modrm >= 0xc0 ) {
					b = LOAD_RM8(modrm);
					CYCLES(CYCLES_LMSW_REG);
				} else {
					ea = GetEA(modrm);
					CYCLES(CYCLES_LMSW_MEM);
				b = READ8(ea);
				}
				I.cr[0] &= ~0x03;
				I.cr[0] |= b & 0x03;
				break;
			}
		case 7:
			{

				break;
			}
		default:
			fatalerror("i486: unimplemented opcode 0x0f 01 /%d at %08X", (modrm >> 3) & 0x7, I.eip - 2);
			break;
	}
}

static void I486OP(group0F01_32)(void)
{
	UINT8 modrm = FETCH();
	UINT32 address, ea;

	switch( (modrm >> 3) & 0x7 )
	{
		case 0:
			{
				if( modrm >= 0xc0 ) {
					address = LOAD_RM32(modrm);
					ea = i386_translate( CS, address );
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
					ea = i386_translate( CS, address );
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
					ea = i386_translate( CS, address );
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
					ea = i386_translate( CS, address );
				} else {
					ea = GetEA(modrm);
				}
				I.idtr.limit = READ16(ea);
				I.idtr.base = READ32(ea + 2);
				CYCLES(CYCLES_LIDT);
				break;
			}
		case 7:
			{

				break;
			}
		default:
			fatalerror("i486: unimplemented opcode 0x0f 01 /%d at %08X", (modrm >> 3) & 0x7, I.eip - 2);
			break;
	}
}

static void I486OP(bswap_eax)(void)
{
	REG32(EAX) = SWITCH_ENDIAN_32(REG32(EAX));
	CYCLES(1);
}

static void I486OP(bswap_ecx)(void)
{
	REG32(ECX) = SWITCH_ENDIAN_32(REG32(ECX));
	CYCLES(1);
}

static void I486OP(bswap_edx)(void)
{
	REG32(EDX) = SWITCH_ENDIAN_32(REG32(EDX));
	CYCLES(1);
}

static void I486OP(bswap_ebx)(void)
{
	REG32(EBX) = SWITCH_ENDIAN_32(REG32(EBX));
	CYCLES(1);
}

static void I486OP(bswap_esp)(void)
{
	REG32(ESP) = SWITCH_ENDIAN_32(REG32(ESP));
	CYCLES(1);
}

static void I486OP(bswap_ebp)(void)
{
	REG32(EBP) = SWITCH_ENDIAN_32(REG32(EBP));
	CYCLES(1);
}

static void I486OP(bswap_esi)(void)
{
	REG32(ESI) = SWITCH_ENDIAN_32(REG32(ESI));
	CYCLES(1);
}

static void I486OP(bswap_edi)(void)
{
	REG32(EDI) = SWITCH_ENDIAN_32(REG32(EDI));
	CYCLES(1);
}

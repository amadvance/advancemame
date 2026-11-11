// license:BSD-3-Clause
// copyright-holders:Ville Linde, Barry Rodewald
#include <math.h>

#define ST(x)	(I.fpu_reg[(I.fpu_top + (x)) & 7])
#define FPU_INFINITY_DOUBLE		U64(0x7ff0000000000000)
#define FPU_INFINITY_SINGLE		(0x7f800000)
#define FPU_SIGN_BIT_DOUBLE		U64(0x8000000000000000)
#define FPU_SIGN_BIT_SINGLE		(0x80000000)

#define FPU_MASK_INVALID_OP			0x0001
#define FPU_MASK_DENORMAL_OP		0x0002
#define FPU_MASK_ZERO_DIVIDE		0x0004
#define FPU_MASK_OVERFLOW			0x0008
#define FPU_MASK_UNDERFLOW			0x0010
#define FPU_MASK_PRECISION			0x0020

#define FPU_BUSY					0x8000
#define FPU_C3						0x4000
#define FPU_STACK_TOP_MASK			0x3800
#define FPU_C2						0x0400
#define FPU_C1						0x0200
#define FPU_C0						0x0100
#define FPU_ERROR_SUMMARY			0x0080
#define FPU_STACK_FAULT				0x0040
#define FPU_EXCEPTION_PRECISION		0x0020
#define FPU_EXCEPTION_UNDERFLOW		0x0010
#define FPU_EXCEPTION_OVERFLOW		0x0008
#define FPU_EXCEPTION_ZERO_DIVIDE	0x0004
#define FPU_EXCEPTION_DENORMAL_OP	0x0002
#define FPU_EXCEPTION_INVALID_OP	0x0001

INLINE void FPU_PUSH(X87_REG value)
{
	I.fpu_top--;
	if (I.fpu_top < 0)
	{
		I.fpu_top = 7;
	}

	I.fpu_reg[I.fpu_top] = value;
}

INLINE X87_REG FPU_POP(void)
{
	X87_REG value = I.fpu_reg[I.fpu_top];

	I.fpu_tag_word |= 3 << (I.fpu_top * 2);

	I.fpu_top++;
	if (I.fpu_top > 7)
	{
		I.fpu_top = 0;
	}

	return value;
}

static void I386OP(fpu_group_d8)(void)
{
	UINT8 modrm = FETCH();
	if (modrm < 0xc0)
	{
		UINT32 ea = GetEA(modrm);

		switch ((modrm >> 3) & 0x7)
		{
		case 6:
			UINT32 src = READ32(ea);
			if(src == 0)
				fatalerror("FPU: Unimplemented Divide-by-zero exception at %08X.\n", I.pc-2);
			ST(0).f = ST(0).f / src;
			CYCLES(1);
			break;
		}
	}
	else
	{
		fatalerror("I386: FPU Op D8 %02X at %08X", modrm, I.pc-2);
	}
}

static void I386OP(fpu_group_d9)(void)
{
	UINT8 modrm = FETCH();

	if (modrm < 0xc0)
	{
		UINT32 ea = GetEA(modrm);

		switch ((modrm >> 3) & 0x7)
		{
			case 5:
			{
				I.fpu_control_word = READ16(ea);
				CYCLES(1);
				break;
			}

			case 6:
			{
				WRITE16(ea, I.fpu_control_word);
				WRITE16(ea+2, I.fpu_status_word);
				WRITE16(ea+4, I.fpu_tag_word);
				WRITE16(ea+6, I.fpu_inst_ptr & 0xffff);
				WRITE16(ea+8, (I.fpu_opcode & 0x07ff) | ((I.fpu_inst_ptr & 0x0f0000) >> 4));
				WRITE16(ea+10, I.fpu_data_ptr & 0xffff);
				WRITE16(ea+12, ((I.fpu_inst_ptr & 0x0f0000) >> 4));
				CYCLES(1);
				break;
			}

			case 7:
			{
				WRITE16(ea, I.fpu_control_word);
				CYCLES(1);
				break;
			}

			default:
				fatalerror("I386: FPU Op D9 %02X at %08X", modrm, I.pc-2);
		}
	}
	else
	{
		switch (modrm & 0x3f)
		{
			case 0x10:
				CYCLES(1);
				break;


			case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
			{
				X87_REG t = ST(modrm & 7);
				FPU_PUSH(t);
				CYCLES(1);
				break;
			}

			case 0x20:
			{
				ST(0).i ^= FPU_SIGN_BIT_DOUBLE;
				CYCLES(1);
				break;
			}

			case 0x28:
			{
				X87_REG t;
				t.f = 1.0;
				FPU_PUSH(t);
				CYCLES(1);
				break;
			}

			case 0x2e:
			{
				X87_REG t;
				t.f = 0.0;
				FPU_PUSH(t);
				CYCLES(1);
				break;
			}
			default:
				fatalerror("I386: FPU Op D9 %02X at %08X", modrm, I.pc-2);
		}
	}
}

static void I386OP(fpu_group_da)(void)
{
	UINT8 modrm = FETCH();
	fatalerror("I386: FPU Op DA %02X at %08X", modrm, I.pc-2);
}

static void I386OP(fpu_group_db)(void)
{
	UINT8 modrm = FETCH();

	if (modrm < 0xc0)
	{
		fatalerror("I386: FPU Op DB %02X at %08X", modrm, I.pc-2);
	}
	else
	{
		switch (modrm & 0x3f)
		{
			case 0x22:
			{

				I.fpu_status_word &= ~0x80ff;

				CYCLES(1);
				break;
			}
			case 0x23:
			{
				I.fpu_control_word = 0x37f;
				I.fpu_status_word = 0;
				I.fpu_tag_word = 0xffff;
				I.fpu_data_ptr = 0;
				I.fpu_inst_ptr = 0;
				I.fpu_opcode = 0;

				CYCLES(1);
				break;
			}

			case 0x24:
			{
				CYCLES(1);
				break;
			}

			default:
				fatalerror("I386: FPU Op DB %02X at %08X", modrm, I.pc-2);
		}
	}
}

static void I386OP(fpu_group_dc)(void)
{
	UINT8 modrm = FETCH();

	if (modrm < 0xc0)
	{

		switch ((modrm >> 3) & 0x7)
		{
			case 1:
			{
				X87_REG value;
				value.i = READ64(GetEA(modrm));
				ST(0).f *= value.f;
				CYCLES_NUM(16);
				break;
			}

			default:
				fatalerror("I386: FPU Op DC %02X at %08X", modrm, I.pc-2);
		}
	}
	else
	{
		switch (modrm & 0x3f)
		{
			case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
			{

				if ((ST(modrm & 7).i & U64(0x7fffffffffffffff)) == 0)
				{

					if (I.fpu_control_word & FPU_MASK_ZERO_DIVIDE)
					{
						ST(modrm & 7).i |= FPU_INFINITY_DOUBLE;
					}
				}
				else
				{
					ST(modrm & 7).f = ST(0).f / ST(modrm & 7).f;
				}
				CYCLES(1);
				break;
			}

			default:
				fatalerror("I386: FPU Op DC %02X at %08X", modrm, I.pc-2);
		}
	}
}

static void I386OP(fpu_group_dd)(void)
{
	UINT8 modrm = FETCH();

	if (modrm < 0xc0)
	{
		UINT32 ea = GetEA(modrm);

		switch ((modrm >> 3) & 0x7)
		{
			case 7:
			{
				WRITE16(ea, (I.fpu_status_word & ~FPU_STACK_TOP_MASK) | (I.fpu_top << 10));
				CYCLES(1);
				break;
			}

			default:
				fatalerror("I386: FPU Op DD %02X at %08X", modrm, I.pc-2);
		}
	}
	else
	{
		switch (modrm & 0x3f)
		{
			default:
				fatalerror("I386: FPU Op DD %02X at %08X", modrm, I.pc-2);
		}
	}
}

static void I386OP(fpu_group_de)(void)
{
	UINT8 modrm = FETCH();

	if (modrm < 0xc0)
	{

		switch ((modrm >> 3) & 0x7)
		{
			default:
				fatalerror("I386: FPU Op DE %02X at %08X", modrm, I.pc-2);
		}
	}
	else
	{
		switch (modrm & 0x3f)
		{
			case 0x19:
			{
				I.fpu_status_word &= ~(FPU_C3 | FPU_C2 | FPU_C0);
				if (ST(0).f > ST(1).f)
				{

				}
				else if (ST(0).f < ST(1).f)
				{
					I.fpu_status_word |= FPU_C0;
				}
				else if (ST(0).f == ST(1).f)
				{
					I.fpu_status_word |= FPU_C3;
				}
				else
				{

					I.fpu_status_word |= (FPU_C3 | FPU_C2 | FPU_C0);
				}
				FPU_POP();
				FPU_POP();
				CYCLES(1);
				break;
			}

			case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
			{
				if ((ST(0).i & U64(0x7fffffffffffffff)) == 0)
				{

					if (I.fpu_control_word & FPU_MASK_ZERO_DIVIDE)
					{
						ST(modrm & 7).i |= FPU_INFINITY_DOUBLE;
					}
				}
				else
				{
					ST(modrm & 7).f = ST(modrm & 7).f / ST(0).f;
				}
				FPU_POP();
				CYCLES(1);
				break;
			}

			default:
				fatalerror("I386: FPU Op DE %02X at %08X", modrm, I.pc-2);
		}
	}
}

static void I386OP(fpu_group_df)(void)
{
	UINT8 modrm = FETCH();

	if (modrm < 0xc0)
	{

		switch ((modrm >> 3) & 0x7)
		{
			case 0:
			{
				X87_REG value;
				value.f = (INT16)READ16(GetEA(modrm));
				FPU_PUSH(value);
				CYCLES_NUM(13);
				break;
			}

			case 7:
			{
				UINT32 ea = GetEA(modrm);
				double value = ST(0).f;
				INT64 result;
				switch ((I.fpu_control_word >> 10) & 3)
				{
					case 0: value = rint(value); break;
					case 1: value = floor(value); break;
					case 2: value = ceil(value); break;
					case 3: value = trunc(value); break;
				}
				/* Out-of-range conversions produce integer indefinite, never a host overflow. */
				if (!isfinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
				{
					I.fpu_status_word |= FPU_EXCEPTION_INVALID_OP;
					result = (INT64)U64(0x8000000000000000);
				}
				else result = (INT64)value;
				WRITE64(ea, result);
				FPU_POP();
				CYCLES_NUM(29);
				break;
			}

			default:
				fatalerror("I386: FPU Op DF %02X at %08X", modrm, I.pc-2);
		}
	}
	else
	{
		switch (modrm & 0x3f)
		{
			case 0x20:
			{
				REG16(AX) = (I.fpu_status_word & ~FPU_STACK_TOP_MASK) | (I.fpu_top << 10);
				CYCLES(1);
				break;
			}

			default:
				fatalerror("I386: FPU Op DF %02X at %08X", modrm, I.pc-2);
		}
	}
}

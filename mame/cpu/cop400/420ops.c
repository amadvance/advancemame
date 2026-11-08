// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria, Curt Coder
/* National Semiconductor COP400 */

#define SC				R.SC
#define IL				R.IL

#define PUSH(addr) 		{ SC = SB; SB = SA; SA = addr; }
#define POP() 			{ PC = SA; SA = SB; SB = SC; }

#include "410ops.c"

#define IN_IN()			IN(COP400_PORT_IN)

INSTRUCTION(adt)
{
	A = (A + 10) & 0x0F;
}

INSTRUCTION(casc)
{
	A = (A ^ 0xF) + RAM_R(B) + C;

	if (A > 0xF)
	{
		C = 1;
		skip = 1;
		A &= 0xF;
	}
	else
	{
		C = 0;
	}
}

INSTRUCTION(cop420_ret)
{
	POP();
	skip = R.last_skip;
}

INSTRUCTION(cqma)
{
	RAM_W(B, Q >> 4);
	A = Q & 0xF;
}

INSTRUCTION(ldd)
{
	UINT8 rd = opcode & 0x3f;

	A = RAM_R(rd);
}

INSTRUCTION(xabr)
{
	UINT8 Br = A & 0x03;
	UINT8 Bd = B & 0x0f;

	A = B >> 4;
	B = (Br << 4) + Bd;
}

INSTRUCTION(skt)
{
	if (R.timerlatch)
	{
		R.timerlatch = 0;
		skip = 1;
	}
}

INSTRUCTION(inin)
{
	A = IN_IN();
}

INSTRUCTION(cop402m_inin)
{
	A = IN_IN() | 0x02;
}

INSTRUCTION(inil)
{
	A = (IN_IN() & 0x09) | 0x04;
}

INSTRUCTION(cop421_inil)
{
	A = 0;
}

INSTRUCTION(ogi)
{
	UINT4 y = opcode & 0x0f;

	WRITE_G(y);
}

// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria, Curt Coder
/* National Semiconductor COP400 */

#define INSTRUCTION(mnemonic) INLINE void (mnemonic)(UINT8 opcode)

#define ROM(addr)			cpu_readop((addr) & 0x3ff)
#define RAM_W(addr, value)	(data_write_byte(addr, value))
#define RAM_R(addr)			(data_read_byte(addr))

#define IN(addr)			io_read_byte(addr)
#define OUT(addr, value)	io_write_byte(addr, value)

#define A				R.A
#define B				R.B
#define C				R.C
#define G				R.G
#define Q				R.Q
#define EN				R.EN
#define SA				R.SA
#define SB				R.SB
#define SIO				R.SIO
#define SKL				R.SKL
#define PC				R.PC
#define prevPC			R.PREVPC
#define skip			R.skip
#define skipLBI			R.skipLBI

#define IN_G()			IN(COP400_PORT_G)
#define IN_L()			IN(COP400_PORT_L)
#define IN_SI()			IN(COP400_PORT_SIO)
#define OUT_G(A)		OUT(COP400_PORT_G, (A) & R.G_mask)
#define OUT_L(A)		OUT(COP400_PORT_L, (A))
#define OUT_D(A)		OUT(COP400_PORT_D, (A) & R.D_mask)
#define OUT_SK(A)		OUT(COP400_PORT_SK, (A))
#define OUT_SO(A)		OUT(COP400_PORT_SIO, (A))

#ifndef PUSH
#define PUSH(addr) 		{ SB = SA; SA = addr; }
#define POP() 			{ PC = SA; SA = SB; }
#endif

static void cop410_serial_tick(void)
{
	if (BIT(EN, 0))
	{
		int si = IN_SI();

		if (R.last_si && !si)
		{
			SIO--;
			SIO &= 0x0f;
		}

		R.last_si = si;

		OUT_SO(BIT(EN, 3));

		OUT_SK(SKL);
	}
	else
	{
		if (BIT(EN, 3))
		{
			OUT_SO(BIT(SIO, 3));
		}
		else
		{
			OUT_SO(0);
		}

		SIO <<= 1;

		SIO = (SIO & 0x0e) | BIT(IN_SI(), 0);

		if (SKL)
		{
			OUT_SK(1);
		}
		else
		{
			OUT_SK(0);
		}
	}

}

INLINE void WRITE_Q(UINT8 data)
{
	Q = data;

	if (BIT(EN, 2))
	{
		OUT_L(Q);
	}
}

INLINE void WRITE_G(UINT8 data)
{
	G = data;
	OUT_G(G);
}

INSTRUCTION(illegal)
{
	logerror("COP400: PC = %04x, Illegal opcode = %02x\n", PC-1, ROM(PC-1));
}

INSTRUCTION(asc)
{
	A = A + C + RAM_R(B);

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

INSTRUCTION(add)
{
	A = (A + RAM_R(B)) & 0x0F;
}

INSTRUCTION(aisc)
{
	UINT4 y = opcode & 0x0f;

	A = A + y;

	if (A > 0x0f)
	{
		skip = 1;
		A &= 0xF;
	}
}

INSTRUCTION(clra)
{
	A = 0;
}

INSTRUCTION(comp)
{
	A = A ^ 0xF;
}

INSTRUCTION(nop)
{
}

INSTRUCTION(rc)
{
	C = 0;
}

INSTRUCTION(sc)
{
	C = 1;
}

INSTRUCTION(xor)
{
	A = A ^ RAM_R(B);
}

INSTRUCTION(jid)
{
	UINT16 addr = (PC & 0x300) | (A << 4) | RAM_R(B);
	PC = (PC & 0x300) | ROM(addr);
}

INSTRUCTION(jmp)
{
	UINT16 a = ((opcode & 0x03) << 8) | ROM(PC);

	PC = a;
}

INSTRUCTION(jp)
{
	UINT4 page = PC >> 6;

	if (page == 2 || page == 3)
	{
		UINT8 a = opcode & 0x7f;
		PC = (PC & 0x380) | a;
	}
	else if ((opcode & 0xc0) == 0xc0)
	{
		UINT8 a = opcode & 0x3f;
		PC = (PC & 0x3c0) | a;
	}
	else
	{
		UINT8 a = opcode & 0x3f;
		PUSH(PC);
		PC = 0x80 | a;
	}
}

INSTRUCTION(jsr)
{
	UINT16 a = ((opcode & 0x03) << 8) | ROM(PC);

	PUSH(PC + 1);
	PC = a;
}

INSTRUCTION(ret)
{
	POP();
}

INSTRUCTION(retsk)
{
	POP();
	skip = 1;
}

INSTRUCTION(camq)
{
	WRITE_Q((A << 4) | RAM_R(B));
}

INSTRUCTION(ld)
{
	UINT8 r = opcode & 0x30;

	A = RAM_R(B);
	B = B ^ r;
}

INSTRUCTION(lqid)
{
	PUSH(PC);
	PC = (PC & 0x300) | (A << 4) | RAM_R(B);
	WRITE_Q(ROM(PC));
	POP();
}

INSTRUCTION(rmb0) { RAM_W(B, RAM_R(B) & 0xE); }
INSTRUCTION(rmb1) { RAM_W(B, RAM_R(B) & 0xD); }
INSTRUCTION(rmb2) { RAM_W(B, RAM_R(B) & 0xB); }
INSTRUCTION(rmb3) { RAM_W(B, RAM_R(B) & 0x7); }

INSTRUCTION(smb0) { RAM_W(B, RAM_R(B) | 0x1); }
INSTRUCTION(smb1) { RAM_W(B, RAM_R(B) | 0x2); }
INSTRUCTION(smb2) { RAM_W(B, RAM_R(B) | 0x4); }
INSTRUCTION(smb3) { RAM_W(B, RAM_R(B) | 0x8); }

INSTRUCTION(stii)
{
	UINT4 y = opcode & 0x0f;
	UINT16 Bd;

	RAM_W(B, y);

	Bd = ((B & 0x0f) + 1) & 0x0f;
	B = (B & 0x30) + Bd;
}

INSTRUCTION(x)
{
	UINT4 r = opcode & 0x30;
	UINT8 t = RAM_R(B);

	RAM_W(B, A);

	A = t;
	B = B ^ r;
}

INSTRUCTION(xad)
{
	UINT8 rd = opcode & 0x3f;
	UINT8 t = A;

	A = RAM_R(rd);

	RAM_W(rd, t);
}

INSTRUCTION(xds)
{
	UINT8 t, Bd;
	UINT4 r = opcode & 0x30;

	t = RAM_R(B);
	RAM_W(B, A);
	A = t;

	Bd = ((B & 0x0f) - 1) & 0x0f;
	B = (B & 0x30) | Bd;

	B = B ^ r;

	if (Bd == 0x0f) skip = 1;
}

INSTRUCTION(xis)
{
	UINT8 t, Bd;
	UINT4 r = opcode & 0x30;

	t = RAM_R(B);
	RAM_W(B, A);
	A = t;

	Bd = ((B & 0x0f) + 1) & 0x0f;
	B = (B & 0x30) | Bd;

	B = B ^ r;

	if (Bd == 0x00) skip = 1;
}

INSTRUCTION(cab)
{
	B = (B & 0x30) | A;
}

INSTRUCTION(cba)
{
	A = B & 0xF;
}

INSTRUCTION(lbi)
{
	if (opcode & 0x80)
	{
		B = opcode & 0x3f;
	}
	else
	{
		B = (opcode & 0x30) | (((opcode & 0x0f) + 1) & 0x0f);
	}

	skipLBI = 1;
}

INSTRUCTION(lei)
{
	UINT4 y = opcode & 0x0f;

	EN = y;

	if (!BIT(EN, 2))
	{
		OUT_L(0);
	}
}

INSTRUCTION(skc)
{
	if (C == 1) skip = 1;
}

INSTRUCTION(ske)
{
	if (A == RAM_R(B)) skip = 1;
}

INSTRUCTION(skgz)
{
	if (IN_G() == 0) skip = 1;
}

INLINE void skgbz(int bit)
{
	if (!BIT(IN_G(), bit)) skip = 1;
}

INSTRUCTION(skgbz0) { skgbz(0); }
INSTRUCTION(skgbz1) { skgbz(1); }
INSTRUCTION(skgbz2) { skgbz(2); }
INSTRUCTION(skgbz3) { skgbz(3); }

INLINE void skmbz(int bit)
{
	if (!BIT(RAM_R(B), bit)) skip = 1;
}

INSTRUCTION(skmbz0) { skmbz(0); }
INSTRUCTION(skmbz1) { skmbz(1); }
INSTRUCTION(skmbz2) { skmbz(2); }
INSTRUCTION(skmbz3) { skmbz(3); }

INSTRUCTION(ing)
{
	A = IN_G();
}

INSTRUCTION(inl)
{
	UINT8 L = IN_L();

	RAM_W(B, L >> 4);
	A = L & 0xF;
}

INSTRUCTION(obd)
{
	OUT_D(B & 0x0f);
}

INSTRUCTION(omg)
{
	WRITE_G(RAM_R(B));
}

INSTRUCTION(xas)
{
	UINT8 t = SIO;
	SIO = A;
	A = t;

	SKL = C;
}

// license:BSD-3-Clause
// copyright-holders:Ville Linde, Ryan Holtz
/* Nintendo/SGI Reality Signal Processor emulator, written by Ville Linde. */


#include "cpuintrf.h"
#include "rsp.h"
#include "debugger.h"
#include <math.h>

#define LOG_INSTRUCTION_EXECUTION		0
#define SAVE_DISASM						0
#define SAVE_DMEM						0
#define RSP_TEST_SYNC                   0

#define PRINT_VECREG(x)		logerror("V%d: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X\n", (x), \
							(UINT16)R_VREG_S((x),0), (UINT16)R_VREG_S((x),1), \
							(UINT16)R_VREG_S((x),2), (UINT16)R_VREG_S((x),3), \
							(UINT16)R_VREG_S((x),4), (UINT16)R_VREG_S((x),5), \
							(UINT16)R_VREG_S((x),6), (UINT16)R_VREG_S((x),7))

#define PRINT_ACCUM(x)     logerror("A%d: %08X|%08X\n", (x), \
                            (UINT32)( ( ACCUM(x) >> 32 ) & 0x00000000ffffffff ),    \
                            (UINT32)(   ACCUM(x)         & 0x00000000ffffffff ))

extern offs_t rsp_dasm_one(char *buffer, offs_t pc, UINT32 op);

#if LOG_INSTRUCTION_EXECUTION
static FILE *exec_output;
#endif

extern UINT32 sp_read_reg(UINT32 reg);
extern void sp_write_reg(UINT32 reg, UINT32 data);
extern READ32_HANDLER( n64_dp_reg_r );
extern WRITE32_HANDLER( n64_dp_reg_w );
extern void sp_set_status(UINT32 status);

typedef struct
{
	UINT64 d[2];

} VECTOR_REG;

typedef struct
{
	UINT32 pc;
	UINT32 r[32];
	VECTOR_REG v[32];
	UINT16 flag[4];
    UINT32 sr;

	INT64 accum[8];
	INT32 square_root_res;
	INT32 square_root_high;
	INT32 reciprocal_res;
	INT32 reciprocal_high;

	UINT32 ppc;
	UINT32 nextpc;

	int (*irq_callback)(int irqline);
} RSP_REGS;

#define RSREG		((op >> 21) & 0x1f)
#define RTREG		((op >> 16) & 0x1f)
#define RDREG		((op >> 11) & 0x1f)
#define SHIFT		((op >> 6) & 0x1f)

#define RSVAL		(rsp.r[RSREG])
#define RTVAL		(rsp.r[RTREG])
#define RDVAL		(rsp.r[RDREG])

#define SIMM16		((INT32)(INT16)(op))
#define UIMM16		((UINT16)(op))
#define UIMM26		(op & 0x03ffffff)

#define JUMP_ABS(addr)			{ rsp.nextpc = 0x04001000 | (((addr) << 2) & 0xfff); }
#define JUMP_ABS_L(addr,l)		{ rsp.nextpc = 0x04001000 | (((addr) << 2) & 0xfff); rsp.r[l] = rsp.pc + 4; }
#define JUMP_REL(offset)		{ rsp.nextpc = 0x04001000 | ((rsp.pc + ((offset) << 2)) & 0xfff); }
#define JUMP_REL_L(offset,l)	{ rsp.nextpc = 0x04001000 | ((rsp.pc + ((offset) << 2)) & 0xfff); rsp.r[l] = rsp.pc + 4; }
#define JUMP_PC(addr)			{ rsp.nextpc = 0x04001000 | ((addr) & 0xfff); }
#define JUMP_PC_L(addr,l)		{ rsp.nextpc = 0x04001000 | ((addr) & 0xfff); rsp.r[l] = rsp.pc + 4; }

#define VDREG		((op >> 6) & 0x1f)
#define VS1REG		((op >> 11) & 0x1f)
#define VS2REG		((op >> 16) & 0x1f)
#define EL			((op >> 21) & 0xf)

#define S_VREG_B(offset)			(((15 - (offset)) & 0x07) << 3)
#define S_VREG_S(offset)			(((7 - (offset)) & 0x03) << 4)
#define S_VREG_L(offset)			(((3 - (offset)) & 0x01) << 5)

#define M_VREG_B(offset)			((UINT64)0x00FF << S_VREG_B(offset))
#define M_VREG_S(offset)			((UINT64)0x0000FFFFul << S_VREG_S(offset))
#define M_VREG_L(offset)			((UINT64)0x00000000FFFFFFFFull << S_VREG_L(offset))

#define R_VREG_B(reg, offset)		((rsp.v[(reg)].d[(15 - (offset)) >> 3] >> S_VREG_B(offset)) & 0x00FF)
#define R_VREG_S(reg, offset)		(INT16)((rsp.v[(reg)].d[(7 - (offset)) >> 2] >> S_VREG_S(offset)) & 0x0000FFFFul)
#define R_VREG_L(reg, offset)		((rsp.v[(reg)].d[(3 - (offset)) >> 1] >> S_VREG_L(offset)) & 0x00000000FFFFFFFFull)

#define W_VREG_B(reg, offset, val)	(rsp.v[(reg)].d[(15 - (offset)) >> 3] = (rsp.v[(reg)].d[(15 - (offset)) >> 3] & ~M_VREG_B(offset)) | (M_VREG_B(offset) & ((UINT64)(val) << S_VREG_B(offset))))
#define W_VREG_S(reg, offset, val)	(rsp.v[(reg)].d[(7 - (offset)) >> 2] = (rsp.v[(reg)].d[(7 - (offset)) >> 2] & ~M_VREG_S(offset)) | (M_VREG_S(offset) & ((UINT64)(val) << S_VREG_S(offset))))
#define W_VREG_L(reg, offset, val)	(rsp.v[(reg)].d[(3 - (offset)) >> 1] = (rsp.v[(reg)].d[(3 - (offset)) >> 1] & ~M_VREG_L(offset)) | (M_VREG_L(offset) & ((UINT64)(val) << S_VREG_L(offset))))

#define VEC_EL_1(x,z)			(vector_elements_1[(x)][(z)])
#define VEC_EL_2(x,z)			(vector_elements_2[(x)][(z)])

#define ACCUM(x)				(rsp.accum[(7-(x))])

#define S_ACCUM_H				(3 << 4)
#define S_ACCUM_M				(2 << 4)
#define S_ACCUM_L				(1 << 4)

#define M_ACCUM_H				((INT64)0x0000FFFFll << S_ACCUM_H)
#define M_ACCUM_M				((INT64)0x0000FFFFll << S_ACCUM_M)
#define M_ACCUM_L				((INT64)0x0000FFFFll << S_ACCUM_L)

#define R_ACCUM_H(x)			((INT16)((ACCUM(x) >> S_ACCUM_H) & 0x00FFFF))
#define R_ACCUM_M(x)			((INT16)((ACCUM(x) >> S_ACCUM_M) & 0x00FFFF))
#define R_ACCUM_L(x)			((INT16)((ACCUM(x) >> S_ACCUM_L) & 0x00FFFF))

#define W_ACCUM_H(x, y)			(ACCUM(x) = (ACCUM(x) & ~M_ACCUM_H) | (M_ACCUM_H & ((INT64)(y) << S_ACCUM_H)))
#define W_ACCUM_M(x, y)			(ACCUM(x) = (ACCUM(x) & ~M_ACCUM_M) | (M_ACCUM_M & ((INT64)(y) << S_ACCUM_M)))
#define W_ACCUM_L(x, y)			(ACCUM(x) = (ACCUM(x) & ~M_ACCUM_L) | (M_ACCUM_L & ((INT64)(y) << S_ACCUM_L)))

#define CARRY_FLAG(x)			((rsp.flag[0] & (1 << ((x)))) ? 1 : 0)
#define CLEAR_CARRY_FLAGS()		{ rsp.flag[0] &= ~0xff; }
#define SET_CARRY_FLAG(x)		{ rsp.flag[0] |= (1 << ((x))); }
#define CLEAR_CARRY_FLAG(x)		{ rsp.flag[0] &= ~(1 << ((x))); }

#define COMPARE_FLAG(x)			((rsp.flag[1] & (1 << ((x)))) ? 1 : 0)
#define CLEAR_COMPARE_FLAGS()	{ rsp.flag[1] &= ~0xff; }
#define SET_COMPARE_FLAG(x)		{ rsp.flag[1] |= (1 << ((x))); }
#define CLEAR_COMPARE_FLAG(x)	{ rsp.flag[1] &= ~(1 << ((x))); }

#define ZERO_FLAG(x)			((rsp.flag[0] & (1 << (8+(x)))) ? 1 : 0)
#define CLEAR_ZERO_FLAGS()		{ rsp.flag[0] &= ~0xff00; }
#define SET_ZERO_FLAG(x)		{ rsp.flag[0] |= (1 << (8+(x))); }
#define CLEAR_ZERO_FLAG(x)		{ rsp.flag[0] &= ~(1 << (8+(x))); }

static RSP_REGS rsp;
static int rsp_icount;

#define ROPCODE(pc)		cpu_readop32(pc)

INLINE UINT8 READ8(UINT32 address)
{
	address = 0x04000000 | (address & 0xfff);
	return program_read_byte_32be(address);
}

INLINE UINT16 READ16(UINT32 address)
{
	address = 0x04000000 | (address & 0xfff);

	if (address & 1)
	{

		return ((program_read_byte_32be(address+0) & 0xff) << 8) | (program_read_byte_32be(address+1) & 0xff);
	}

	return program_read_word_32be(address);
}

INLINE UINT32 READ32(UINT32 address)
{
	address = 0x04000000 | (address & 0xfff);

	if (address & 3)
	{

		return ((program_read_byte_32be(address + 0) & 0xff) << 24) |
			   ((program_read_byte_32be(address + 1) & 0xff) << 16) |
			   ((program_read_byte_32be(address + 2) & 0xff) << 8) |
			   ((program_read_byte_32be(address + 3) & 0xff) << 0);
	}

	return program_read_dword_32be(address);
}

INLINE void WRITE8(UINT32 address, UINT8 data)
{
	address = 0x04000000 | (address & 0xfff);
	program_write_byte_32be(address, data);
}

INLINE void WRITE16(UINT32 address, UINT16 data)
{
	address = 0x04000000 | (address & 0xfff);

	if (address & 1)
	{

		program_write_byte_32be(address + 0, (data >> 8) & 0xff);
		program_write_byte_32be(address + 1, (data >> 0) & 0xff);
		return;
	}

	program_write_word_32be(address, data);
}

INLINE void WRITE32(UINT32 address, UINT32 data)
{
	address = 0x04000000 | (address & 0xfff);

	if (address & 3)
	{

		program_write_byte_32be(address + 0, (data >> 24) & 0xff);
		program_write_byte_32be(address + 1, (data >> 16) & 0xff);
		program_write_byte_32be(address + 2, (data >> 8) & 0xff);
		program_write_byte_32be(address + 3, (data >> 0) & 0xff);
		return;
	}

	program_write_dword_32be(address, data);
}

static UINT32 get_cop0_reg(int reg)
{
	if (reg >= 0 && reg < 8)
	{
		return sp_read_reg(reg);
	}
	else if (reg >= 8 && reg < 16)
	{
		return n64_dp_reg_r(reg - 8, 0x00000000);
	}
	else
	{
		fatalerror("RSP: get_cop0_reg: %d", reg);
	}
}

static void set_cop0_reg(int reg, UINT32 data)
{
	if (reg >= 0 && reg < 8)
	{
		sp_write_reg(reg, data);
	}
	else if (reg >= 8 && reg < 16)
	{
		n64_dp_reg_w(reg - 8, data, 0x00000000);
	}
	else
	{
		fatalerror("RSP: set_cop0_reg: %d, %08X\n", reg, data);
	}
}

static void unimplemented_opcode(UINT32 op)
{
#ifdef MAME_DEBUG
	char string[200];
	rsp_dasm_one(string, rsp.ppc, op);
	logerror("%08X: %s\n", rsp.ppc, string);
#endif

#if SAVE_DISASM
	{
		char string[200];
		int i;
		FILE *dasm;
		dasm = fopen("rsp_disasm.txt", "wt");

		for (i=0; i < 0x1000; i+=4)
		{
			UINT32 opcode = ROPCODE(0x04001000 + i);
			rsp_dasm_one(string, 0x04001000 + i, opcode);
			fprintf(dasm, "%08X: %08X   %s\n", 0x04001000 + i, opcode, string);
		}
		fclose(dasm);
	}
#endif
#if SAVE_DMEM
	{
		int i;
		FILE *dmem;
		dmem = fopen("rsp_dmem.bin", "wb");

		for (i=0; i < 0x1000; i++)
		{
			fputc(READ8(0x04000000 + i), dmem);
		}
		fclose(dmem);
	}
#endif

	fatalerror("RSP: unknown opcode %02X (%08X) at %08X\n", op >> 26, op, rsp.ppc);
}

static const int vector_elements_1[16][8] =
{
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 1, 2, 3, 4, 5, 6 ,7 },
	{ 1, 3, 5, 7, 0, 2, 4, 6 },
	{ 0, 2, 4, 6, 1, 3, 5, 7 },
	{ 1, 2, 3, 5, 6, 7, 0, 4 },
	{ 0, 2, 3, 4, 6, 7, 1, 5 },
	{ 0, 1, 3, 4, 5, 7, 2, 6 },
	{ 0, 1, 2, 4, 5, 6, 3, 7 },
	{ 1, 2, 3, 4, 5, 6, 7, 0 },
	{ 0, 2, 3, 4, 5, 6, 7, 1 },
	{ 0, 1, 3, 4, 5, 6, 7, 2 },
	{ 0, 1, 2, 4, 5, 6, 7, 3 },
	{ 0, 1, 2, 3, 5, 6, 7, 4 },
	{ 0, 1, 2, 3, 4, 6, 7, 5 },
	{ 0, 1, 2, 3, 4, 5, 7, 6 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
};

static const int vector_elements_2[16][8] =
{
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 0, 2, 2, 4, 4, 6, 6 },
	{ 1, 1, 3, 3, 5, 5, 7, 7 },
	{ 0, 0, 0, 0, 4, 4, 4, 4 },
	{ 1, 1, 1, 1, 5, 5, 5, 5 },
	{ 2, 2, 2, 2, 6, 6, 6, 6 },
	{ 3, 3, 3, 3, 7, 7, 7, 7 },
	{ 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 2, 2, 2, 2, 2, 2, 2, 2 },
	{ 3, 3, 3, 3, 3, 3, 3, 3 },
	{ 4, 4, 4, 4, 4, 4, 4, 4 },
	{ 5, 5, 5, 5, 5, 5, 5, 5 },
	{ 6, 6, 6, 6, 6, 6, 6, 6 },
	{ 7, 7, 7, 7, 7, 7, 7, 7 },
};

static void rsp_init(int index, int clock, const void *_config, int (*irqcallback)(int))
{

#if LOG_INSTRUCTION_EXECUTION
	exec_output = fopen("rsp_execute.txt", "wt");
#endif

	rsp.irq_callback = irqcallback;

    rsp.sr = RSP_STATUS_HALT;
}

static void rsp_exit(void)
{
#if SAVE_DISASM
	{
		char string[200];
		int i;
		FILE *dasm;
		dasm = fopen("rsp_disasm.txt", "wt");

		for (i=0; i < 0x1000; i+=4)
		{
			UINT32 opcode = ROPCODE(0x04001000 + i);
			rsp_dasm_one(string, 0x04001000 + i, opcode);
			fprintf(dasm, "%08X: %08X   %s\n", 0x04001000 + i, opcode, string);
		}
		fclose(dasm);
	}
#endif
#if SAVE_DMEM
	{

		int i;
		FILE *dmem;
		dmem = fopen("rsp_dmem.bin", "wb");

		for (i=0; i < 0x1000; i++)
		{
			fputc(READ8(0x04000000 + i), dmem);
		}
		fclose(dmem);
	}
#endif

#if LOG_INSTRUCTION_EXECUTION
	fclose(exec_output);
#endif
}

static void rsp_reset(void)
{
	rsp.pc = 0;
	rsp.sr = RSP_STATUS_HALT;
	rsp.nextpc = ~0;
}

static void handle_lwc2(UINT32 op)
{
	int i, end;
	UINT32 ea;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
		offset |= 0xffffffc0;

	switch ((op >> 11) & 0x1f)
	{
		case 0x00:
		{

			ea = (base) ? rsp.r[base] + offset : offset;
			W_VREG_B(dest, index, READ8(ea));
			break;
		}
		case 0x01:
		{

			ea = (base) ? rsp.r[base] + (offset * 2) : (offset * 2);

			end = index + 2;

			for (i=index; i < end; i++)
			{
				W_VREG_B(dest, i, READ8(ea));
				ea++;
			}
			break;
		}
		case 0x02:
		{

			ea = (base) ? rsp.r[base] + (offset * 4) : (offset * 4);

			end = index + 4;

			for (i=index; i < end; i++)
			{
				W_VREG_B(dest, i, READ8(ea));
				ea++;
			}
			break;
		}
		case 0x03:
		{

			ea = (base) ? rsp.r[base] + (offset * 8) : (offset * 8);

			end = index + 8;

			for (i=index; i < end; i++)
			{
				W_VREG_B(dest, i, READ8(ea));
				ea++;
			}
			break;
		}
		case 0x04:
		{

			ea = (base) ? rsp.r[base] + (offset * 16) : (offset * 16);

			end = index + (16 - (ea & 0xf));
			if (end > 16) end = 16;

			for (i=index; i < end; i++)
			{
				W_VREG_B(dest, i, READ8(ea));
				ea++;
			}
			break;
		}
		case 0x05:
		{

			ea = (base) ? rsp.r[base] + (offset * 16) : (offset * 16);

			index = 16 - ((ea & 0xf) - index);
			end = 16;
			ea &= ~0xf;

			for (i=index; i < end; i++)
			{
				W_VREG_B(dest, i, READ8(ea));
				ea++;
			}
			break;
		}
		case 0x06:
		{

			ea = (base) ? rsp.r[base] + (offset * 8) : (offset * 8);

			for (i=0; i < 8; i++)
			{
				W_VREG_S(dest, i, READ8(ea + (((16-index) + i) & 0xf)) << 8);
			}
			break;
		}
		case 0x07:
		{

			ea = (base) ? rsp.r[base] + (offset * 8) : (offset * 8);

			for (i=0; i < 8; i++)
			{
				W_VREG_S(dest, i, READ8(ea + (((16-index) + i) & 0xf)) << 7);
			}
			break;
		}
		case 0x08:
		{

			ea = (base) ? rsp.r[base] + (offset * 16) : (offset * 16);

			for (i=0; i < 8; i++)
			{
				W_VREG_S(dest, i, READ8(ea + (((16-index) + (i<<1)) & 0xf)) << 7);
			}
			break;
		}
		case 0x09:
		{

			fatalerror("RSP: LFV\n");

			if (index & 0x7)	fatalerror("RSP: LFV: index = %d at %08X\n", index, rsp.ppc);

			ea = (base) ? rsp.r[base] + (offset * 16) : (offset * 16);

			if ((ea & 0xf) > 0)	fatalerror("RSP: LFV: 16-byte boundary crossing at %08X, recheck this!\n", rsp.ppc);

			end = (index >> 1) + 4;

			for (i=index >> 1; i < end; i++)
			{
				W_VREG_S(dest, i, READ8(ea) << 7);
				ea += 4;
			}
			break;
		}
		case 0x0a:
		{

			ea = (base) ? rsp.r[base] + (offset * 16) : (offset * 16);

			if ((ea & 0xf) > 0) fatalerror("RSP: LWV: 16-byte boundary crossing at %08X, recheck this!\n", rsp.ppc);

			end = (16 - index) + 16;

			for (i=(16 - index); i < end; i++)
			{
				W_VREG_B(dest, i & 0xf, READ8(ea));
				ea += 4;
			}
			break;
		}
		case 0x0b:
		{

			int element;
			int vs = dest;
			int ve = dest + 8;
			if (ve > 32)
				ve = 32;

			element = 7 - (index >> 1);

			if (index & 1)	fatalerror("RSP: LTV: index = %d\n", index);

			ea = (base) ? rsp.r[base] + (offset * 16) : (offset * 16);

			ea = ((ea + 8) & ~0xf) + (index & 1);
			for (i=vs; i < ve; i++)
			{
				element = ((8 - (index >> 1) + (i-vs)) << 1);
				W_VREG_B(i, (element & 0xf), READ8(ea));
				W_VREG_B(i, ((element+1) & 0xf), READ8(ea+1));

				ea += 2;
			}
			break;
		}

		default:
		{
			unimplemented_opcode(op);
			break;
		}
	}
}

static void handle_swc2(UINT32 op)
{
	int i, end;
	int eaoffset;
	UINT32 ea;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
		offset |= 0xffffffc0;

	switch ((op >> 11) & 0x1f)
	{
		case 0x00:
		{

			ea = (base) ? rsp.r[base] + offset : offset;
			WRITE8(ea, R_VREG_B(dest, index));
			break;
		}
		case 0x01:
		{

			ea = (base) ? rsp.r[base] + (offset * 2) : (offset * 2);

			end = index + 2;

			for (i=index; i < end; i++)
			{
				WRITE8(ea, R_VREG_B(dest, i));
				ea++;
			}
			break;
		}
		case 0x02:
		{

			ea = (base) ? rsp.r[base] + (offset * 4) : (offset * 4);

			end = index + 4;

			for (i=index; i < end; i++)
			{
				WRITE8(ea, R_VREG_B(dest, i));
				ea++;
			}
			break;
		}
		case 0x03:
		{

			ea = (base) ? rsp.r[base] + (offset * 8) : (offset * 8);

			end = index + 8;

			for (i=index; i < end; i++)
			{
				WRITE8(ea, R_VREG_B(dest, i));
				ea++;
			}
			break;
		}
		case 0x04:
		{

			ea = (base) ? rsp.r[base] + (offset * 16) : (offset * 16);

			end = index + (16 - (ea & 0xf));

			for (i=index; i < end; i++)
			{
				WRITE8(ea, R_VREG_B(dest, i & 0xf));
				ea++;
			}
			break;
		}
		case 0x05:
		{

			int o;
			ea = (base) ? rsp.r[base] + (offset * 16) : (offset * 16);

			end = index + (ea & 0xf);
			o = (16 - (ea & 0xf)) & 0xf;
			ea &= ~0xf;

			for (i=index; i < end; i++)
			{
				WRITE8(ea, R_VREG_B(dest, ((i + o) & 0xf)));
				ea++;
			}
			break;
		}
		case 0x06:
		{

			ea = (base) ? rsp.r[base] + (offset * 8) : (offset * 8);
			end = index + 8;

			for (i=index; i < end; i++)
			{
				if ((i & 0xf) < 8)
				{
					WRITE8(ea, R_VREG_B(dest, ((i & 0xf) << 1)));
				}
				else
				{
					WRITE8(ea, R_VREG_S(dest, (i & 0x7)) >> 7);
				}
				ea++;
			}
			break;
		}
		case 0x07:
		{

			ea = (base) ? rsp.r[base] + (offset * 8) : (offset * 8);
			end = index + 8;

			for (i=index; i < end; i++)
			{
				if ((i & 0xf) < 8)
				{
					WRITE8(ea, R_VREG_S(dest, (i & 0x7)) >> 7);
				}
				else
				{
					WRITE8(ea, R_VREG_B(dest, ((i & 0x7) << 1)));
				}
				ea++;
			}
			break;
		}
		case 0x08:
		{

			ea = (base) ? rsp.r[base] + (offset * 16) : (offset * 16);

			for (i=0; i < 8; i++)
			{
				UINT8 d = ((R_VREG_B(dest, ((index + (i << 1) + 0) & 0xf))) << 1) |
						  ((R_VREG_B(dest, ((index + (i << 1) + 1) & 0xf))) >> 7);

				WRITE8(ea, d);
				ea += 2;
			}
			break;
		}
		case 0x09:
		{

			if (index & 0x7)	logerror("RSP: SFV: index = %d at %08X\n", index, rsp.ppc);

			ea = (base) ? rsp.r[base] + (offset * 16) : (offset * 16);

			eaoffset = ea & 0xf;
			ea &= ~0xf;

			end = (index >> 1) + 4;

			for (i=index >> 1; i < end; i++)
			{
				WRITE8(ea + (eaoffset & 0xf), R_VREG_S(dest, i) >> 7);
				eaoffset += 4;
			}
			break;
		}
		case 0x0a:
		{

			ea = (base) ? rsp.r[base] + (offset * 16) : (offset * 16);

			eaoffset = ea & 0xf;
			ea &= ~0xf;

			end = index + 16;

			for (i=index; i < end; i++)
			{
				WRITE8(ea + (eaoffset & 0xf), R_VREG_B(dest, i & 0xf));
				eaoffset++;
			}
			break;
		}
		case 0x0b:
		{

			int element, eaoffset;
			int vs = dest;
			int ve = dest + 8;
			if (ve > 32)
				ve = 32;

			element = 8 - (index >> 1);
			if (index & 0x1)	fatalerror("RSP: STV: index = %d at %08X\n", index, rsp.ppc);

			ea = (base) ? rsp.r[base] + (offset * 16) : (offset * 16);

			if (ea & 0x1)		fatalerror("RSP: STV: ea = %08X at %08X\n", ea, rsp.ppc);

			eaoffset = (ea & 0xf) + (element * 2);
			ea &= ~0xf;

			for (i=vs; i < ve; i++)
			{
				WRITE16(ea + (eaoffset & 0xf), R_VREG_S(i, element & 0x7));
				eaoffset += 2;
				element++;
			}
			break;
		}

		default:
		{
			unimplemented_opcode(op);
			break;
		}
	}
}

INLINE UINT16 SATURATE_ACCUM(int accum, int slice, UINT16 negative, UINT16 positive)
{
	if ((INT16)R_ACCUM_H(accum) < 0)
	{
		if ((UINT16)(R_ACCUM_H(accum)) != 0xffff)
		{
			return negative;
		}
		else
		{
			if ((INT16)R_ACCUM_M(accum) >= 0)
			{
				return negative;
			}
			else
			{
				if (slice == 0)
				{
					return R_ACCUM_L(accum);
				}
				else if (slice == 1)
				{
					return R_ACCUM_M(accum);
				}
			}
		}
	}
	else
	{
		if ((UINT16)(R_ACCUM_H(accum)) != 0)
		{
			return positive;
		}
		else
		{
			if ((INT16)R_ACCUM_M(accum) < 0)
			{
				return positive;
			}
			else
			{
				if (slice == 0)
				{
					return R_ACCUM_L(accum);
				}
				else
				{
					return R_ACCUM_M(accum);
				}
			}
		}
	}

	return 0;
}

#define WRITEBACK_RESULT() 					\
	do {									\
		W_VREG_S(VDREG, 0, vres[0]);			\
		W_VREG_S(VDREG, 1, vres[1]);			\
		W_VREG_S(VDREG, 2, vres[2]);			\
		W_VREG_S(VDREG, 3, vres[3]);			\
		W_VREG_S(VDREG, 4, vres[4]);			\
		W_VREG_S(VDREG, 5, vres[5]);			\
		W_VREG_S(VDREG, 6, vres[6]);			\
		W_VREG_S(VDREG, 7, vres[7]);			\
	} while(0)

#if 0
static float float_round(float input)
{
    INT32 integer = (INT32)input;
    float fraction = input - (float)integer;
    float output = 0.0f;
    if( fraction >= 0.5f )
    {
        output = (float)( integer + 1 );
    }
    else
    {
        output = (float)integer;
    }
    return output;
}
#endif

static void handle_vector_ops(UINT32 op)
{
	int i;
	INT16 vres[8];

	switch (op & 0x3f)
	{
		case 0x00:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT32 s1 = (INT32)(INT16)R_VREG_S(VS1REG, del);
				INT32 s2 = (INT32)(INT16)R_VREG_S(VS2REG, sel);
				if (s1 == -32768 && s2 == -32768)
				{

					W_ACCUM_H(del, 0);
					W_ACCUM_M(del, -32768);
					W_ACCUM_L(del, -32768);
					vres[del] = 0x7fff;
				}
				else
				{
					INT64 r =  s1 * s2 * 2;
					r += 0x8000;
					W_ACCUM_H(del, (r < 0) ? 0xffff : 0);
					W_ACCUM_M(del, (INT16)(r >> 16));
					W_ACCUM_L(del, (UINT16)(r));
					vres[del] = R_ACCUM_M(del);
				}
			}
			WRITEBACK_RESULT();

			break;
		}

		case 0x01:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT32 s1 = (INT32)(INT16)R_VREG_S(VS1REG, del);
				INT32 s2 = (INT32)(INT16)R_VREG_S(VS2REG, sel);
				INT64 r = s1 * s2 * 2;
				r += 0x8000;

				W_ACCUM_H(del, (UINT16)(r >> 32));
				W_ACCUM_M(del, (UINT16)(r >> 16));
				W_ACCUM_L(del, (UINT16)(r));

				if (r < 0)
				{
					vres[del] = 0;
				}
				else if (((INT16)(R_ACCUM_H(del)) ^ (INT16)(R_ACCUM_M(del))) < 0)
				{
					vres[del] = -1;
				}
				else
				{
					vres[del] = R_ACCUM_M(del);
				}
			}
			WRITEBACK_RESULT();
			break;
		}

		case 0x04:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				UINT32 s1 = (UINT32)(UINT16)R_VREG_S(VS1REG, del);
				UINT32 s2 = (UINT32)(UINT16)R_VREG_S(VS2REG, sel);
				UINT32 r = s1 * s2;

				W_ACCUM_H(del, 0);
				W_ACCUM_M(del, 0);
				W_ACCUM_L(del, (UINT16)(r >> 16));

				vres[del] = R_ACCUM_L(del);
			}
			WRITEBACK_RESULT();
			break;
		}

		case 0x05:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT32 s1 = (INT32)(INT16)R_VREG_S(VS1REG, del);
				INT32 s2 = (UINT16)R_VREG_S(VS2REG, sel);
				INT32 r =  s1 * s2;

				W_ACCUM_H(del, (r < 0) ? 0xffff : 0);
				W_ACCUM_M(del, (INT16)(r >> 16));
				W_ACCUM_L(del, (UINT16)(r));

				vres[del] = R_ACCUM_M(del);
			}
			WRITEBACK_RESULT();
			break;

		}

		case 0x06:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT32 s1 = (UINT16)R_VREG_S(VS1REG, del);
				INT32 s2 = (INT32)(INT16)R_VREG_S(VS2REG, sel);
				INT32 r = s1 * s2;

				W_ACCUM_H(del, (r < 0) ? 0xffff : 0);
				W_ACCUM_M(del, (INT16)(r >> 16));
				W_ACCUM_L(del, (UINT16)(r));

				vres[del] = R_ACCUM_L(del);
			}
			WRITEBACK_RESULT();
			break;
		}

		case 0x07:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT32 s1 = (INT32)(INT16)R_VREG_S(VS1REG, del);
				INT32 s2 = (INT32)(INT16)R_VREG_S(VS2REG, sel);
				INT32 r = s1 * s2;

				W_ACCUM_H(del, (INT16)(r >> 16));
				W_ACCUM_M(del, (UINT16)(r));
				W_ACCUM_L(del, 0);

				if (r < -32768) r = -32768;
				if (r >  32767)	r = 32767;
				vres[del] = (INT16)(r);
			}
			WRITEBACK_RESULT();
			break;
		}

		case 0x08:
		{

			for (i=0; i < 8; i++)
			{
				UINT16 res;
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT32 s1 = (INT32)(INT16)R_VREG_S(VS1REG, del);
				INT32 s2 = (INT32)(INT16)R_VREG_S(VS2REG, sel);
				INT32 r = s1 * s2;

				ACCUM(del) += (INT64)(r) << 17;
				res = SATURATE_ACCUM(del, 1, 0x8000, 0x7fff);

				vres[del] = res;
			}
			WRITEBACK_RESULT();
			break;
		}

		case 0x09:
		{

			for (i=0; i < 8; i++)
			{
				UINT16 res;
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT32 s1 = (INT32)(INT16)R_VREG_S(VS1REG, del);
				INT32 s2 = (INT32)(INT16)R_VREG_S(VS2REG, sel);
				INT32 r1 = s1 * s2;
				UINT32 r2 = (UINT16)R_ACCUM_L(del) + ((UINT16)(r1) * 2);
				UINT32 r3 = (UINT16)R_ACCUM_M(del) + (UINT16)((r1 >> 16) * 2) + (UINT16)(r2 >> 16);

				W_ACCUM_L(del, (UINT16)(r2));
				W_ACCUM_M(del, (UINT16)(r3));
				W_ACCUM_H(del, (UINT16)R_ACCUM_H(del) + (UINT16)(r3 >> 16) + (UINT16)(r1 >> 31));

				if ((INT16)R_ACCUM_H(del) < 0)
				{
					res = 0;
				}
				else
				{
					if (R_ACCUM_H(del) != 0)
					{
						res = 0xffff;
					}
					else
					{
						if ((INT16)R_ACCUM_M(del) < 0)
						{
							res = 0xffff;
						}
						else
						{
							res = R_ACCUM_M(del);
						}
					}
				}

				vres[del] = res;
			}
			WRITEBACK_RESULT();
			break;
		}

		case 0x0c:
		{

			for (i=0; i < 8; i++)
			{
				UINT16 res;
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				UINT32 s1 = (UINT32)(UINT16)R_VREG_S(VS1REG, del);
				UINT32 s2 = (UINT32)(UINT16)R_VREG_S(VS2REG, sel);
				UINT32 r1 = s1 * s2;
				UINT32 r2 = (UINT16)R_ACCUM_L(del) + (r1 >> 16);
				UINT32 r3 = (UINT16)R_ACCUM_M(del) + (r2 >> 16);

				W_ACCUM_L(del, (UINT16)(r2));
				W_ACCUM_M(del, (UINT16)(r3));
				W_ACCUM_H(del, (INT16)R_ACCUM_H(del) + (INT16)(r3 >> 16));

				res = SATURATE_ACCUM(del, 0, 0x0000, 0xffff);

				vres[del] = res;
			}
			WRITEBACK_RESULT();
			break;
		}

		case 0x0d:
		{

			for (i=0; i < 8; i++)
			{
				UINT16 res;
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				UINT32 s1 = (INT32)(INT16)R_VREG_S(VS1REG, del);
				UINT32 s2 = (UINT16)R_VREG_S(VS2REG, sel);
				UINT32 r1 = s1 * s2;
				UINT32 r2 = (UINT16)R_ACCUM_L(del) + (UINT16)(r1);
				UINT32 r3 = (UINT16)R_ACCUM_M(del) + (r1 >> 16) + (r2 >> 16);

				W_ACCUM_L(del, (UINT16)(r2));
				W_ACCUM_M(del, (UINT16)(r3));
				W_ACCUM_H(del, (UINT16)R_ACCUM_H(del) + (UINT16)(r3 >> 16));
				if ((INT32)(r1) < 0)
					W_ACCUM_H(del, (UINT16)R_ACCUM_H(del) - 1);

				res = SATURATE_ACCUM(del, 1, 0x8000, 0x7fff);

				vres[del] = res;
			}
			WRITEBACK_RESULT();
			break;
		}

		case 0x0e:
		{

			for (i=0; i < 8; i++)
			{
				UINT16 res;
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT32 s1 = (UINT16)R_VREG_S(VS1REG, del);
				INT32 s2 = (INT32)(INT16)R_VREG_S(VS2REG, sel);
				UINT32 r1 = s1 * s2;
				UINT32 r2 = (UINT16)R_ACCUM_L(del) + (UINT16)(r1);
				UINT32 r3 = (UINT16)R_ACCUM_M(del) + (r1 >> 16) + (r2 >> 16);

				W_ACCUM_L(del, (UINT16)(r2));
				W_ACCUM_M(del, (UINT16)(r3));
				W_ACCUM_H(del, (UINT16)R_ACCUM_H(del) + (UINT16)(r3 >> 16));
				if ((INT32)(r1) < 0)
					W_ACCUM_H(del, (UINT16)R_ACCUM_H(del) - 1);

				res = SATURATE_ACCUM(del, 0, 0x0000, 0xffff);

				vres[del] = res;
			}
			WRITEBACK_RESULT();
			break;
		}

		case 0x0f:
		{

			for (i=0; i < 8; i++)
			{
				UINT16 res;
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT32 s1 = (INT32)(INT16)R_VREG_S(VS1REG, del);
				INT32 s2 = (INT32)(INT16)R_VREG_S(VS2REG, sel);
				INT64 r = s1 * s2;

				ACCUM(del) += (INT64)(r) << 32;

				res = SATURATE_ACCUM(del, 1, 0x8000, 0x7fff);

				vres[del] = res;
			}
			WRITEBACK_RESULT();
			break;
		}

		case 0x10:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT32 s1 = (INT32)(INT16)R_VREG_S(VS1REG, del);
				INT32 s2 = (INT32)(INT16)R_VREG_S(VS2REG, sel);
				INT32 r = s1 + s2 + CARRY_FLAG(del);

				W_ACCUM_L(del, (INT16)(r));

				if (r > 32767) r = 32767;
				if (r < -32768) r = -32768;
				vres[del] = (INT16)(r);
			}
			CLEAR_ZERO_FLAGS();
			CLEAR_CARRY_FLAGS();
			WRITEBACK_RESULT();
			break;
		}

		case 0x11:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT32 s1 = (INT32)(INT16)R_VREG_S(VS1REG, del);
				INT32 s2 = (INT32)(INT16)R_VREG_S(VS2REG, sel);
				INT32 r = s1 - s2 - CARRY_FLAG(del);

				W_ACCUM_L(del, (INT16)(r));

				if (r > 32767) r = 32767;
				if (r < -32768) r = -32768;

				vres[del] = (INT16)(r);
			}
			CLEAR_ZERO_FLAGS();
			CLEAR_CARRY_FLAGS();
			WRITEBACK_RESULT();
			break;
		}

		case 0x13:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT16 s1 = (INT16)R_VREG_S(VS1REG, del);
				INT16 s2 = (INT16)R_VREG_S(VS2REG, sel);

				if (s1 < 0)
				{
					if (s2 == -32768)
					{
						vres[del] = 32767;
					}
					else
					{
						vres[del] = -s2;
					}
				}
				else if (s1 > 0)
				{
					vres[del] = s2;
				}
				else
				{
					vres[del] = 0;
				}

				W_ACCUM_L(del, vres[del]);
			}
			WRITEBACK_RESULT();
			break;
		}

		case 0x14:
		{

			CLEAR_ZERO_FLAGS();
			CLEAR_CARRY_FLAGS();

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT32 s1 = (UINT32)(UINT16)R_VREG_S(VS1REG, del);
				INT32 s2 = (UINT32)(UINT16)R_VREG_S(VS2REG, sel);
				INT32 r = s1 + s2;

				vres[del] = (INT16)(r);
				W_ACCUM_L(del, (INT16)(r));

				if (r & 0xffff0000)
				{
					SET_CARRY_FLAG(del);
				}
			}
			WRITEBACK_RESULT();
			break;
		}

		case 0x15:
		{

			CLEAR_ZERO_FLAGS();
			CLEAR_CARRY_FLAGS();

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT32 s1 = (UINT32)(UINT16)R_VREG_S(VS1REG, del);
				INT32 s2 = (UINT32)(UINT16)R_VREG_S(VS2REG, sel);
				INT32 r = s1 - s2;

				vres[del] = (INT16)(r);
				W_ACCUM_L(del, (UINT16)(r));

				if ((UINT16)(r) != 0)
				{
					SET_ZERO_FLAG(del);
				}
				if (r & 0xffff0000)
				{
					SET_CARRY_FLAG(del);
				}
			}
			WRITEBACK_RESULT();
			break;
		}

		case 0x1d:
		{

			switch (EL)
			{
				case 0x08:
				{
					for (i=0; i < 8; i++)
					{
						W_VREG_S(VDREG, i, R_ACCUM_H(i));
					}
					break;
				}
				case 0x09:
				{
					for (i=0; i < 8; i++)
					{
						W_VREG_S(VDREG, i, R_ACCUM_M(i));
					}
					break;
				}
				case 0x0a:
				{
					for (i=0; i < 8; i++)
					{
						W_VREG_S(VDREG, i, R_ACCUM_L(i));
					}
					break;
				}
				default:	fatalerror("RSP: VSAW: el = %d\n", EL);
			}
			break;
		}

		case 0x20:
		{

			rsp.flag[1] = 0;

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);

				if (R_VREG_S(VS1REG, del) < R_VREG_S(VS2REG, sel))
				{
					vres[del] = R_VREG_S(VS1REG, del);
					SET_COMPARE_FLAG(del);
				}
				else if (R_VREG_S(VS1REG, del) == R_VREG_S(VS2REG, sel))
				{
					vres[del] = R_VREG_S(VS1REG, del);
					if (ZERO_FLAG(del) != 0 && CARRY_FLAG(del) != 0)
					{
						SET_COMPARE_FLAG(del);
					}
				}
				else
				{
					vres[del] = R_VREG_S(VS2REG, sel);
				}

				W_ACCUM_L(del, vres[del]);
			}

			CLEAR_ZERO_FLAGS();
			CLEAR_CARRY_FLAGS();
			WRITEBACK_RESULT();
			break;
		}

		case 0x21:
		{

			rsp.flag[1] = 0;

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);

				vres[del] = R_VREG_S(VS2REG, sel);
				W_ACCUM_L(del, vres[del]);

				if (R_VREG_S(VS1REG, del) == R_VREG_S(VS2REG, sel))
				{
					if (ZERO_FLAG(del) == 0)
					{
						SET_COMPARE_FLAG(del);
					}
				}
			}

			CLEAR_ZERO_FLAGS();
			CLEAR_CARRY_FLAGS();
			WRITEBACK_RESULT();
			break;
		}

		case 0x22:
		{

			rsp.flag[1] = 0;

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);

				vres[del] = R_VREG_S(VS1REG, del);
				W_ACCUM_L(del, vres[del]);

				if (R_VREG_S(VS1REG, del) != R_VREG_S(VS2REG, sel))
				{
					SET_COMPARE_FLAG(del);
				}
				else
				{
					if (ZERO_FLAG(del) != 0)
					{
						SET_COMPARE_FLAG(del);
					}
				}
			}

			CLEAR_ZERO_FLAGS();
			CLEAR_CARRY_FLAGS();
			WRITEBACK_RESULT();
			break;
		}

		case 0x23:
		{

			rsp.flag[1] = 0;

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);

				if (R_VREG_S(VS1REG, del) == R_VREG_S(VS2REG, sel))
				{
					if (ZERO_FLAG(del) == 0 || CARRY_FLAG(del) == 0)
					{
						SET_COMPARE_FLAG(del);
					}
				}
				else if (R_VREG_S(VS1REG, del) > R_VREG_S(VS2REG, sel))
				{
					SET_COMPARE_FLAG(del);
				}

				if (COMPARE_FLAG(del) != 0)
				{
					vres[del] = R_VREG_S(VS1REG, del);
				}
				else
				{
					vres[del] = R_VREG_S(VS2REG, sel);
				}

				W_ACCUM_L(del, vres[del]);
			}

			CLEAR_ZERO_FLAGS();
			CLEAR_CARRY_FLAGS();
			WRITEBACK_RESULT();
			break;
		}

		case 0x24:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT16 s1 = R_VREG_S(VS1REG, del);
				INT16 s2 = R_VREG_S(VS2REG, sel);

				if (CARRY_FLAG(del) != 0)
				{
					if (ZERO_FLAG(del) != 0)
					{
						if (COMPARE_FLAG(del) != 0)
						{
							W_ACCUM_L(del, -(UINT16)s2);
						}
						else
						{
							W_ACCUM_L(del, s1);
						}
					}
					else
					{
						if (rsp.flag[2] & (1 << (del)))
						{
							if (((UINT32)(INT16)(s1) + (UINT32)(INT16)(s2)) > 0x10000)
							{
								W_ACCUM_L(del, s1);
								CLEAR_COMPARE_FLAG(del);
							}
							else
							{
								W_ACCUM_L(del, -((UINT16)s2));
								SET_COMPARE_FLAG(del);
							}
						}
						else
						{
							if (((UINT32)(INT16)(s1) + (UINT32)(INT16)(s2)) != 0)
							{
								W_ACCUM_L(del, s1);
								CLEAR_COMPARE_FLAG(del);
							}
							else
							{
								W_ACCUM_L(del, -((UINT16)s2));
								SET_COMPARE_FLAG(del);
							}
						}
					}
				}
				else
				{
					if (ZERO_FLAG(del) != 0)
					{
						if (rsp.flag[1] & (1 << (8+del)))
						{
							W_ACCUM_L(del, s2);
						}
						else
						{
							W_ACCUM_L(del, s1);
						}
					}
					else
					{
						if (((INT32)(UINT16)s1 - (INT32)(UINT16)s2) >= 0)
						{
							W_ACCUM_L(del, s2);
							rsp.flag[1] |= (1 << (8+del));
						}
						else
						{
							W_ACCUM_L(del, s1);
							rsp.flag[1] &= ~(1 << (8+del));
						}
					}
				}

				vres[del] = R_ACCUM_L(del);
			}
			CLEAR_ZERO_FLAGS();
			CLEAR_CARRY_FLAGS();
			rsp.flag[2] = 0;
			WRITEBACK_RESULT();
			break;
		}

		case 0x25:
		{

			CLEAR_ZERO_FLAGS();
			CLEAR_CARRY_FLAGS();
			rsp.flag[1] = 0;
			rsp.flag[2] = 0;

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT16 s1 = R_VREG_S(VS1REG, del);
				INT16 s2 = R_VREG_S(VS2REG, sel);

				if ((s1 ^ s2) < 0)
				{
					SET_CARRY_FLAG(del);
					if (s2 < 0)
					{
						rsp.flag[1] |= (1 << (8+del));
					}

					if (s1 + s2 <= 0)
					{
						if (s1 + s2 == -1)
						{
							rsp.flag[2] |= (1 << (del));
						}
						SET_COMPARE_FLAG(del);
						vres[del] = -((UINT16)s2);
					}
					else
					{
						vres[del] = s1;
					}

					if (s1 + s2 != 0)
					{
						if (s1 != ~s2)
						{
							SET_ZERO_FLAG(del);
						}
					}
				}
				else
				{
					if (s2 < 0)
					{
						SET_COMPARE_FLAG(del);
					}
					if (s1 - s2 >= 0)
					{
						rsp.flag[1] |= (1 << (8+del));
						vres[del] = s2;
					}
					else
					{
						vres[del] = s1;
					}

					if ((s1 - s2) != 0)
					{
						if (s1 != ~s2)
						{
							SET_ZERO_FLAG(del);
						}
					}
				}

				W_ACCUM_L(del, vres[del]);
			}
			WRITEBACK_RESULT();
			break;
		}

		case 0x26:
		{

			rsp.flag[0] = 0;
			rsp.flag[1] = 0;
			rsp.flag[2] = 0;

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				INT16 s1 = R_VREG_S(VS1REG, del);
				INT16 s2 = R_VREG_S(VS2REG, sel);

				if ((INT16)(s1 ^ s2) < 0)
				{
					if (s2 < 0)
					{
						rsp.flag[1] |= (1 << (8+del));
					}
					if ((s1 + s2) <= 0)
					{
						W_ACCUM_L(del, ~((UINT16)s2));
						SET_COMPARE_FLAG(del);
					}
					else
					{
						W_ACCUM_L(del, s1);
					}
				}
				else
				{
					if (s2 < 0)
					{
						SET_COMPARE_FLAG(del);
					}
					if ((s1 - s2) >= 0)
					{
						W_ACCUM_L(del, s2);
						rsp.flag[1] |= (1 << (8+del));
					}
					else
					{
						W_ACCUM_L(del, s1);
					}
				}

				vres[del] = R_ACCUM_L(del);
			}
			WRITEBACK_RESULT();
			break;
		}

		case 0x27:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				if (COMPARE_FLAG(del) != 0)
				{
					vres[del] = R_VREG_S(VS1REG, del);
				}
				else
				{
					vres[del] = R_VREG_S(VS2REG, VEC_EL_2(EL, sel));
				}

				W_ACCUM_L(del, vres[del]);
			}
			WRITEBACK_RESULT();
			break;
		}
		case 0x28:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				vres[del] = R_VREG_S(VS1REG, del) & R_VREG_S(VS2REG, sel);
				W_ACCUM_L(del, vres[del]);
			}
			WRITEBACK_RESULT();
			break;
		}
		case 0x29:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				vres[del] = ~((R_VREG_S(VS1REG, del) & R_VREG_S(VS2REG, sel)));
				W_ACCUM_L(del, vres[del]);
			}
			WRITEBACK_RESULT();
			break;
		}
		case 0x2a:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				vres[del] = R_VREG_S(VS1REG, del) | R_VREG_S(VS2REG, sel);
				W_ACCUM_L(del, vres[del]);
			}
			WRITEBACK_RESULT();
			break;
		}
		case 0x2b:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				vres[del] = ~((R_VREG_S(VS1REG, del) | R_VREG_S(VS2REG, sel)));
				W_ACCUM_L(del, vres[del]);
			}
			WRITEBACK_RESULT();
			break;
		}
		case 0x2c:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				vres[del] = R_VREG_S(VS1REG, del) ^ R_VREG_S(VS2REG, sel);
				W_ACCUM_L(del, vres[del]);
			}
			WRITEBACK_RESULT();
			break;
		}
		case 0x2d:
		{

			for (i=0; i < 8; i++)
			{
				int del = VEC_EL_1(EL, i);
				int sel = VEC_EL_2(EL, del);
				vres[del] = ~((R_VREG_S(VS1REG, del) ^ R_VREG_S(VS2REG, sel)));
				W_ACCUM_L(del, vres[del]);
			}
			WRITEBACK_RESULT();
			break;
		}

		case 0x30:
		{

			int del = (VS1REG & 7);
			int sel = VEC_EL_2(EL, del);
			INT32 rec;

			rec = (INT16)(R_VREG_S(VS2REG, sel));

			if (rec == 0)
			{

				rec = 0x7fffffff;
			}
			else
			{
				int negative = 0;
				if (rec < 0)
				{
					rec = ~rec+1;
					negative = 1;
				}
				for (i = 15; i > 0; i--)
				{
					if (rec & (1 << i))
					{
						rec &= ((0xffc0) >> (15 - i));
						i = 0;
					}
				}
                rec = (INT32)(0x7fffffff / (double)rec);
				for (i = 31; i > 0; i--)
				{
					if (rec & (1 << i))
					{
						rec &= ((0xffff8000) >> (31 - i));
						i = 0;
					}
				}
				if (negative)
				{
					rec = ~rec;
				}
			}

			for (i=0; i < 8; i++)
			{
				int element = VEC_EL_2(EL, i);
				W_ACCUM_L(i, R_VREG_S(VS2REG, element));
			}

			rsp.reciprocal_res = rec;

			W_VREG_S(VDREG, del, (UINT16)(rsp.reciprocal_res));
			break;
		}

		case 0x31:
		{

			int del = (VS1REG & 7);
			int sel = VEC_EL_2(EL, del);
			INT32 rec;

			rec = ((UINT16)(R_VREG_S(VS2REG, sel)) | ((UINT32)(rsp.reciprocal_high) << 16));

			if (rec == 0)
			{

				rec = 0x7fffffff;
			}
			else
			{
				int negative = 0;
				if (rec < 0)
				{
					if (((UINT32)(rec & 0xffff0000) == 0xffff0000) && ((INT16)(rec & 0xffff) < 0))
					{
						rec = ~rec+1;
					}
					else
					{
						rec = ~rec;
					}
					negative = 1;
				}
				for (i = 31; i > 0; i--)
				{
					if (rec & (1 << i))
					{
						rec &= ((0xffc00000) >> (31 - i));
						i = 0;
					}
				}
				rec = (0x7fffffff / rec);
				for (i = 31; i > 0; i--)
				{
					if (rec & (1 << i))
					{
						rec &= ((0xffff8000) >> (31 - i));
						i = 0;
					}
				}
				if (negative)
				{
					rec = ~rec;
				}
			}

			for (i=0; i < 8; i++)
			{
				int element = VEC_EL_2(EL, i);
				W_ACCUM_L(i, R_VREG_S(VS2REG, element));
			}

			rsp.reciprocal_res = rec;

			W_VREG_S(VDREG, del, (UINT16)(rsp.reciprocal_res));
			break;
		}

		case 0x32:
		{

			int del = (VS1REG & 7);
			int sel = VEC_EL_2(EL, del);

			rsp.reciprocal_high = R_VREG_S(VS2REG, sel);

			for (i=0; i < 8; i++)
			{
				int element = VEC_EL_2(EL, i);
				W_ACCUM_L(i, R_VREG_S(VS2REG, element));
			}

			W_VREG_S(VDREG, del, (INT16)(rsp.reciprocal_res >> 16));
			break;
		}

		case 0x33:
		{

			int element = VS1REG & 7;
			W_VREG_S(VDREG, element, R_VREG_S(VS2REG, VEC_EL_2(EL, 7-element)));
			break;
		}

		case 0x35:
		{

			int del = (VS1REG & 7);
			int sel = VEC_EL_2(EL, del);
			INT32 sqr;

			sqr = (UINT16)(R_VREG_S(VS2REG, sel)) | ((UINT32)(rsp.square_root_high) << 16);

			if (sqr == 0)
			{

				sqr = 0x7fffffff;
			}
			else if (sqr == 0xffff8000)
			{

				sqr = 0xffff8000;
			}
			else
			{
				int negative = 0;
				if (sqr < 0)
				{
					if (((UINT32)(sqr & 0xffff0000) == 0xffff0000) && ((INT16)(sqr & 0xffff) < 0))
					{
						sqr = ~sqr+1;
					}
					else
					{
						sqr = ~sqr;
					}
					negative = 1;
				}
				for (i = 31; i > 0; i--)
				{
					if (sqr & (1 << i))
					{
						sqr &= (0xff800000 >> (31 - i));
						i = 0;
					}
				}
				sqr = (INT32)(0x7fffffff / sqrt(sqr));
				for (i = 31; i > 0; i--)
				{
					if (sqr & (1 << i))
					{
						sqr &= (0xffff8000 >> (31 - i));
						i = 0;
					}
				}
				if (negative)
				{
					sqr = ~sqr;
				}
			}

			for (i=0; i < 8; i++)
			{
				int element = VEC_EL_2(EL, i);
				W_ACCUM_L(i, R_VREG_S(VS2REG, element));
			}

			rsp.square_root_res = sqr;

			W_VREG_S(VDREG, del, (UINT16)(rsp.square_root_res));
			break;
		}

		case 0x36:
		{

			int del = (VS1REG & 7);
			int sel = VEC_EL_2(EL, del);

			rsp.square_root_high = R_VREG_S(VS2REG, sel);

			for (i=0; i < 8; i++)
			{
				int element = VEC_EL_2(EL, i);
				W_ACCUM_L(i, R_VREG_S(VS2REG, element));
			}

			W_VREG_S(VDREG, del, (INT16)(rsp.square_root_res >> 16));
			break;
		}

		default:	unimplemented_opcode(op); break;
	}
}

static int rsp_execute(int cycles)
{
	UINT32 op;

	rsp_icount = cycles;

	rsp.pc = 0x4001000 | (rsp.pc & 0xfff);

    if( rsp.sr & ( RSP_STATUS_HALT | RSP_STATUS_BROKE ) )
    {
        rsp_icount = 0;
    }

	while (rsp_icount > 0)
	{
		rsp.ppc = rsp.pc;
		CALL_MAME_DEBUG;

		op = ROPCODE(rsp.pc);
		if (rsp.nextpc != ~0)
		{
			rsp.pc = rsp.nextpc;
			rsp.nextpc = ~0;
		}
		else
		{
			rsp.pc += 4;
		}

		switch (op >> 26)
		{
			case 0x00:
			{
				switch (op & 0x3f)
				{
					case 0x00:			if (RDREG) RDVAL = (UINT32)RTVAL << SHIFT; break;
					case 0x02:			if (RDREG) RDVAL = (UINT32)RTVAL >> SHIFT; break;
					case 0x03:			if (RDREG) RDVAL = (INT32)RTVAL >> SHIFT; break;
					case 0x04:			if (RDREG) RDVAL = (UINT32)RTVAL << (RSVAL & 0x1f); break;
					case 0x06:			if (RDREG) RDVAL = (UINT32)RTVAL >> (RSVAL & 0x1f); break;
					case 0x07:			if (RDREG) RDVAL = (INT32)RTVAL >> (RSVAL & 0x1f); break;
					case 0x08:			JUMP_PC(RSVAL); break;
					case 0x09:			JUMP_PC_L(RSVAL, RDREG); break;
					case 0x0d:
					{
						sp_set_status(0x3);
						rsp_icount = 1;

#if LOG_INSTRUCTION_EXECUTION
						fprintf(exec_output, "\n---------- break ----------\n\n");
#endif
						break;
					}
					case 0x20:			if (RDREG) RDVAL = (INT32)(RSVAL + RTVAL); break;
					case 0x21:			if (RDREG) RDVAL = (INT32)(RSVAL + RTVAL); break;
					case 0x22:			if (RDREG) RDVAL = (INT32)(RSVAL - RTVAL); break;
					case 0x23:			if (RDREG) RDVAL = (INT32)(RSVAL - RTVAL); break;
					case 0x24:			if (RDREG) RDVAL = RSVAL & RTVAL; break;
					case 0x25:			if (RDREG) RDVAL = RSVAL | RTVAL; break;
					case 0x26:			if (RDREG) RDVAL = RSVAL ^ RTVAL; break;
					case 0x27:			if (RDREG) RDVAL = ~(RSVAL | RTVAL); break;
					case 0x2a:			if (RDREG) RDVAL = (INT32)RSVAL < (INT32)RTVAL; break;
					case 0x2b:			if (RDREG) RDVAL = (UINT32)RSVAL < (UINT32)RTVAL; break;
					default:	unimplemented_opcode(op); break;
				}
				break;
			}

			case 0x01:
			{
				switch (RTREG)
				{
					case 0x00:			if ((INT32)(RSVAL) < 0) JUMP_REL(SIMM16); break;
					case 0x01:			if ((INT32)(RSVAL) >= 0) JUMP_REL(SIMM16); break;
					case 0x11:		if ((INT32)(RSVAL) >= 0) JUMP_REL_L(SIMM16, 31); break;
					default:	unimplemented_opcode(op); break;
				}
				break;
			}

			case 0x02:				JUMP_ABS(UIMM26); break;
			case 0x03:			JUMP_ABS_L(UIMM26, 31); break;
			case 0x04:			if (RSVAL == RTVAL) JUMP_REL(SIMM16); break;
			case 0x05:			if (RSVAL != RTVAL) JUMP_REL(SIMM16); break;
			case 0x06:			if ((INT32)RSVAL <= 0) JUMP_REL(SIMM16); break;
			case 0x07:			if ((INT32)RSVAL > 0) JUMP_REL(SIMM16); break;
			case 0x08:			if (RTREG) RTVAL = (INT32)(RSVAL + SIMM16); break;
			case 0x09:			if (RTREG) RTVAL = (INT32)(RSVAL + SIMM16); break;
			case 0x0a:			if (RTREG) RTVAL = (INT32)(RSVAL) < ((INT32)SIMM16); break;
			case 0x0b:			if (RTREG) RTVAL = (UINT32)(RSVAL) < (UINT32)((INT32)SIMM16); break;
			case 0x0c:			if (RTREG) RTVAL = RSVAL & UIMM16; break;
			case 0x0d:			if (RTREG) RTVAL = RSVAL | UIMM16; break;
			case 0x0e:			if (RTREG) RTVAL = RSVAL ^ UIMM16; break;
			case 0x0f:			if (RTREG) RTVAL = UIMM16 << 16; break;

			case 0x10:
			{
				switch ((op >> 21) & 0x1f)
				{
					case 0x00:			if (RTREG) RTVAL = get_cop0_reg(RDREG); break;
					case 0x04:			set_cop0_reg(RDREG, RTVAL); break;
					default:	unimplemented_opcode(op); break;
				}
				break;
			}

			case 0x12:
			{
				switch ((op >> 21) & 0x1f)
				{
					case 0x00:
					{

						int el = (op >> 7) & 0xf;
						UINT16 b1 = R_VREG_B(VS1REG, (el+0) & 0xf);
						UINT16 b2 = R_VREG_B(VS1REG, (el+1) & 0xf);
						if (RTREG) RTVAL = (INT32)(INT16)((b1 << 8) | (b2));
						break;
					}
					case 0x02:
					{

						if (RTREG) RTVAL = rsp.flag[RDREG];
						break;
					}
					case 0x04:
					{

						int el = (op >> 7) & 0xf;
						W_VREG_B(VS1REG, (el+0) & 0xf, (RTVAL >> 8) & 0xff);
						W_VREG_B(VS1REG, (el+1) & 0xf, (RTVAL >> 0) & 0xff);
						break;
					}
					case 0x06:
					{

						rsp.flag[RDREG] = RTVAL & 0xffff;
						break;
					}

					case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
					case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
					{
						handle_vector_ops(op);
						break;
					}

					default:	unimplemented_opcode(op); break;
				}
				break;
			}

			case 0x20:			if (RTREG) RTVAL = (INT32)(INT8)READ8(RSVAL + SIMM16); break;
			case 0x21:			if (RTREG) RTVAL = (INT32)(INT16)READ16(RSVAL + SIMM16); break;
			case 0x23:			if (RTREG) RTVAL = READ32(RSVAL + SIMM16); break;
			case 0x24:			if (RTREG) RTVAL = (UINT8)READ8(RSVAL + SIMM16); break;
			case 0x25:			if (RTREG) RTVAL = (UINT16)READ16(RSVAL + SIMM16); break;
			case 0x28:			WRITE8(RSVAL + SIMM16, RTVAL); break;
			case 0x29:			WRITE16(RSVAL + SIMM16, RTVAL); break;
			case 0x2b:			WRITE32(RSVAL + SIMM16, RTVAL); break;
			case 0x32:			handle_lwc2(op); break;
			case 0x3a:			handle_swc2(op); break;

			default:
			{
				unimplemented_opcode(op);
				break;
			}
		}

#if LOG_INSTRUCTION_EXECUTION
		{
			int i, l;
			static UINT32 prev_regs[32];
			static VECTOR_REG prev_vecs[32];
			char string[200];
			rsp_dasm_one(string, rsp.ppc, op);

			fprintf(exec_output, "%08X: %s", rsp.ppc, string);

			l = strlen(string);
			if (l < 36)
			{
				for (i=l; i < 36; i++)
				{
					fprintf(exec_output, " ");
				}
			}

			fprintf(exec_output, "| ");

			for (i=0; i < 32; i++)
			{
				if (rsp.r[i] != prev_regs[i])
				{
					fprintf(exec_output, "R%d: %08X ", i, rsp.r[i]);
				}
				prev_regs[i] = rsp.r[i];
			}

			for (i=0; i < 32; i++)
			{
				if (rsp.v[i].d[0] != prev_vecs[i].d[0] || rsp.v[i].d[1] != prev_vecs[i].d[1])
				{
					fprintf(exec_output, "V%d: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X ", i,
					(UINT16)R_VREG_S(i,0), (UINT16)R_VREG_S(i,1), (UINT16)R_VREG_S(i,2), (UINT16)R_VREG_S(i,3), (UINT16)R_VREG_S(i,4), (UINT16)R_VREG_S(i,5), (UINT16)R_VREG_S(i,6), (UINT16)R_VREG_S(i,7));
				}
				prev_vecs[i].d[0] = rsp.v[i].d[0];
				prev_vecs[i].d[1] = rsp.v[i].d[1];
			}

			fprintf(exec_output, "\n");

		}
#endif

		--rsp_icount;

        if( rsp.sr & RSP_STATUS_SSTEP )
        {
            rsp.sr |= RSP_STATUS_BROKE;
        }

        if( rsp.sr & ( RSP_STATUS_HALT | RSP_STATUS_BROKE ) )
        {
            rsp_icount = 0;
        }

	}

	return cycles - rsp_icount;
}

static void rsp_get_context(void *dst)
{

	if (dst)
		*(RSP_REGS *)dst = rsp;
}

static void rsp_set_context(void *src)
{

	if (src)
		rsp = *(RSP_REGS *)src;
}

#ifdef MAME_DEBUG
static offs_t rsp_dasm(char *buffer, offs_t pc, UINT8 *oprom, UINT8 *opram, int bytes)
{
	UINT32 op = LITTLE_ENDIANIZE_INT32(*(UINT32 *)opram);
	return rsp_dasm_one(buffer, pc, op);
}
#endif

static void rsp_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{

		case CPUINFO_INT_PC:
        case CPUINFO_INT_REGISTER + RSP_PC:             rsp.pc = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R0:             rsp.r[0] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R1:             rsp.r[1] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R2:             rsp.r[2] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R3:             rsp.r[3] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R4:             rsp.r[4] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R5:             rsp.r[5] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R6:             rsp.r[6] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R7:             rsp.r[7] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R8:             rsp.r[8] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R9:             rsp.r[9] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R10:            rsp.r[10] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R11:            rsp.r[11] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R12:            rsp.r[12] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R13:            rsp.r[13] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R14:            rsp.r[14] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R15:            rsp.r[15] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R16:            rsp.r[16] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R17:            rsp.r[17] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R18:            rsp.r[18] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R19:            rsp.r[19] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R20:            rsp.r[20] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R21:            rsp.r[21] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R22:            rsp.r[22] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R23:            rsp.r[23] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R24:            rsp.r[24] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R25:            rsp.r[25] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R26:            rsp.r[26] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R27:            rsp.r[27] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R28:            rsp.r[28] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R29:            rsp.r[29] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_R30:            rsp.r[30] = info->i;        break;
        case CPUINFO_INT_SP:
        case CPUINFO_INT_REGISTER + RSP_R31:            rsp.r[31] = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_SR:             rsp.sr = info->i;        break;
        case CPUINFO_INT_REGISTER + RSP_NEXTPC:         rsp.nextpc = info->i;        break;
	}
}

void rsp_get_info(UINT32 state, union cpuinfo *info)
{
	switch(state)
	{

		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(rsp);					break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 1;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 1;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE:					info->i = CLEAR_LINE;					break;

		case CPUINFO_INT_PREVIOUSPC:					info->i = rsp.ppc;						break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + RSP_PC:				info->i = rsp.pc;						break;

		case CPUINFO_INT_REGISTER + RSP_R0:				info->i = rsp.r[0];						break;
		case CPUINFO_INT_REGISTER + RSP_R1:				info->i = rsp.r[1];						break;
		case CPUINFO_INT_REGISTER + RSP_R2:				info->i = rsp.r[2];						break;
		case CPUINFO_INT_REGISTER + RSP_R3:				info->i = rsp.r[3];						break;
		case CPUINFO_INT_REGISTER + RSP_R4:				info->i = rsp.r[4];						break;
		case CPUINFO_INT_REGISTER + RSP_R5:				info->i = rsp.r[5];						break;
		case CPUINFO_INT_REGISTER + RSP_R6:				info->i = rsp.r[6];						break;
		case CPUINFO_INT_REGISTER + RSP_R7:				info->i = rsp.r[7];						break;
		case CPUINFO_INT_REGISTER + RSP_R8:				info->i = rsp.r[8];						break;
		case CPUINFO_INT_REGISTER + RSP_R9:				info->i = rsp.r[9];						break;
		case CPUINFO_INT_REGISTER + RSP_R10:			info->i = rsp.r[10];					break;
		case CPUINFO_INT_REGISTER + RSP_R11:			info->i = rsp.r[11];					break;
		case CPUINFO_INT_REGISTER + RSP_R12:			info->i = rsp.r[12];					break;
		case CPUINFO_INT_REGISTER + RSP_R13:			info->i = rsp.r[13];					break;
		case CPUINFO_INT_REGISTER + RSP_R14:			info->i = rsp.r[14];					break;
		case CPUINFO_INT_REGISTER + RSP_R15:			info->i = rsp.r[15];					break;
		case CPUINFO_INT_REGISTER + RSP_R16:			info->i = rsp.r[16];					break;
		case CPUINFO_INT_REGISTER + RSP_R17:			info->i = rsp.r[17];					break;
		case CPUINFO_INT_REGISTER + RSP_R18:			info->i = rsp.r[18];					break;
		case CPUINFO_INT_REGISTER + RSP_R19:			info->i = rsp.r[19];					break;
		case CPUINFO_INT_REGISTER + RSP_R20:			info->i = rsp.r[20];					break;
		case CPUINFO_INT_REGISTER + RSP_R21:			info->i = rsp.r[21];					break;
		case CPUINFO_INT_REGISTER + RSP_R22:			info->i = rsp.r[22];					break;
		case CPUINFO_INT_REGISTER + RSP_R23:			info->i = rsp.r[23];					break;
		case CPUINFO_INT_REGISTER + RSP_R24:			info->i = rsp.r[24];					break;
		case CPUINFO_INT_REGISTER + RSP_R25:			info->i = rsp.r[25];					break;
		case CPUINFO_INT_REGISTER + RSP_R26:			info->i = rsp.r[26];					break;
		case CPUINFO_INT_REGISTER + RSP_R27:			info->i = rsp.r[27];					break;
		case CPUINFO_INT_REGISTER + RSP_R28:			info->i = rsp.r[28];					break;
		case CPUINFO_INT_REGISTER + RSP_R29:			info->i = rsp.r[29];					break;
		case CPUINFO_INT_REGISTER + RSP_R30:			info->i = rsp.r[30];					break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + RSP_R31:			info->i = rsp.r[31];					break;
        case CPUINFO_INT_REGISTER + RSP_SR:             info->i = rsp.sr;                       break;
        case CPUINFO_INT_REGISTER + RSP_NEXTPC:         info->i = rsp.nextpc;                   break;

		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = rsp_get_context;		break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = rsp_set_context;		break;
		case CPUINFO_PTR_SET_INFO:						info->setinfo = rsp_set_info;			break;
		case CPUINFO_PTR_INIT:							info->init = rsp_init;					break;
		case CPUINFO_PTR_RESET:							info->reset = rsp_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = rsp_exit;					break;
		case CPUINFO_PTR_EXECUTE:						info->execute = rsp_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
#ifdef MAME_DEBUG
		case CPUINFO_PTR_DISASSEMBLE_NEW:				info->disassemble_new = rsp_dasm;			break;
#endif
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &rsp_icount;				break;

		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "RSP");					break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "RSP");					break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.0");					break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__);				break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (C) 2005");	break;

		case CPUINFO_STR_FLAGS:							strcpy(info->s = cpuintrf_temp_str(), " ");					break;

		case CPUINFO_STR_REGISTER + RSP_PC:				sprintf(info->s = cpuintrf_temp_str(), "PC: %08X", rsp.pc);	break;

		case CPUINFO_STR_REGISTER + RSP_R0:				sprintf(info->s = cpuintrf_temp_str(), "R0: %08X", rsp.r[0]); break;
		case CPUINFO_STR_REGISTER + RSP_R1:				sprintf(info->s = cpuintrf_temp_str(), "R1: %08X", rsp.r[1]); break;
		case CPUINFO_STR_REGISTER + RSP_R2:				sprintf(info->s = cpuintrf_temp_str(), "R2: %08X", rsp.r[2]); break;
		case CPUINFO_STR_REGISTER + RSP_R3:				sprintf(info->s = cpuintrf_temp_str(), "R3: %08X", rsp.r[3]); break;
		case CPUINFO_STR_REGISTER + RSP_R4:				sprintf(info->s = cpuintrf_temp_str(), "R4: %08X", rsp.r[4]); break;
		case CPUINFO_STR_REGISTER + RSP_R5:				sprintf(info->s = cpuintrf_temp_str(), "R5: %08X", rsp.r[5]); break;
		case CPUINFO_STR_REGISTER + RSP_R6:				sprintf(info->s = cpuintrf_temp_str(), "R6: %08X", rsp.r[6]); break;
		case CPUINFO_STR_REGISTER + RSP_R7:				sprintf(info->s = cpuintrf_temp_str(), "R7: %08X", rsp.r[7]); break;
		case CPUINFO_STR_REGISTER + RSP_R8:				sprintf(info->s = cpuintrf_temp_str(), "R8: %08X", rsp.r[8]); break;
		case CPUINFO_STR_REGISTER + RSP_R9:				sprintf(info->s = cpuintrf_temp_str(), "R9: %08X", rsp.r[9]); break;
		case CPUINFO_STR_REGISTER + RSP_R10:			sprintf(info->s = cpuintrf_temp_str(), "R10: %08X", rsp.r[10]); break;
		case CPUINFO_STR_REGISTER + RSP_R11:			sprintf(info->s = cpuintrf_temp_str(), "R11: %08X", rsp.r[11]); break;
		case CPUINFO_STR_REGISTER + RSP_R12:			sprintf(info->s = cpuintrf_temp_str(), "R12: %08X", rsp.r[12]); break;
		case CPUINFO_STR_REGISTER + RSP_R13:			sprintf(info->s = cpuintrf_temp_str(), "R13: %08X", rsp.r[13]); break;
		case CPUINFO_STR_REGISTER + RSP_R14:			sprintf(info->s = cpuintrf_temp_str(), "R14: %08X", rsp.r[14]); break;
		case CPUINFO_STR_REGISTER + RSP_R15:			sprintf(info->s = cpuintrf_temp_str(), "R15: %08X", rsp.r[15]); break;
		case CPUINFO_STR_REGISTER + RSP_R16:			sprintf(info->s = cpuintrf_temp_str(), "R16: %08X", rsp.r[16]); break;
		case CPUINFO_STR_REGISTER + RSP_R17:			sprintf(info->s = cpuintrf_temp_str(), "R17: %08X", rsp.r[17]); break;
		case CPUINFO_STR_REGISTER + RSP_R18:			sprintf(info->s = cpuintrf_temp_str(), "R18: %08X", rsp.r[18]); break;
		case CPUINFO_STR_REGISTER + RSP_R19:			sprintf(info->s = cpuintrf_temp_str(), "R19: %08X", rsp.r[19]); break;
		case CPUINFO_STR_REGISTER + RSP_R20:			sprintf(info->s = cpuintrf_temp_str(), "R20: %08X", rsp.r[20]); break;
		case CPUINFO_STR_REGISTER + RSP_R21:			sprintf(info->s = cpuintrf_temp_str(), "R21: %08X", rsp.r[21]); break;
		case CPUINFO_STR_REGISTER + RSP_R22:			sprintf(info->s = cpuintrf_temp_str(), "R22: %08X", rsp.r[22]); break;
		case CPUINFO_STR_REGISTER + RSP_R23:			sprintf(info->s = cpuintrf_temp_str(), "R23: %08X", rsp.r[23]); break;
		case CPUINFO_STR_REGISTER + RSP_R24:			sprintf(info->s = cpuintrf_temp_str(), "R24: %08X", rsp.r[24]); break;
		case CPUINFO_STR_REGISTER + RSP_R25:			sprintf(info->s = cpuintrf_temp_str(), "R25: %08X", rsp.r[25]); break;
		case CPUINFO_STR_REGISTER + RSP_R26:			sprintf(info->s = cpuintrf_temp_str(), "R26: %08X", rsp.r[26]); break;
		case CPUINFO_STR_REGISTER + RSP_R27:			sprintf(info->s = cpuintrf_temp_str(), "R27: %08X", rsp.r[27]); break;
		case CPUINFO_STR_REGISTER + RSP_R28:			sprintf(info->s = cpuintrf_temp_str(), "R28: %08X", rsp.r[28]); break;
		case CPUINFO_STR_REGISTER + RSP_R29:			sprintf(info->s = cpuintrf_temp_str(), "R29: %08X", rsp.r[29]); break;
		case CPUINFO_STR_REGISTER + RSP_R30:			sprintf(info->s = cpuintrf_temp_str(), "R30: %08X", rsp.r[30]); break;
		case CPUINFO_STR_REGISTER + RSP_R31:			sprintf(info->s = cpuintrf_temp_str(), "R31: %08X", rsp.r[31]); break;
        case CPUINFO_STR_REGISTER + RSP_SR:             sprintf(info->s = cpuintrf_temp_str(), "SR: %08X",  rsp.sr);    break;
        case CPUINFO_STR_REGISTER + RSP_NEXTPC:         sprintf(info->s = cpuintrf_temp_str(), "NPC: %08X", rsp.nextpc);break;
	}
}

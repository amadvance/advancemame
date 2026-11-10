// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/* Cube Quest AM2901-based sound, rotation and line processors. */

#include "driver.h"
#include "debugger.h"
#include "cubeqcpu.h"

enum alu_src
{
	AQ = 0,
	AB = 1,
	ZQ = 2,
	ZB = 3,
	ZA = 4,
	DA = 5,
	DQ = 6,
	DZ = 7
};

enum alu_ins
{
	ADD   = 0,
	SUBR  = 1,
	SUBS  = 2,
	OR    = 3,
	AND   = 4,
	NOTRS = 5,
	EXOR  = 6,
	EXNOR = 7
};

enum alu_dst
{
	QREG  = 0,
	NOP   = 1,
	RAMA  = 2,
	RAMF  = 3,
	RAMQD = 4,
	RAMD  = 5,
	RAMQU = 6,
	RAMU  = 7
};

#define FOREGROUND 0
#define BACKGROUND 1
#define ODD_FIELD 0
#define EVEN_FIELD 1
static struct
{
	UINT16  ram[16];
	UINT16  q;
	UINT16  f;
	UINT16  y;
	UINT32  cflag;
	UINT32  vflag;

	UINT8   pc;
	UINT16  platch;
	UINT8   rtnlatch;
	UINT8   adrcntr;
	UINT16  adrlatch;
	UINT16  dinlatch;
	UINT16  ramwlatch;

	UINT16 *sram;

	int prev_ipram;
	int prev_ipwrt;

	UINT16 *sound_data;

	int icount;

	UINT64 *rom;
} snd;

static struct
{
	UINT16  ram[16];
	UINT16  q;
	UINT16  f;
	UINT16  y;
	UINT32  cflag;
	UINT32  vflag;

	UINT16  pc;
	UINT8   seqcnt;

	UINT8   dsrclatch;
	UINT8   rsrclatch;
	UINT16  dynaddr;
	UINT16  dyndata;
	UINT16  yrlatch;
	UINT16  ydlatch;
	UINT16  dinlatch;
	UINT8   divreg;

	UINT16  linedata;
	UINT16  lineaddr;

	UINT16 *dram;
	UINT16 *sram;

	UINT8 prev_dred;
	UINT8 prev_dwrt;
	UINT8 wc;
	UINT8 rc;
	UINT8 clkcnt;

	int icount;

	UINT8 flags;

	UINT64 *rom;
} rot;

static struct
{
	UINT16  ram[16];
	UINT16  q;
	UINT16  f;
	UINT16  y;
	UINT32  cflag;
	UINT32  vflag;

	UINT8   pc[2];

	UINT16  seqcnt;
	UINT16  clatch;
	UINT8   zlatch;

	UINT16  xcnt;
	UINT16  ycnt;
	UINT8   sreg;

	UINT16  fadlatch;
	UINT16  badlatch;

	UINT16  sramdlatch;

	UINT8   fglatch;
	UINT8   bglatch;
	UINT8   gt0reg;
	UINT8   fdxreg;
	UINT32  field;

	UINT32  clkcnt;

	UINT16 *sram;
	UINT8 *ptr_ram;
	UINT32 *e_stack;
	UINT32 *o_stack;

	int icount;

	UINT8 flags;
	UINT16 curpc;

	UINT64 *rom;
} lin;

static void cubeqcpu_line_w(offs_t offset, UINT16 data);
static UINT16 cubeqcpu_line_r(void);
enum snd_latch_type
{
	PLTCH = 0,
	DAC = 1,
	ADLATCH = 2
};

static int snd_do_sndjmp(UINT8 jmp)
{
	switch (jmp)
	{
		 case 0: return 1;
		 case 2: return snd.f & 0x8000 ? 0 : 1;
		 case 3: return snd.f & 0x8000 ? 1 : 0;
		 case 5: return snd.f == 0 ? 0 : 1;
		 case 6: return snd.vflag ? 0 : 1;
		 case 7: return snd.adrcntr & 0x80 ? 0: 1;
	}

	return 0;
}

#define SND_PC          (snd.pc)
#define SND_DATA_IN     (_ramen ? snd.sound_data[snd.platch] : snd.dinlatch)

static int snd_execute(int cycles)
{
	snd.icount = cycles;

	do
	{
		UINT64 inst = snd.rom[SND_PC];
		UINT32 inslow = inst & 0xffffffff;
		UINT32 inshig = inst >> 32;

		UINT8 t               = (inshig >> 24) & 0xff;
		const UINT8 b         = (inshig >> 20) & 0xf;
		const UINT8 a         = (inshig >> 16) & 0xf;
		const UINT8 ci        = (inshig >> 15) & 1;
		const UINT8 i5_3      = (inshig >> 12) & 7;
		const int _ramen  = (inshig >> 11) & 1;
		const UINT8 i2_0      = (inshig >> 8) & 7;
		const int rtnltch = (inshig >> 7) & 1;
		const UINT8 jmp       = (inshig >> 4) & 7;
		const int inca    = (inshig >> 3) & 1;
		const UINT8 i8_6      = (inshig >> 0) & 7;
		const int _ipram  = (inslow >> 31) & 1;
		const int _ipwrt  = (inslow >> 30) & 1;
		const UINT8 latch     = (inslow >> 28) & 3;
		const int rtn     = (inslow >> 27) & 1;
		const int _rin    = (inslow >> 26) & 1;

		CALL_MAME_DEBUG;

		if (rtn)
			t = snd.rtnlatch;

		{
			UINT16 r = 0;
			UINT16 s = 0;

			UINT32 res = 0;
			UINT32 cflag = 0;
			UINT32 vflag = 0;

			switch (i2_0)
			{
				case AQ: r = snd.ram[a];      s = snd.q;      break;
				case AB: r = snd.ram[a];      s = snd.ram[b]; break;
				case ZQ: r = 0;             s = snd.q;      break;
				case ZB: r = 0;             s = snd.ram[b]; break;
				case ZA: r = 0;             s = snd.ram[a]; break;
				case DA: r = SND_DATA_IN;   s = snd.ram[a]; break;
				case DQ: r = SND_DATA_IN;   s = snd.q;      break;
				case DZ: r = SND_DATA_IN;   s = 0;        break;
			}

			switch (i5_3)
			{
				case ADD:
					res = r + s + ci;
					cflag = (res >> 16) & 1;
					vflag = (((r & 0x7fff) + (s & 0x7fff) + ci) >> 15) ^ cflag;
					break;
				case SUBR:
					res = ~r + s + ci;
					cflag = (res >> 16) & 1;
					vflag = (((s & 0x7fff) + (~r & 0x7fff) + ci) >> 15) ^ cflag;
					break;
				case SUBS:
					res = r + ~s + ci;
					cflag = (res >> 16) & 1;
					vflag = (((r & 0x7fff) + (~s & 0x7fff) + ci) >> 15) ^ cflag;
					break;
				case OR:
					res = r | s;
					break;
				case AND:
					res = r & s;
					break;
				case NOTRS:
					res = ~r & s;
					break;
				case EXOR:
					res = r ^ s;
					break;
				case EXNOR:
					res = ~(r ^ s);
					break;
			}

			snd.f = res;
			snd.cflag = cflag;
			snd.vflag = vflag;

			switch (i8_6)
			{
				case QREG:
					snd.q = snd.f;
					snd.y = snd.f;
					break;
				case NOP:
					snd.y = snd.f;
					break;
				case RAMA:
					snd.y = snd.ram[a];
					snd.ram[b] = snd.f;
					break;
				case RAMF:
					snd.ram[b] = snd.f;
					snd.y = snd.f;
					break;
				case RAMQD:
				{
					UINT16 qin;

					snd.ram[b] = (_rin ? 0 : 0x8000) | (snd.f >> 1);
					snd.q >>= 1;
					snd.y = snd.f;

					qin = (((snd.y >> 15) ^ (snd.y >> 1)) & 1) ? 0 : 0x8000;

					snd.q |= qin;
					break;
				}
				case RAMD:
					snd.ram[b] = (_rin ? 0 : 0x8000) | (snd.f >> 1);
					snd.y = snd.f;
					break;
				case RAMQU:
					snd.ram[b] = (snd.f << 1) | (_rin ? 0 : 0x0001);
					snd.q <<= 1;
					snd.y = snd.f;
					break;
				case RAMU:
					snd.ram[b] = (snd.f << 1) | (_rin ? 0 : 0x0001);
					snd.y = snd.f;
					break;
			}
		}

		if (!snd.prev_ipram)
		{
			UINT16 addr = snd.adrlatch | (snd.adrcntr & 0x7f);

			if (!snd.prev_ipwrt)
			snd.sram[addr] = snd.ramwlatch;
			else
			snd.dinlatch = snd.sram[addr];
		}

		if (latch == PLTCH)
		{
			snd.platch = ((t & 3) << 9) | ((snd.y >> 6) & 0x1ff);
		}
		else if (latch == DAC)
		{
			cubeqcpu_dac_w((snd.y & 0xfff0) | ((snd.adrcntr >> 3) & 0xf));
		}
		else if (latch == ADLATCH)
		{
			snd.adrcntr = snd.y & 0x7f;

			snd.adrlatch = snd.y & 0x780;
		}

		if (snd_do_sndjmp(jmp))
			snd.pc = rtn ? snd.rtnlatch : t;
		else
			snd.pc++;

		if (rtnltch)
			snd.rtnlatch = t;

		if (inca && latch != ADLATCH)
			snd.adrcntr++;

		if (!_ipwrt)
			snd.ramwlatch = snd.y;

		snd.prev_ipram = _ipram;
		snd.prev_ipwrt = _ipwrt;

		snd.icount--;
	} while (snd.icount > 0);
	return cycles - snd.icount;
}

enum rot_spf
{
	SPF_UNUSED0 = 0,
	SPF_UNUSED1 = 1,
	SPF_OP      = 2,
	SPF_RET     = 3,
	SPF_SQLTCH  = 4,
	SPF_SWRT    = 5,
	SPF_DIV     = 6,
	SPF_MULT    = 7,
	SPF_DRED    = 8,
	SPF_DWRT    = 9
};

enum rot_yout
{
	YOUT_UNUSED0 = 0,
	YOUT_UNUSED1 = 1,
	YOUT_Y2LDA   = 2,
	YOUT_Y2LDD   = 3,
	YOUT_Y2DAD   = 4,
	YOUT_Y2DYN   = 5,
	YOUT_Y2R     = 6,
	YOUT_Y2D     = 7
};

static int rot_do_rotjmp(UINT8 jmp)
{
	int ret = 0;

	switch (jmp & 7)
	{
		 case 0: ret = 0;                 break;
		 case 1: ret = (rot.seqcnt == 0xf); break;
		 case 2: ret = rot.cflag;           break;
		 case 3: ret = !(rot.clkcnt & 0x3); break;
		 case 4: ret = 0;                 break;
		 case 5: ret = BIT(rot.f, 15);      break;
		 case 6: ret = (!BIT(rot.f, 15) && !(rot.f == 0)); break;
		 case 7: ret = (rot.f == 0);        break;
	}

	return !(!ret ^ BIT(jmp, 3));
}

#define ROT_PC              (rot.pc & 0x1ff)
#define ROT_SRAM_ADDRESS    ((rot.dsrclatch & 2) ? rot.yrlatch : (rot.rsrclatch | 0x700))

static int rot_execute(int cycles)
{
	rot.icount = cycles;

	do
	{
		UINT64 inst = rot.rom[ROT_PC];

		UINT32 inslow = inst & 0xffffffff;
		UINT32 inshig = inst >> 32;

		UINT16 t              = (inshig >> 20) & 0xfff;
		const UINT8 jmp       = (inshig >> 16) & 0xf;
		const UINT8 spf       = (inshig >> 12) & 0xf;
		const int rsrc    = (inshig >> 11) & 0x1;
		const UINT8 yout      = (inshig >> 8) & 0x7;
		const UINT8 sel       = (inshig >> 6) & 0x3;
		const UINT8 dsrc      = (inshig >> 4) & 0x3;
		const UINT8 b         = (inshig >> 0) & 0xf;
		const UINT8 a         = (inslow >> 28) & 0xf;
		const UINT8 i8_6      = (inslow >> 24) & 0x7;
		UINT8 ci              = (inslow >> 23) & 0x1;
		UINT8 i5_3            = (inslow >> 20) & 0x7;
		const int _sex    = (inslow >> 19) & 0x1;
		UINT8 i2_0            = (inslow >> 16) & 0x7;

		int dsrclatch;
		UINT16 data_in = 0xffff;

		CALL_MAME_DEBUG;

		if (!(rot.clkcnt & 3))
		{
			if (rot.wc)
			{
				rot.wc = 0;
				rot.dram[rot.dynaddr & 0x3fff] = rot.dyndata;
			}
			if (rot.rc)
			{
				rot.rc = 0;
				rot.dinlatch = rot.dram[rot.dynaddr & 0x3fff];
			}
		}

		if (!rot.prev_dwrt)
			rot.wc = 1;
		else if (!rot.prev_dred)
			rot.rc = 1;

		if (~rot.dsrclatch & 0x10)
			data_in = rot.dinlatch;
		else if (~rot.dsrclatch & 0x20)
			data_in = rot.sram[ROT_SRAM_ADDRESS];
		else if (~rot.dsrclatch & 0x40)
			data_in = rot.ydlatch;
		else if (~rot.dsrclatch & 0x80)
			data_in = t & 0xfff;

		if ((spf == SPF_RET) && (rot.dsrclatch & 0x80))
			t = data_in;
		else if (spf == SPF_OP)
			t = (t & ~0xf) | (data_in >> 12);

		if (~rot.dsrclatch & 1)
			rot.sram[ROT_SRAM_ADDRESS] = data_in;

		if (!_sex)
			data_in = (data_in & ~0xf000) | ((data_in & 0x800) ? 0xf000 : 0);

		{
			UINT16 r = 0;
			UINT16 s = 0;

			UINT32 res = 0;
			UINT32 cflag = 0;
			UINT32 vflag = 0;

			if ((spf == SPF_MULT) && !BIT(rot.q, 0))
				i2_0 |= 2;

			switch (i2_0)
			{
				case 0: r = rot.ram[a]; s = rot.q;      break;
				case 1: r = rot.ram[a]; s = rot.ram[b]; break;
				case 2: r = 0;                s = rot.q;      break;
				case 3: r = 0;                s = rot.ram[b]; break;
				case 4: r = 0;                s = rot.ram[a]; break;
				case 5: r = data_in;          s = rot.ram[a]; break;
				case 6: r = data_in;          s = rot.q;      break;
				case 7: r = data_in;          s = 0;                break;
			}

			if ((spf == SPF_DIV) && rot.divreg)
			{
				i5_3 |= 1;
				ci = 1;
			}

			switch (i5_3)
			{
				case ADD:
					res = r + s + ci;
					cflag = (res >> 16) & 1;
					vflag = (((r & 0x7fff) + (s & 0x7fff) + ci) >> 15) ^ cflag;
					break;
				case SUBR:
					res = ~r + s + ci;
					cflag = (res >> 16) & 1;
					vflag = (((s & 0x7fff) + (~r & 0x7fff) + ci) >> 15) ^ cflag;
					break;
				case SUBS:
					res = r + ~s + ci;
					cflag = (res >> 16) & 1;
					vflag = (((r & 0x7fff) + (~s & 0x7fff) + ci) >> 15) ^ cflag;
					break;
				case OR:
					res = r | s;
					break;
				case AND:
					res = r & s;
					break;
				case NOTRS:
					res = ~r & s;
					break;
				case EXOR:
					res = r ^ s;
					break;
				case EXNOR:
					res = ~(r ^ s);
					break;
			}

			rot.f = res;
			rot.cflag = cflag;
			rot.vflag = vflag;

			switch (i8_6)
			{
				case QREG:
					rot.q = rot.f;
					rot.y = rot.f;
					break;
				case NOP:
					rot.y = rot.f;
					break;
				case RAMA:
					rot.y = rot.ram[a];
					rot.ram[b] = rot.f;
					break;
				case RAMF:
					rot.ram[b] = rot.f;
					rot.y = rot.f;
					break;
				case RAMQD:
				{
					UINT16 q0 = rot.q & 1;
					UINT16 r0 = rot.f & 1;
					UINT16 q15 = 0;
					UINT16 r15 = 0;

					switch (sel)
					{
						case 0: q15 = r15 = 0;
								break;
						case 1: q15 = r15 = 0x8000;
								break;
						case 2: q15 = q0 << 15;
								r15 = r0 << 15;
								break;
						case 3: q15 = r0 << 15;
								r15 = (rot.vflag ^ BIT(rot.f, 15)) << 15;
								break;
					}

					rot.ram[b] = r15 | (rot.f >> 1);
					rot.q = q15 | (rot.q >> 1);
					rot.y = rot.f;
					break;
				}
				case RAMD:
				{
					UINT16 r0 = rot.f & 1;
					UINT16 r15 = 0;

					switch (sel)
					{
						case 0: r15 = 0;        break;
						case 1: r15 = 0x8000;   break;
						case 2: r15 = r0 << 15; break;
						case 3:
							r15 = (rot.vflag ^ BIT(rot.f, 15)) << 15;
							break;
					}

					rot.ram[b] = r15 | (rot.f >> 1);
					rot.y = rot.f;
					break;
				}
				case RAMQU:
				{
					UINT16 q15 = BIT(rot.q, 15);
					UINT16 r15 = BIT(rot.f, 15);
					UINT16 q0 = 0;
					UINT16 r0 = 0;

					switch (sel)
					{
						case 0: q0 = 0; r0 = 0;     break;
						case 1: q0 = 1; r0 = 1;     break;
						case 2: q0 = q15; r0 = r15; break;
						case 3:
						{
							q0 = (spf == SPF_DIV) && !BIT(rot.f, 15);
							r0 = q15;
							break;
						}
					}

					rot.ram[b] = (rot.f << 1) | r0;
					rot.q = (rot.q << 1) | q0;
					rot.y = rot.f;
					break;
				}
				case RAMU:
				{
					UINT16 q15 = BIT(rot.q, 15);
					UINT16 r15 = BIT(rot.f, 15);
					UINT16 r0 = 0;

					switch (sel)
					{
						case 0: r0 = 0;     break;
						case 1: r0 = 1;     break;
						case 2: r0 = r15;   break;
						case 3: r0 = q15;   break;
					}

					rot.ram[b] = (rot.f << 1) | r0;
					rot.y = rot.f;
					break;
				}
			}
		}

		if (rot_do_rotjmp(jmp))
			rot.pc = t;
		else
			rot.pc = (rot.pc + 1) & 0xfff;

		if (spf == SPF_SQLTCH)
			rot.seqcnt = t & 0xf;
		else if ((spf == SPF_MULT) || (spf == SPF_DIV))
			rot.seqcnt = (rot.seqcnt + 1) & 0xf;

		dsrclatch =
				(~(0x10 << dsrc) & 0xf0)
				| (rsrc ? 0x04 : 0x02)
				| (spf == SPF_SWRT ? 0 : 1);

		if (!BIT(rot.dsrclatch, 2) && BIT(dsrclatch, 2))
			rot.rsrclatch = t & 0xff;

		rot.dsrclatch = dsrclatch;

		switch (yout)
		{
			case YOUT_Y2LDA:
			{
				rot.lineaddr = rot.y & 0xfff;
				break;
			}
			case YOUT_Y2LDD:
			{
				rot.linedata = ((t & 0xf) << 12) | (rot.y & 0xfff);
				cubeqcpu_line_w(rot.lineaddr, rot.linedata);
				break;
			}
			case YOUT_Y2DAD: rot.dynaddr = rot.y & 0x3fff;  break;
			case YOUT_Y2DYN: rot.dyndata = rot.y & 0xffff;  break;
			case YOUT_Y2R:   rot.yrlatch = rot.y & 0x7ff;   break;
			case YOUT_Y2D:   rot.ydlatch = rot.y;           break;
		}

		rot.divreg = (spf == SPF_DIV) && !BIT(rot.f, 15);

		rot.prev_dred = !(spf == SPF_DRED);
		rot.prev_dwrt = !(spf == SPF_DWRT);

		rot.clkcnt++;
		rot.icount--;
	} while (rot.icount > 0);
	return cycles - rot.icount;
}

enum line_spf
{
	LSPF_UNUSED  = 0,
	LSPF_FSTOP   = 1,
	LSPF_SREG    = 2,
	LSPF_FSTRT   = 3,
	LSPF_PWRT    = 4,
	LSPF_MULT    = 5,
	LSPF_LSTOP   = 6,
	LSPF_BRES    = 7
};

enum line_latch
{
	LLATCH_UNUSED   = 0,
	LLATCH_SEQLATCH = 1,
	LLATCH_XLATCH   = 2,
	LLATCH_YLATCH   = 3,
	LLATCH_BADLATCH = 4,
	LLATCH_FADLATCH = 5,
	LLATCH_CLATCH   = 6,
	LLATCH_ZLATCH   = 7
};

enum sreg_bits
{
	SREG_E0     = 0,
	SREG_DX_DY  = 1,
	SREG_DY     = 2,
	SREG_DX     = 3,
	SREG_LE0    = 4,
	SREG_LDX_DY = 5,
	SREG_LDY    = 6,
	SREG_LDX    = 7
};

static int lin_do_linjmp(UINT8 jmp)
{
	int ret = 0;

	switch (jmp & 7)
	{
		 case 0: ret = 0; break;
		 case 1: ret = BIT(lin.f, 11); break;
		 case 2: ret = (lin.seqcnt == 0xfff); break;
		 case 3: ret = !(lin.f == 0) && !BIT(lin.f, 11); break;
		 case 4: ret = (lin.cflag); break;
		 case 5: ret = (lin.f == 0); break;
	}

	return !(!ret ^ BIT(jmp, 3));
}

static void lin_cubeqcpu_swap_line_banks(void)
{
	lin.field = lin.field ^ 1;
}

static void lin_cubeqcpu_clear_stack(void)
{
	memset(&lin.ptr_ram[lin.field * 256], 0, 256);
}

static UINT8 lin_cubeqcpu_get_ptr_ram_val(int i)
{
	return lin.ptr_ram[((lin.field^1) * 256) + i];
}

static UINT32* lin_cubeqcpu_get_stack_ram()
{
	if (lin.field != ODD_FIELD)
		return lin.o_stack;
	else
		return lin.e_stack;
}

#define LINE_PC         ((lin.pc[prog] & 0x7f) | ((prog == BACKGROUND) ? 0x80 : 0))

static int lin_execute(int cycles)
{
	lin.icount = cycles;
	UINT32  *stack_ram;
	UINT8   *ptr_ram;

	if (lin.field == ODD_FIELD)
	{
		stack_ram = lin.o_stack;
		ptr_ram = &lin.ptr_ram[0];
	}
	else
	{
		stack_ram = lin.e_stack;
		ptr_ram = &lin.ptr_ram[0x100];
	}

	do
	{
		int prog = (lin.clkcnt & 3) ? BACKGROUND : FOREGROUND;

		lin.curpc = LINE_PC;
		UINT64 inst = lin.rom[lin.curpc];

		UINT32 inslow = inst & 0xffffffff;
		UINT32 inshig = inst >> 32;

		UINT8 t               = (inshig >> 24) & 0xff;
		const UINT8 jmp       = (inshig >> 20) & 0xf;
		const UINT8 latch     = (inshig >> 16) & 0x7;
		const int op      = (inshig >> 15) & 0x1;
		const UINT8 spf       = (inshig >> 12) & 0x7;
		const UINT8 b         = (inshig >> 8) & 0xf;
		UINT8 a               = (inshig >> 4) & 0xf;
		const UINT8 i8_6      = (inshig >> 0) & 0x7;
		const UINT8 ci        = (inslow >> 31) & 0x1;
		const UINT8 i5_3      = (inslow >> 28) & 0x7;
		const UINT8 _pbcs     = (inslow >> 27) & 0x1;
		UINT8 i2_0            = (inslow >> 24) & 0x7;

		UINT16  data_in = 0;

		CALL_MAME_DEBUG;

		if (prog == FOREGROUND)
		{
			if (!BIT(lin.fglatch, 5))
				data_in = lin.sram[lin.fadlatch];
			else
				data_in = cubeqcpu_line_r();
		}
		else
		{
			if (!BIT(lin.bglatch, 4))
				lin.sram[lin.badlatch] = lin.sramdlatch;
			else if (BIT(lin.bglatch, 2))
				data_in = lin.sram[lin.badlatch];
			else
				data_in = cubeqcpu_line_r();
		}

		if ((lin.clkcnt & 3) == 1)
		{
			if (BIT(lin.fglatch, 4) && (lin.ycnt < 256))
			{
				UINT32 data;
				UINT16 h = lin.xcnt;
				UINT8 v = lin.ycnt & 0xff;

				if (h >= 320)
					h = (h & 0x800) ? 0 : 319;

				if (BIT(lin.fglatch, 3))
					data = (0 << 19) | (h << 8) | lin.zlatch;
				else
					data = (1 << 19) | ((lin.clatch & 0x100) << 9) | (h << 8) | (lin.clatch & 0xff);

				stack_ram[(v << 7) | (ptr_ram[v] & 0x7f)] = data;

				ptr_ram[v] = (ptr_ram[v] + 1) & 0x7f;
			}
		}

		if (op)
			t = (t & ~0xf) | (data_in >> 12);

		if ((spf == LSPF_MULT) && !BIT(lin.q, 0))
			i2_0 |= 2;

		if ((prog == FOREGROUND) && !BIT(lin.fglatch, 2))
			a |= lin.gt0reg;

		{
			UINT16 r = 0;
			UINT16 s = 0;

			UINT16 res = 0;
			UINT32 cflag = 0;
			UINT32 vflag = 0;

			switch (i2_0)
			{
				case 0: r = lin.ram[a];   s = lin.q;      break;
				case 1: r = lin.ram[a];   s = lin.ram[b]; break;
				case 2: r = 0;                  s = lin.q;      break;
				case 3: r = 0;                  s = lin.ram[b]; break;
				case 4: r = 0;                  s = lin.ram[a]; break;
				case 5: r = data_in;            s = lin.ram[a]; break;
				case 6: r = data_in;            s = lin.q;      break;
				case 7: r = data_in;            s = 0;                break;
			}

			r &= 0xfff;
			s &= 0xfff;

			switch (i5_3)
			{
				case ADD:
					res = r + s + ci;
					cflag = (res >> 12) & 1;
					vflag = (((r & 0x7ff) + (s & 0x7ff) + ci) >> 11) ^ cflag;
					break;
				case SUBR:
					res = (r ^ 0x0FFF) + s + ci;
					cflag = (res >> 12) & 1;
					vflag = (((s & 0x7ff) + (~r & 0x7ff) + ci) >> 11) ^ cflag;
					break;
				case SUBS:
					res = r + (s ^ 0x0FFF) + ci;
					cflag = (res >> 12) & 1;
					vflag = (((r & 0x7ff) + (~s & 0x7ff) + ci) >> 11) ^ cflag;
					break;
				case OR:
					res = r | s;
					break;
				case AND:
					res = r & s;
					break;
				case NOTRS:
					res = ~r & s;
					break;
				case EXOR:
					res = r ^ s;
					break;
				case EXNOR:
					res = ~(r ^ s);
					break;
			}

			lin.f = res & 0xfff;
			lin.cflag = cflag;
			lin.vflag = vflag;

			switch (i8_6)
			{
				case QREG:
					lin.q = lin.f;
					lin.y = lin.f;
					break;
				case NOP:
					lin.y = lin.f;
					break;
				case RAMA:
					lin.y = lin.ram[a];
					lin.ram[b] = lin.f;
					break;
				case RAMF:
					lin.ram[b] = lin.f;
					lin.y = lin.f;
					break;
				case RAMQD:
				{
					UINT16 r11 = (BIT(lin.f, 11) ^ lin.vflag) ? 0x800 : 0;
					UINT16 q11 = (prog == BACKGROUND) ? 0x800 : 0;

					lin.ram[b] = r11 | (lin.f >> 1);
					lin.q = q11 | (lin.q >> 1);
					lin.y = lin.f;
					break;
				}
				case RAMD:
				{
					UINT16 r11 = (BIT(lin.f, 11) ^ lin.vflag) ? 0x800 : 0;

					lin.ram[b] = r11 | (lin.f >> 1);
					lin.y = lin.f;
					break;
				}
				case RAMQU:
				{
					UINT16 r0 = (prog == BACKGROUND);

					lin.ram[b] = (lin.f << 1) | r0;
					lin.q = (lin.q << 1) | 0;
					lin.y = lin.f;
					break;
				}
				case RAMU:
				{
					UINT16 r0 = (prog == BACKGROUND);

					lin.ram[b] = (lin.f << 1) | r0;
					lin.y = lin.f;
					break;
				}
			}
		}

		if (lin_do_linjmp(jmp))
			lin.pc[prog] = t & 0x7f;
		else
			lin.pc[prog] = (lin.pc[prog] + 1) & 0x7f;

		if (prog == BACKGROUND)
			lin.pc[prog] |= 0x80;
		else
		{
			if (latch == LLATCH_XLATCH)
				lin.xcnt = lin.y & 0xfff;
			else
			{
				int _xcet;
				int mux_sel = (BIT(lin.sreg, SREG_DX_DY) << 1) | (BIT(lin.sreg, SREG_DX) ^ BIT(lin.sreg, SREG_DY));

				if (mux_sel == 0)
					_xcet = !(spf == LSPF_BRES);
				else if (mux_sel == 1)
					_xcet = BIT(lin.fglatch, 1);
				else if (mux_sel == 2)
					_xcet = !(lin.gt0reg && (spf == LSPF_BRES));
				else
					_xcet = BIT(lin.fglatch, 0);

				if (!_xcet)
					lin.xcnt = (lin.xcnt + (BIT(lin.sreg, SREG_DX) ? 1 : -1)) & 0xfff;
			}

			if (latch == LLATCH_YLATCH)
				lin.ycnt = lin.y & 0xfff;
			else
			{
				int _ycet;
				int mux_sel = (BIT(lin.sreg, SREG_DX_DY) << 1) | (BIT(lin.sreg, SREG_DX) ^ BIT(lin.sreg, SREG_DY));

				if (mux_sel == 0)
					_ycet = !(lin.gt0reg && (spf == LSPF_BRES));
				else if (mux_sel == 1)
					_ycet = BIT(lin.fglatch, 0);
				else if (mux_sel == 2)
					_ycet = !(spf == LSPF_BRES);
				else
					_ycet = BIT(lin.fglatch, 1);

				if (!_ycet)
					lin.ycnt = (lin.ycnt + (BIT(lin.sreg, SREG_DY) ? 1 : -1)) & 0xfff;
			}
		}

		if (latch == LLATCH_CLATCH)
			lin.clatch = lin.y & 0x1ff;
		else if (latch == LLATCH_ZLATCH)
			lin.zlatch = lin.y & 0xff;
		else if (latch == LLATCH_FADLATCH)
			lin.fadlatch = lin.y & 0xfff;
		else if (latch == LLATCH_BADLATCH)
			lin.badlatch = lin.y & 0xfff;

		if (!BIT(lin.bglatch, 5))
			lin.sramdlatch = ((t & 0xf) << 12) | (lin.y & 0x0fff);

		if (prog == FOREGROUND)
		{
			int mux_sel = (!(spf == LSPF_FSTOP) << 1) | !(spf == LSPF_LSTOP);
			int dowrt;
			int start_stop;

			if (mux_sel == 0)
			{
				dowrt = 0;
				start_stop = 0;
			}
			else if (mux_sel == 1)
			{
				dowrt = lin.fdxreg ^ BIT(lin.sreg, SREG_DX);
				start_stop = lin.fdxreg;
			}
			else if (mux_sel == 2)
			{
				dowrt = BIT(lin.sreg, SREG_LDX) ^ BIT(lin.sreg, SREG_DX);
				start_stop = BIT(lin.sreg, SREG_DX);
			}
			else
			{
				dowrt = (spf == LSPF_BRES) && (BIT(lin.sreg, SREG_DX_DY) || lin.gt0reg);
				start_stop = BIT(lin.sreg, SREG_DY);
			}

			lin.fglatch =
					(latch == LLATCH_FADLATCH ? 0 : (1 << 5))
					| (dowrt << 4)
					| (start_stop << 3)
					| (_pbcs << 2)
					| (spf == LSPF_BRES ? 0 : (1 << 1))
					| (lin.gt0reg && (spf == LSPF_BRES) ? 0 : 1);
		}
		else
		{
			int _lpwrt = BIT(lin.bglatch, 5);

			lin.bglatch =
					(!(spf == LSPF_PWRT) << 5)
					| (_lpwrt << 4)
					| ((!_lpwrt || (!(spf == LSPF_PWRT) && (latch == LLATCH_BADLATCH))) << 2);
		}

		if (spf == LSPF_SREG)
		{
			lin.sreg = (lin.sreg << 1) | !BIT(lin.f, 11);

			lin.gt0reg = !(lin.f == 0) && !BIT(lin.f, 11);
		}
		else if (spf == LSPF_FSTRT)
		{
			lin.fdxreg = BIT(lin.sreg, 3);
		}

		if (latch == LLATCH_SEQLATCH)
		{
			lin.seqcnt = lin.y & 0xfff;
		}
		else if (spf == LSPF_BRES)
		{
			lin.seqcnt = (lin.seqcnt + 1) & 0xfff;

			lin.gt0reg = !(lin.f == 0) && !BIT(lin.f, 11);
		}

		lin.icount--;
		lin.clkcnt++;
	} while (lin.icount > 0);
	return cycles - lin.icount;
}

static void cubeqcpu_line_w(offs_t offset, UINT16 data) { lin.sram[offset] = data; }
static UINT16 cubeqcpu_line_r(void) { return rot.linedata; }
READ16_HANDLER( cubeqcpu_rotram_r ) { return rot.dram[offset]; }
WRITE16_HANDLER( cubeqcpu_rotram_w ) { COMBINE_DATA(&rot.dram[offset]); }
READ16_HANDLER( cubeqcpu_sndram_r ) { return snd.sram[offset]; }
WRITE16_HANDLER( cubeqcpu_sndram_w ) { COMBINE_DATA(&snd.sram[offset]); }
void cubeqcpu_swap_banks(void) { lin_cubeqcpu_swap_line_banks(); lin_cubeqcpu_clear_stack(); }
UINT8 cubeqcpu_ptr(int y) { return lin_cubeqcpu_get_ptr_ram_val(y); }
UINT32 *cubeqcpu_stack(void) { return lin_cubeqcpu_get_stack_ram(); }

#ifdef MAME_DEBUG
const char *const cubeq_ins[] =
{
	"ADD  ",
	"SUBR ",
	"SUBS ",
	"OR   ",
	"AND  ",
	"NOTRS",
	"EXOR ",
	"EXNOR",
};

const char *const cubeq_src[] =
{
	"A,Q",
	"A,B",
	"0,Q",
	"0,B",
	"0,A",
	"D,A",
	"D,Q",
	"D,0",
};

const char *const cubeq_dst[] =
{
	"QREG ",
	"NOP  ",
	"RAMA ",
	"RAMF ",
	"RAMQD",
	"RAMD ",
	"RAMQU",
	"RAMU ",
};

static offs_t snd_dasm(char *buffer, offs_t pc)
{
	static const char *const jmps[] =
	{
		"JUMP ",
		"     ",
		"JMSB ",
		"JNMSB",
		"     ",
		"JZERO",
		"JOVR ",
		"JLOOP",
	};

	static const char *const latches[] =
	{
		"PLTCH  ",
		"DAC    ",
		"ADLATCH",
		"       ",
	};

	UINT64 inst = ((UINT64 *)memory_region(REGION_CPU4))[pc / 8];
	UINT32 inslow = inst & 0xffffffff;
	UINT32 inshig = inst >> 32;

	int t       = (inshig >> 24) & 0xff;
	int b       = (inshig >> 20) & 0xf;
	int a       = (inshig >> 16) & 0xf;
	int ci      = (inshig >> 15) & 1;
	int i5_3    = (inshig >> 12) & 7;
	int _ramen  = (inshig >> 11) & 1;
	int i2_0    = (inshig >> 8) & 7;
	int rtnltch = (inshig >> 7) & 1;
	int jmp     = (inshig >> 4) & 7;
	int inca    = (inshig >> 3) & 1;
	int i8_6    = (inshig >> 0) & 7;
	int _ipram  = (inslow >> 31) & 1;
	int _ipwrt  = (inslow >> 30) & 1;
	int latch   = (inslow >> 28) & 3;
	int rtn     = (inslow >> 27) & 1;
	int _rin    = (inslow >> 26) & 1;

	sprintf(buffer, "%s %s %s %x,%x,%c %02x %s %s %02x %s %s %s %c %c %c",
			cubeq_ins[i5_3],
			cubeq_src[i2_0],
			cubeq_dst[i8_6],
			a,
			b,
			ci ? 'C' : ' ',
			_rin,
			jmps[jmp],
			rtn ? "RET" : "   ",
			t,
			latches[latch],
			rtnltch ? "RTLATCH" : "       ",
			_ramen ? "PROM" : "RAM ",
			_ipram ? ' ' : 'R',
			_ipwrt ? ' ' : 'W',
			inca ? 'I' : ' ');

	return 8 | DASMFLAG_SUPPORTED;
}
static offs_t rot_dasm(char *buffer, offs_t pc)
{
	static const char *const jmps[] =
	{
		"       ",
		"JSEQ   ",
		"JC     ",
		"JSYNC  ",
		"JLDWAIT",
		"JMSB   ",
		"JGEONE ",
		"JZERO  ",

		"JUMP   ",
		"JNSEQ  ",
		"JNC    ",
		"JNSYNC ",
		"JNLDWAI",
		"JNMSB  ",
		"JLTONE ",
		"JNZERO ",
	};

	static const char *const youts[] =
	{
		"     ",
		"     ",
		"Y2LDA",
		"Y2LDD",
		"Y2DAD",
		"Y2DIN",
		"Y2R  ",
		"Y2D  ",
	};

	static const char *const spfs[] =
	{
		"      ",
		"      ",
		"OP    ",
		"RET   ",
		"SQLTCH",
		"SWRT  ",
		"DIV   ",
		"MULT  ",

		"DRED  ",
		"DWRT  ",
		"???   ",
		"???   ",
		"???   ",
		"???   ",
		"???   ",
		"???   "
	};

	UINT64 inst = ((UINT64 *)memory_region(REGION_CPU2))[pc / 8];
	UINT32 inslow = inst & 0xffffffff;
	UINT32 inshig = inst >> 32;

	int t       = (inshig >> 20) & 0xfff;
	int jmp     = (inshig >> 16) & 0xf;
	int spf     = (inshig >> 12) & 0xf;

	int yout    = (inshig >> 8) & 0x7;
	int sel     = (inshig >> 6) & 0x3;

	int b       = (inshig >> 0) & 0xf;
	int a       = (inslow >> 28) & 0xf;
	int i8_6    = (inslow >> 24) & 0x7;
	int ci      = (inslow >> 23) & 0x1;
	int i5_3    = (inslow >> 20) & 0x7;

	int i2_0    = (inslow >> 16) & 0x7;

	sprintf(buffer, "%s %s,%s %x,%x,%c %d %s %s %s %02x",
			cubeq_ins[i5_3],
			cubeq_src[i2_0],
			cubeq_dst[i8_6],
			a,
			b,
			ci ? 'C' : ' ',
			sel,
			jmps[jmp],
			youts[yout],
			spfs[spf],
			t);

	return 8 | DASMFLAG_SUPPORTED;
}
static offs_t lin_dasm(char *buffer, offs_t pc)
{
	static const char *const jmps[] =
	{
		"     ",
		"JMSB ",
		"JSEQ ",
		"JGTZ ",
		"JC   ",
		"JZ   ",
		"?????",
		"?????",

		"JUMP ",
		"JNMSB",
		"JNSEQ",
		"JLEZ ",
		"JNC  ",
		"JNZ  ",
		"?????",
		"?????",
	};

	static const char *const latches[] =
	{
		"       ",
		"SEQLTCH",
		"XLTCH  ",
		"YLTCH  ",
		"BGLTCH ",
		"FGLTCH ",
		"CLTCH  ",
		"ZLTCH  ",
	};

	static const char *const spfs[] =
	{
		"      ",
		"FSTOP ",
		"FREG  ",
		"FSTART",
		"PWRT  ",
		"MULT  ",
		"LSTOP ",
		"BRES  ",
	};

	UINT64 inst = ((UINT64 *)memory_region(REGION_CPU3))[pc / 8];
	UINT32 inslow = inst & 0xffffffff;
	UINT32 inshig = inst >> 32;

	int t       = (inshig >> 24) & 0xff;
	int jmp     = (inshig >> 20) & 0xf;
	int latch   = (inshig >> 16) & 0x7;
	int op      = (inshig >> 15) & 0x1;
	int spf     = (inshig >> 12) & 0x7;
	int b       = (inshig >> 8) & 0xf;
	int a       = (inshig >> 4) & 0xf;
	int i8_6    = (inshig >> 0) & 0x7;
	int ci      = (inslow >> 31) & 0x1;
	int i5_3    = (inslow >> 28) & 0x7;
	int _pbcs   = (inslow >> 27) & 0x1;
	int i2_0    = (inslow >> 24) & 0x7;

	sprintf(buffer, "%s %s,%s %x,%x %c %s %02x %s %s %s %s",
			cubeq_ins[i5_3],
			cubeq_src[i2_0],
			cubeq_dst[i8_6],
			a,
			b,
			ci ? 'C' : ' ',
			jmps[jmp],
			t,
			latches[latch],
			op ? "OP" : "  ",
			_pbcs ? "  " : "PB",
			spfs[spf]);

	return 8 | DASMFLAG_SUPPORTED;
}
#endif
/* Keep each engine in a distinct scheduler family; contexts are keyed by the core-file string. */
static void snd_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	memset(&snd, 0, sizeof(snd));
	snd.sram = auto_malloc(sizeof(UINT16) * 2048);
	memset(snd.sram, 0, sizeof(UINT16) * 2048);
	snd.rom = (UINT64 *)memory_region(REGION_CPU4);
	snd.sound_data = (UINT16 *)memory_region(REGION_SOUND1);
}
static void snd_reset(void) { snd.pc = 0; }
static void snd_get_context(void *dst) { if (dst) memcpy(dst, &snd, sizeof(snd)); }
static void snd_set_context(void *src) { if (src) memcpy(&snd, src, sizeof(snd)); }
static void snd_set_info(UINT32 state, union cpuinfo *info) { if (state == CPUINFO_INT_PC) snd.pc = info->i / 8; }
void cquestsnd_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
#ifdef MAME_DEBUG
	case CPUINFO_PTR_DISASSEMBLE: info->disassemble = snd_dasm; break;
#endif
	case CPUINFO_INT_CLOCK_DIVIDER: info->i = 1; break;
	case CPUINFO_INT_CONTEXT_SIZE: info->i = sizeof(snd); break;
	case CPUINFO_INT_ENDIANNESS: info->i = CPU_IS_BE; break;
	case CPUINFO_INT_MIN_INSTRUCTION_BYTES:
	case CPUINFO_INT_MAX_INSTRUCTION_BYTES: info->i = 8; break;
	case CPUINFO_INT_MIN_CYCLES:
	case CPUINFO_INT_MAX_CYCLES: info->i = 1; break;
	case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 64; break;
	case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 11; break;
	case CPUINFO_INT_PC:
	case CPUINFO_INT_PREVIOUSPC: info->i = snd.pc * 8; break;
	case CPUINFO_PTR_INIT: info->init = snd_init; break;
	case CPUINFO_PTR_RESET: info->reset = snd_reset; break;
	case CPUINFO_PTR_EXECUTE: info->execute = snd_execute; break;
	case CPUINFO_PTR_GET_CONTEXT: info->getcontext = snd_get_context; break;
	case CPUINFO_PTR_SET_CONTEXT: info->setcontext = snd_set_context; break;
	case CPUINFO_PTR_SET_INFO: info->setinfo = snd_set_info; break;
	case CPUINFO_PTR_INSTRUCTION_COUNTER: info->icount = &snd.icount; break;
	case CPUINFO_STR_NAME: strcpy(info->s = cpuintrf_temp_str(), "CQUESTSND"); break;
	case CPUINFO_STR_CORE_FAMILY: strcpy(info->s = cpuintrf_temp_str(), "Cube Quest snd"); break;
	case CPUINFO_STR_CORE_VERSION: strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
	case CPUINFO_STR_CORE_FILE: strcpy(info->s = cpuintrf_temp_str(), __FILE__ ":snd"); break;
	case CPUINFO_STR_CORE_CREDITS: strcpy(info->s = cpuintrf_temp_str(), "Philip Bennett"); break;
	case CPUINFO_STR_FLAGS: sprintf(info->s = cpuintrf_temp_str(), "%c%c%c", snd.cflag ? 'C' : '.', snd.vflag ? 'V' : '.', snd.f ? '.' : 'Z'); break;
	case CPUINFO_STR_REGISTER + 1: sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", snd.pc * 8); break;
	case CPUINFO_STR_REGISTER + 2: sprintf(info->s = cpuintrf_temp_str(), "Q:%04X", snd.q); break;
	default:
		if (state >= CPUINFO_STR_REGISTER + 3 && state < CPUINFO_STR_REGISTER + 19)
			sprintf(info->s = cpuintrf_temp_str(), "R%X:%04X", state - CPUINFO_STR_REGISTER - 3, snd.ram[state - CPUINFO_STR_REGISTER - 3]);
		break;
	}
}

static void rot_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	memset(&rot, 0, sizeof(rot));
	rot.dram = auto_malloc(sizeof(UINT16) * 16384);
	memset(rot.dram, 0, sizeof(UINT16) * 16384);
	rot.sram = auto_malloc(sizeof(UINT16) * 2048);
	memset(rot.sram, 0, sizeof(UINT16) * 2048);
	rot.rom = (UINT64 *)memory_region(REGION_CPU2);
}
static void rot_reset(void) { rot.pc = 0; rot.wc = 0; rot.prev_dred = rot.prev_dwrt = 1; }
static void rot_get_context(void *dst) { if (dst) memcpy(dst, &rot, sizeof(rot)); }
static void rot_set_context(void *src) { if (src) memcpy(&rot, src, sizeof(rot)); }
static void rot_set_info(UINT32 state, union cpuinfo *info) { if (state == CPUINFO_INT_PC) rot.pc = info->i / 8; }
void cquestrot_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
#ifdef MAME_DEBUG
	case CPUINFO_PTR_DISASSEMBLE: info->disassemble = rot_dasm; break;
#endif
	case CPUINFO_INT_CLOCK_DIVIDER: info->i = 1; break;
	case CPUINFO_INT_CONTEXT_SIZE: info->i = sizeof(rot); break;
	case CPUINFO_INT_ENDIANNESS: info->i = CPU_IS_BE; break;
	case CPUINFO_INT_MIN_INSTRUCTION_BYTES:
	case CPUINFO_INT_MAX_INSTRUCTION_BYTES: info->i = 8; break;
	case CPUINFO_INT_MIN_CYCLES:
	case CPUINFO_INT_MAX_CYCLES: info->i = 1; break;
	case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 64; break;
	case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 12; break;
	case CPUINFO_INT_PC:
	case CPUINFO_INT_PREVIOUSPC: info->i = (rot.pc & 0x1ff) * 8; break;
	case CPUINFO_PTR_INIT: info->init = rot_init; break;
	case CPUINFO_PTR_RESET: info->reset = rot_reset; break;
	case CPUINFO_PTR_EXECUTE: info->execute = rot_execute; break;
	case CPUINFO_PTR_GET_CONTEXT: info->getcontext = rot_get_context; break;
	case CPUINFO_PTR_SET_CONTEXT: info->setcontext = rot_set_context; break;
	case CPUINFO_PTR_SET_INFO: info->setinfo = rot_set_info; break;
	case CPUINFO_PTR_INSTRUCTION_COUNTER: info->icount = &rot.icount; break;
	case CPUINFO_STR_NAME: strcpy(info->s = cpuintrf_temp_str(), "CQUESTROT"); break;
	case CPUINFO_STR_CORE_FAMILY: strcpy(info->s = cpuintrf_temp_str(), "Cube Quest rot"); break;
	case CPUINFO_STR_CORE_VERSION: strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
	case CPUINFO_STR_CORE_FILE: strcpy(info->s = cpuintrf_temp_str(), __FILE__ ":rot"); break;
	case CPUINFO_STR_CORE_CREDITS: strcpy(info->s = cpuintrf_temp_str(), "Philip Bennett"); break;
	case CPUINFO_STR_FLAGS: sprintf(info->s = cpuintrf_temp_str(), "%c%c%c", rot.cflag ? 'C' : '.', rot.vflag ? 'V' : '.', rot.f ? '.' : 'Z'); break;
	case CPUINFO_STR_REGISTER + 1: sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", rot.pc * 8); break;
	case CPUINFO_STR_REGISTER + 2: sprintf(info->s = cpuintrf_temp_str(), "Q:%04X", rot.q); break;
	default:
		if (state >= CPUINFO_STR_REGISTER + 3 && state < CPUINFO_STR_REGISTER + 19)
			sprintf(info->s = cpuintrf_temp_str(), "R%X:%04X", state - CPUINFO_STR_REGISTER - 3, rot.ram[state - CPUINFO_STR_REGISTER - 3]);
		break;
	}
}

static void lin_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	memset(&lin, 0, sizeof(lin));
	lin.sram = auto_malloc(sizeof(UINT16) * 4096);
	memset(lin.sram, 0, sizeof(UINT16) * 4096);
	lin.ptr_ram = auto_malloc(sizeof(UINT8) * 1024);
	memset(lin.ptr_ram, 0, sizeof(UINT8) * 1024);
	lin.e_stack = auto_malloc(sizeof(UINT32) * 32768);
	memset(lin.e_stack, 0, sizeof(UINT32) * 32768);
	lin.o_stack = auto_malloc(sizeof(UINT32) * 32768);
	memset(lin.o_stack, 0, sizeof(UINT32) * 32768);
	lin.rom = (UINT64 *)memory_region(REGION_CPU3);
}
static void lin_reset(void) { lin.clkcnt = 0; lin.pc[0] = 0; lin.pc[1] = 0x80; }
static void lin_get_context(void *dst) { if (dst) memcpy(dst, &lin, sizeof(lin)); }
static void lin_set_context(void *src) { if (src) memcpy(&lin, src, sizeof(lin)); }
static void lin_set_info(UINT32 state, union cpuinfo *info) { if (state == CPUINFO_INT_PC) lin.pc[0] = info->i / 8; }
void cquestlin_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
#ifdef MAME_DEBUG
	case CPUINFO_PTR_DISASSEMBLE: info->disassemble = lin_dasm; break;
#endif
	case CPUINFO_INT_CLOCK_DIVIDER: info->i = 1; break;
	case CPUINFO_INT_CONTEXT_SIZE: info->i = sizeof(lin); break;
	case CPUINFO_INT_ENDIANNESS: info->i = CPU_IS_BE; break;
	case CPUINFO_INT_MIN_INSTRUCTION_BYTES:
	case CPUINFO_INT_MAX_INSTRUCTION_BYTES: info->i = 8; break;
	case CPUINFO_INT_MIN_CYCLES:
	case CPUINFO_INT_MAX_CYCLES: info->i = 1; break;
	case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 64; break;
	case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 11; break;
	case CPUINFO_INT_PC:
	case CPUINFO_INT_PREVIOUSPC: info->i = lin.curpc * 8; break;
	case CPUINFO_PTR_INIT: info->init = lin_init; break;
	case CPUINFO_PTR_RESET: info->reset = lin_reset; break;
	case CPUINFO_PTR_EXECUTE: info->execute = lin_execute; break;
	case CPUINFO_PTR_GET_CONTEXT: info->getcontext = lin_get_context; break;
	case CPUINFO_PTR_SET_CONTEXT: info->setcontext = lin_set_context; break;
	case CPUINFO_PTR_SET_INFO: info->setinfo = lin_set_info; break;
	case CPUINFO_PTR_INSTRUCTION_COUNTER: info->icount = &lin.icount; break;
	case CPUINFO_STR_NAME: strcpy(info->s = cpuintrf_temp_str(), "CQUESTLIN"); break;
	case CPUINFO_STR_CORE_FAMILY: strcpy(info->s = cpuintrf_temp_str(), "Cube Quest lin"); break;
	case CPUINFO_STR_CORE_VERSION: strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
	case CPUINFO_STR_CORE_FILE: strcpy(info->s = cpuintrf_temp_str(), __FILE__ ":lin"); break;
	case CPUINFO_STR_CORE_CREDITS: strcpy(info->s = cpuintrf_temp_str(), "Philip Bennett"); break;
	case CPUINFO_STR_FLAGS: sprintf(info->s = cpuintrf_temp_str(), "%c%c%c", lin.cflag ? 'C' : '.', lin.vflag ? 'V' : '.', lin.f ? '.' : 'Z'); break;
	case CPUINFO_STR_REGISTER + 1: sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", lin.curpc * 8); break;
	case CPUINFO_STR_REGISTER + 2: sprintf(info->s = cpuintrf_temp_str(), "Q:%04X", lin.q); break;
	default:
		if (state >= CPUINFO_STR_REGISTER + 3 && state < CPUINFO_STR_REGISTER + 19)
			sprintf(info->s = cpuintrf_temp_str(), "R%X:%04X", state - CPUINFO_STR_REGISTER - 3, lin.ram[state - CPUINFO_STR_REGISTER - 3]);
		break;
	}
}

// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria, Curt Coder
/* National Semiconductor COP400 */

#include "cpuintrf.h"
#include "debugger.h"
#include "cop400.h"

typedef struct {
	unsigned cycles;
	void (*function) (UINT8 opcode);
}	s_opcode;

#define UINT1	UINT8
#define UINT4	UINT8
#define UINT6	UINT8
#define UINT10	UINT16

typedef struct
{
	UINT10 	PC;
	UINT10	PREVPC;
	UINT4	A;
	UINT6	B;
	UINT1	C;
	UINT4	EN;
	UINT4	G;
	UINT8	Q;
	UINT10	SA, SB, SC;
	UINT4	SIO;
	UINT1	SKL;
	UINT8   skip, skipLBI;
	UINT1	timerlatch;
	UINT16	counter;
	UINT8	G_mask;
	UINT8	D_mask;
	UINT4	IL;
	int		last_si;
	int		last_skip;
} COP420_Regs;

static COP420_Regs R;
static int    cop420_ICount;

static int InstLen[256];
static int LBIops[256];
static int LBIops33[256];

#include "420ops.c"

static const s_opcode opcode_23_map[256]=
{
	{1, ldd		 	},{1, ldd		},{1, ldd	 	},{1, ldd	 	},{1, ldd	 	},{1, ldd 		},{1, ldd	 	},{1, ldd	 	},
	{1, ldd		 	},{1, ldd		},{1, ldd	 	},{1, ldd	 	},{1, ldd	 	},{1, ldd 		},{1, ldd	 	},{1, ldd	 	},
	{1, ldd		 	},{1, ldd		},{1, ldd	 	},{1, ldd	 	},{1, ldd	 	},{1, ldd 		},{1, ldd	 	},{1, ldd	 	},
	{1, ldd		 	},{1, ldd		},{1, ldd	 	},{1, ldd	 	},{1, ldd	 	},{1, ldd 		},{1, ldd	 	},{1, ldd	 	},
	{1, ldd		 	},{1, ldd		},{1, ldd	 	},{1, ldd	 	},{1, ldd	 	},{1, ldd 		},{1, ldd	 	},{1, ldd	 	},
	{1, ldd		 	},{1, ldd		},{1, ldd	 	},{1, ldd	 	},{1, ldd	 	},{1, ldd 		},{1, ldd	 	},{1, ldd	 	},
	{1, ldd		 	},{1, ldd		},{1, ldd	 	},{1, ldd	 	},{1, ldd	 	},{1, ldd 		},{1, ldd	 	},{1, ldd	 	},
	{1, ldd		 	},{1, ldd		},{1, ldd	 	},{1, ldd	 	},{1, ldd	 	},{1, ldd 		},{1, ldd	 	},{1, ldd	 	},

	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},

	{1, xad		 	},{1, xad		},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},
	{1, xad		 	},{1, xad		},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},
	{1, xad		 	},{1, xad		},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},
	{1, xad		 	},{1, xad		},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},
	{1, xad		 	},{1, xad		},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},
	{1, xad		 	},{1, xad		},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},
	{1, xad		 	},{1, xad		},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},
	{1, xad		 	},{1, xad		},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},{1, xad	 	},

	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	}
};

static void cop420_op23(UINT8 opcode)
{
	UINT8 opcode23 = ROM(PC++);

	(*(opcode_23_map[opcode23].function))(opcode23);
}

static const s_opcode opcode_33_map[256]=
{
	{1, illegal		},{1, skgbz0 	},{1, illegal 	},{1, skgbz2 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, skgbz1 	},{1, illegal 	},{1, skgbz3 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, skgz	 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, inin 		},{1, inil 		},{1, ing	 	},{1, illegal 	},{1, cqma	 	},{1, illegal 	},{1, inl	 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, omg	 	},{1, illegal 	},{1, camq	 	},{1, illegal 	},{1, obd	 	},{1, illegal 	},

	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, ogi	 		},{1, ogi	 	},{1, ogi	 	},{1, ogi	 	},{1, ogi	 	},{1, ogi	 	},{1, ogi	 	},{1, ogi	 	},
	{1, ogi	 		},{1, ogi	 	},{1, ogi	 	},{1, ogi	 	},{1, ogi	 	},{1, ogi	 	},{1, ogi	 	},{1, ogi	 	},
	{1, lei 		},{1, lei 		},{1, lei 		},{1, lei 		},{1, lei 		},{1, lei 		},{1, lei 		},{1, lei 		},
	{1, lei 		},{1, lei 		},{1, lei	 	},{1, lei	 	},{1, lei	 	},{1, lei	 	},{1, lei	 	},{1, lei	 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},

	{1, lbi		 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},
	{1, lbi		 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},
	{1, lbi		 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},
	{1, lbi		 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},
	{1, lbi		 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},
	{1, lbi		 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},
	{1, lbi		 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},
	{1, lbi		 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},{1, lbi	 	},

	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	}
};

static void cop420_op33(UINT8 opcode)
{
	UINT8 opcode33 = ROM(PC++);

	(*(opcode_33_map[opcode33].function))(opcode33);
}

static const s_opcode opcode_map[256]=
{
	{1, clra		},{1, skmbz0	},{1, xor		},{1, skmbz2		},{1, xis		},{1, ld		},{1, x			},{1, xds		},
	{1, lbi			},{1, lbi		},{1, lbi		},{1, lbi			},{1, lbi		},{1, lbi		},{1, lbi		},{1, lbi		},
	{1, casc		},{1, skmbz1	},{1, xabr		},{1, skmbz3		},{1, xis		},{1, ld		},{1, x			},{1, xds		},
	{1, lbi			},{1, lbi		},{1, lbi		},{1, lbi			},{1, lbi		},{1, lbi		},{1, lbi		},{1, lbi		},
	{1, skc			},{1, ske		},{1, sc		},{2, cop420_op23	},{1, xis		},{1, ld		},{1, x			},{1, xds 		},
	{1, lbi			},{1, lbi		},{1, lbi		},{1, lbi			},{1, lbi		},{1, lbi		},{1, lbi		},{1, lbi		},
	{1, asc			},{1, add		},{1, rc		},{2, cop420_op33  	},{1, xis		},{1, ld		},{1, x			},{1, xds		},
	{1, lbi			},{1, lbi		},{1, lbi		},{1, lbi			},{1, lbi		},{1, lbi		},{1, lbi		},{1, lbi		},

	{1, comp		},{1, skt		},{1, rmb2		},{1, rmb3			},{1, nop		},{1, rmb1		},{1, smb2		},{1, smb1		},
	{1,	cop420_ret	},{1, retsk		},{1, adt		},{1, smb3			},{1, rmb0		},{1, smb0		},{1, cba		},{1, xas		},
	{1, cab			},{1, aisc		},{1, aisc		},{1, aisc			},{1, aisc		},{1, aisc		},{1, aisc		},{1, aisc		},
	{1, aisc		},{1, aisc		},{1, aisc		},{1, aisc			},{1, aisc		},{1, aisc		},{1, aisc		},{1, aisc		},
	{2, jmp			},{2, jmp		},{2, jmp		},{2, jmp			},{0, illegal	},{0, illegal	},{0, illegal	},{0, illegal   },
	{2, jsr			},{2, jsr		},{2, jsr		},{2, jsr			},{0, illegal	},{0, illegal	},{0, illegal	},{0, illegal	},
	{1, stii		},{1, stii		},{1, stii		},{1, stii			},{1, stii		},{1, stii		},{1, stii		},{1, stii		},
	{1, stii		},{1, stii		},{1, stii		},{1, stii			},{1, stii		},{1, stii		},{1, stii		},{1, stii		},

	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{2, lqid		},

	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{2, jid		}
};

static ADDRESS_MAP_START( cop420_internal_ram, ADDRESS_SPACE_DATA, 8 )
	AM_RANGE(0x00, 0x3f) AM_RAM
ADDRESS_MAP_END

static void cop420_counter_tick(void)
{
	R.counter++;

	if (R.counter == 1024)
	{
		R.counter = 0;
		R.timerlatch = 1;
	}
}

static void cop420_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	int i;

	memset(&R, 0, sizeof(R));
	R.G_mask = 0x0F;
	R.D_mask = 0x0F;

	for (i=0; i<256; i++) InstLen[i]=1;

	InstLen[0x60] = InstLen[0x61] = InstLen[0x62] = InstLen[0x63] =
	InstLen[0x68] = InstLen[0x69] = InstLen[0x6a] = InstLen[0x6b] =
	InstLen[0x33] = InstLen[0x23] = 2;

	for (i=0; i<256; i++) LBIops[i] = 0;
	for (i=0x08; i<0x10; i++) LBIops[i] = 1;
	for (i=0x18; i<0x20; i++) LBIops[i] = 1;
	for (i=0x28; i<0x30; i++) LBIops[i] = 1;
	for (i=0x38; i<0x40; i++) LBIops[i] = 1;

	for (i=0; i<256; i++) LBIops33[i] = 0;
	for (i=0x80; i<0xc0; i++) LBIops33[i] = 1;

	state_save_register_item("cop420", index, PC);
	state_save_register_item("cop420", index, R.PREVPC);
	state_save_register_item("cop420", index, A);
	state_save_register_item("cop420", index, B);
	state_save_register_item("cop420", index, C);
	state_save_register_item("cop420", index, EN);
	state_save_register_item("cop420", index, G);
	state_save_register_item("cop420", index, Q);
	state_save_register_item("cop420", index, SA);
	state_save_register_item("cop420", index, SB);
	state_save_register_item("cop420", index, SC);
	state_save_register_item("cop420", index, SIO);
	state_save_register_item("cop420", index, SKL);
	state_save_register_item("cop420", index, skip);
	state_save_register_item("cop420", index, skipLBI);
	state_save_register_item("cop420", index, R.timerlatch);
	state_save_register_item("cop420", index, R.counter);
	state_save_register_item("cop420", index, R.G_mask);
	state_save_register_item("cop420", index, R.D_mask);
	state_save_register_item("cop420", index, R.last_si);
	state_save_register_item("cop420", index, R.last_skip);
}

static void cop420_reset(void)
{
	PC = 0;
	A = 0;
	B = 0;
	C = 0;
	OUT_D(0);
	EN = 0;
	R.counter = 0;
	R.timerlatch = 1;
	skip = 0;
	skipLBI = 0;
	WRITE_G(0);
}

static int cop420_execute(int cycles)
{
	UINT8 opcode;

	cop420_ICount = cycles;

	do
	{
		int cycles_before = cop420_ICount;
		prevPC = PC;

		CALL_MAME_DEBUG;

		opcode = ROM(PC);

		if (skipLBI == 1)
		{
			int is_lbi = 0;

			if (opcode == 0x33)
			{
				is_lbi = LBIops33[ROM(PC+1)];
			}
			else
			{
				is_lbi = LBIops[opcode];
			}

			if (is_lbi == 0)
			{
				skipLBI = 0;
			}
			else
			{
				cop420_ICount -= opcode_map[opcode].cycles;

				PC += InstLen[opcode];
			}
		}

		if (skipLBI == 0)
		{
			int inst_cycles = opcode_map[opcode].cycles;

			PC++;

			(*(opcode_map[opcode].function))(opcode);
			cop420_ICount -= inst_cycles;

			if (BIT(EN, 1))
			{
				UINT8 in = IN_IN();

				if (BIT(IL, 1) && !BIT(in, 1))
				{
					void *function = opcode_map[ROM(PC)].function;

					if ((function != jp) &&	(function != jmp) && (function != jsr))
					{
						R.last_skip = skip;
						skip = 0;

						PUSH(PC + 1);

						PC = 0x0ff;

						EN &= ~0x02;
					}
				}

				IL = in;
			}

			if (skip == 1)
			{
				void *function = opcode_map[ROM(PC)].function;

				opcode = ROM(PC);

				if ((function == lqid) || (function == jid))
				{
					cop420_ICount -= 1;
				}
				else
				{
					cop420_ICount -= opcode_map[opcode].cycles;
				}
				PC += InstLen[opcode];

				skip = 0;
			}
		}
		/* Advance peripherals in CPU cycles so skipped instructions keep their timing. */
		while (cycles_before-- > cop420_ICount)
		{
			cop410_serial_tick();
			cop420_counter_tick();
		}
		PC &= 0x3ff;
	} while (cop420_ICount > 0);

	return cycles - cop420_ICount;
}

static void cop420_get_context (void *dst)
{
	if( dst )
		*(COP420_Regs*)dst = R;
}

static void cop420_set_context (void *src)
{
	if( src )
		R = *(COP420_Regs*)src;
}

static void cop420_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + COP400_PC:			PC = info->i;							break;

	    case CPUINFO_INT_REGISTER + COP400_A:			A = info->i;							break;
		case CPUINFO_INT_REGISTER + COP400_B:			B = info->i;							break;
		case CPUINFO_INT_REGISTER + COP400_C:			C = info->i;							break;
		case CPUINFO_INT_REGISTER + COP400_EN:			EN = info->i;							break;
		case CPUINFO_INT_REGISTER + COP400_G:			G = info->i;							break;
		case CPUINFO_INT_REGISTER + COP400_Q:			Q = info->i;							break;
		case CPUINFO_INT_REGISTER + COP400_SA:			SA = info->i;							break;
		case CPUINFO_INT_REGISTER + COP400_SB:			SB = info->i;							break;
		case CPUINFO_INT_REGISTER + COP400_SC:			SC = info->i;							break;
		case CPUINFO_INT_REGISTER + COP400_SIO:			SIO = info->i;							break;
		case CPUINFO_INT_REGISTER + COP400_SKL:			SKL = info->i;							break;
	}
}

void cop420_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(R);					break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 0;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 16;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 2;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 2;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 10;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 8;		break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 9;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + 0:				info->i = 0;						    break;

		case CPUINFO_INT_PREVIOUSPC:					info->i = prevPC;						break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + COP400_PC:			info->i = PC;							break;
		case CPUINFO_INT_REGISTER + COP400_A:			info->i = A;							break;
		case CPUINFO_INT_REGISTER + COP400_B:			info->i = B;							break;
		case CPUINFO_INT_REGISTER + COP400_C:			info->i = C;							break;
		case CPUINFO_INT_REGISTER + COP400_EN:			info->i = EN;							break;
		case CPUINFO_INT_REGISTER + COP400_G:			info->i = G;							break;
		case CPUINFO_INT_REGISTER + COP400_Q:			info->i = Q;							break;
		case CPUINFO_INT_REGISTER + COP400_SA:			info->i = SA;							break;
		case CPUINFO_INT_REGISTER + COP400_SB:			info->i = SB;							break;
		case CPUINFO_INT_REGISTER + COP400_SC:			info->i = SC;							break;
		case CPUINFO_INT_REGISTER + COP400_SIO:			info->i = SIO;							break;
		case CPUINFO_INT_REGISTER + COP400_SKL:			info->i = SKL;							break;

		case CPUINFO_PTR_SET_INFO:						info->setinfo = cop420_set_info;		break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = cop420_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = cop420_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = cop420_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = cop420_reset;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = cop420_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
#ifdef MAME_DEBUG
		case CPUINFO_PTR_DISASSEMBLE_NEW:					info->disassemble_new = cop420_dasm;		break;
#endif
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &cop420_ICount;			break;

		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_DATA:
			info->internal_map = construct_map_cop420_internal_ram;								break;

		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "COP420");				break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "National Semiconductor COPS"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.0");					break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__);				break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright MAME Team"); break;

		case CPUINFO_STR_FLAGS:
			sprintf(info->s = cpuintrf_temp_str(), " ");
			break;

		case CPUINFO_STR_REGISTER + COP400_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%04X", PC);		break;
		case CPUINFO_STR_REGISTER + COP400_A:			sprintf(info->s = cpuintrf_temp_str(), "A:%01X", A );			break;
		case CPUINFO_STR_REGISTER + COP400_B:			sprintf(info->s = cpuintrf_temp_str(), "B:%02X", B );			break;
		case CPUINFO_STR_REGISTER + COP400_C:			sprintf(info->s = cpuintrf_temp_str(), "C:%01X", C);			break;
		case CPUINFO_STR_REGISTER + COP400_EN:			sprintf(info->s = cpuintrf_temp_str(), "EN:%01X", EN);		break;
		case CPUINFO_STR_REGISTER + COP400_G:			sprintf(info->s = cpuintrf_temp_str(), "G:%01X", G);			break;
		case CPUINFO_STR_REGISTER + COP400_Q:			sprintf(info->s = cpuintrf_temp_str(), "Q:%02X", Q);			break;
		case CPUINFO_STR_REGISTER + COP400_SA:			sprintf(info->s = cpuintrf_temp_str(), "SA:%04X", SA);		break;
		case CPUINFO_STR_REGISTER + COP400_SB:			sprintf(info->s = cpuintrf_temp_str(), "SB:%04X", SB);		break;
		case CPUINFO_STR_REGISTER + COP400_SC:			sprintf(info->s = cpuintrf_temp_str(), "SC:%04X", SC);		break;
		case CPUINFO_STR_REGISTER + COP400_SIO:			sprintf(info->s = cpuintrf_temp_str(), "SIO:%01X", SIO);		break;
		case CPUINFO_STR_REGISTER + COP400_SKL:			sprintf(info->s = cpuintrf_temp_str(), "SKL:%01X", SKL);		break;
	}
}

/*
   Texas Instruments TMS320C51 DSP Emulator

   Written by Ville Linde
*/

#include "driver.h"
#include "tms32051.h"
#include "debugger.h"



enum {
	TMS32051_PC = 1,
	TMS32051_ACC,
	TMS32051_ACCB,
	TMS32051_PREG,
	TMS32051_BMAR,
	TMS32051_RPTC,
	TMS32051_ARP,
	TMS32051_ARB,
	TMS32051_AR0,
	TMS32051_AR1,
	TMS32051_AR2,
	TMS32051_AR3,
	TMS32051_AR4,
	TMS32051_AR5,
	TMS32051_AR6,
	TMS32051_AR7,
};

typedef struct {
	UINT16 pc;
	UINT16 op;
	INT32 acc;
	INT32 accb;
	INT32 preg;
	UINT16 ar[8];
	INT32 rptc;

	struct
	{
		UINT16 dp;
		UINT16 intm;
		UINT16 ovm;
		UINT16 ov;
		UINT16 arp;
	} st0;

	struct
	{
		UINT16 arb;
		UINT16 cnf;
		UINT16 tc;
		UINT16 sxm;
		UINT16 c;
		UINT16 hm;
		UINT16 xf;
		UINT16 pm;
	} st1;

	UINT16 bmar;
	UINT16 brcr;
	UINT16 paer;
	UINT16 pasr;

	struct
	{
		UINT16 iptr;
		UINT16 avis;
		UINT16 ovly;
		UINT16 ram;
		UINT16 mpmc;
		UINT16 ndx;
		UINT16 trm;
		UINT16 braf;
	} pmst;

	UINT16 ifr;
	UINT16 imr;

	UINT16 pcstack[8];
	int pcstack_ptr;

	UINT16 rpt_start, rpt_end;

	UINT16 cbcr;

	// timer regs
	UINT16 tim;
	UINT16 prd;
	UINT16 tcr;

	int (*irq_callback)(int irqline);
} TMS_REGS;

static TMS_REGS tms;
static int tms_icount;

#define CYCLES(x)		(tms_icount -= x)

#define ROPCODE()		cpu_readop16((tms.pc++) << 1)

INLINE void CHANGE_PC(UINT16 new_pc)
{
	tms.pc = new_pc;
	change_pc(tms.pc << 1);
}

INLINE UINT16 PM_READ16(UINT16 address)
{
	return program_read_word_16le(address << 1);
}

INLINE void PM_WRITE16(UINT16 address, UINT16 data)
{
	program_write_word_16le(address << 1, data);
}

INLINE UINT16 DM_READ16(UINT16 address)
{
	return data_read_word_16le(address << 1);
}

INLINE void DM_WRITE16(UINT16 address, UINT16 data)
{
	data_write_word_16le(address << 1, data);
}

#include "32051ops.c"
#include "32051ops.h"

static void op_group_be(void)
{
	tms32051_opcode_table_be[tms.op & 0xff]();
}

static void op_group_bf(void)
{
	tms32051_opcode_table_bf[tms.op & 0xff]();
}

/*****************************************************************************/

static offs_t tms_dasm(char *buffer, offs_t pc)
{
#ifdef MAME_DEBUG
	return tms32051_dasm_one(buffer, pc);
#else
	UINT16 op = ROPCODE();
	sprintf(buffer, "$%04X", op);
	return 1;
#endif
}

static void tms_init(int index, int clock, const void *_config, int (*irqcallback)(int))
{

}

static void tms_reset(void)
{
	int i;
	UINT16 src, dst, length;

	src = 0x7800;
	dst = DM_READ16(src++);
	length = DM_READ16(src++);

	CHANGE_PC(dst);

	for (i=0; i < length; i++)
	{
		UINT16 data = DM_READ16(src++);
		PM_WRITE16(dst++, data);
	}
}

static void tms_exit(void)
{
	/* TODO */
}

static void tms_get_context(void *dst)
{
	/* copy the context */
	if (dst)
		*(TMS_REGS *)dst = tms;
}

static void tms_set_context(void *src)
{
	/* copy the context */
	if (src)
		tms = *(TMS_REGS *)src;

	CHANGE_PC(tms.pc);
}

static int tms_execute(int num_cycles)
{
	tms_icount = num_cycles;

	while(tms_icount > 0)
	{
		CALL_MAME_DEBUG;

		tms.op = ROPCODE();
		tms32051_opcode_table[tms.op >> 8]();

		// handle single repeat
		if (tms.rptc > 0)
		{
			if (tms.pc == tms.rpt_end)
			{
				CHANGE_PC(tms.rpt_start);
				tms.rptc--;
			}
		}
		else
		{
			tms.rptc = 0;
		}

		// handle block repeat
		if (tms.pmst.braf)
		{
			if (tms.pc == tms.paer)
			{
				CHANGE_PC(tms.pasr);
				tms.brcr--;
				if (tms.brcr <= 0)
				{
					tms.pmst.braf = 0;
				}
			}
		}

		tms_icount--;
	}
	return num_cycles - tms_icount;
}


/*****************************************************************************/

/* Debugger definitions */

static UINT8 tms_reg_layout[] =
{
	TMS32051_PC,		-1,
	TMS32051_ACC,		TMS32051_ACCB, -1,
	TMS32051_PREG,		TMS32051_BMAR, -1,
	TMS32051_ARP,		TMS32051_ARB, -1,
	TMS32051_AR0,		TMS32051_AR1, -1,
	TMS32051_AR2,		TMS32051_AR3, -1,
	TMS32051_AR4,		TMS32051_AR5, -1,
	TMS32051_AR6,		TMS32051_AR7, 0
};

static UINT8 tms_win_layout[] =
{
	 0,16,34,17,	/* register window (top rows) */
	 0, 0,80,15,	/* disassembler window (left colums) */
	35,16,45, 2,	/* memory #2 window (right, lower middle) */
	35,19,45, 3,	/* memory #1 window (right, upper middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};


static READ16_HANDLER( cpuregs_r )
{
	switch (offset)
	{
		case 0x04:	return tms.imr;
		case 0x06:	return tms.ifr;

		case 0x07:		// PMST
		{
			UINT16 r = 0;
			r |= tms.pmst.iptr << 11;
			r |= tms.pmst.avis << 7;
			r |= tms.pmst.ovly << 5;
			r |= tms.pmst.ram << 4;
			r |= tms.pmst.mpmc << 3;
			r |= tms.pmst.ndx << 2;
			r |= tms.pmst.trm << 1;
			r |= tms.pmst.braf << 0;
			return r;
		}

		case 0x09:	return tms.brcr;
		case 0x1e:	return tms.cbcr;
		case 0x1f:	return tms.bmar;
		case 0x24:	return tms.tim;
		case 0x25:	return tms.prd;
		case 0x26:	return tms.tcr;
		case 0x28:	return 0;	// PDWSR
		default:	fatalerror("32051: cpuregs_r: unimplemented memory-mapped register %02X at %04X\n", offset, tms.pc-1);
	}

	return 0;
}

static WRITE16_HANDLER( cpuregs_w )
{
	switch (offset)
	{
		case 0x04:	tms.imr = data; break;
		case 0x06:	tms.ifr = data; break;

		case 0x07:		// PMST
		{
			tms.pmst.iptr = (data >> 11) & 0x1f;
			tms.pmst.avis = (data & 0x80) ? 1 : 0;
			tms.pmst.ovly = (data & 0x20) ? 1 : 0;
			tms.pmst.ram = (data & 0x10) ? 1 : 0;
			tms.pmst.mpmc = (data & 0x08) ? 1 : 0;
			tms.pmst.ndx = (data & 0x04) ? 1 : 0;
			tms.pmst.trm = (data & 0x02) ? 1 : 0;
			tms.pmst.braf = (data & 0x01) ? 1 : 0;
			break;
		}

		case 0x09:	tms.brcr = data; break;
		case 0x1e:	tms.cbcr = data; break;
		case 0x1f:	tms.bmar = data; break;
		case 0x24:	tms.tim = data; break;
		case 0x25:	tms.prd = data; break;
		case 0x26:	tms.tcr = data; break;
		case 0x28:	break;		// PDWSR
		default:	fatalerror("32051: cpuregs_w: unimplemented memory-mapped register %02X, data %04X at %04X\n", offset, data, tms.pc-1);
	}
}

/**************************************************************************
 * Internal memory map
 **************************************************************************/

static ADDRESS_MAP_START( internal_pgm, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x2000, 0x23ff) AM_RAM		// SARAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( internal_data, ADDRESS_SPACE_DATA, 16 )
	AM_RANGE(0x0000, 0x005f) AM_READWRITE(cpuregs_r, cpuregs_w)
	AM_RANGE(0x0060, 0x007f) AM_RAM		// DARAM B2
	AM_RANGE(0x0100, 0x04ff) AM_RAM		// DARAM B0 & B1
ADDRESS_MAP_END

/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void tms_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + TMS32051_PC:		tms.pc = info->i;						break;
	}
}

void tms_get_info(UINT32 state, union cpuinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(tms);				break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 6;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 2;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 5;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = -1;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = -1;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE:			info->i = CLEAR_LINE;	break;

		case CPUINFO_INT_PREVIOUSPC:					/* not implemented */					break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + TMS32051_PC:		info->i = tms.pc;						break;
		case CPUINFO_INT_REGISTER + TMS32051_ACC:		info->i = tms.acc;						break;
		case CPUINFO_INT_REGISTER + TMS32051_ACCB:		info->i = tms.accb;						break;
		case CPUINFO_INT_REGISTER + TMS32051_PREG:		info->i = tms.preg;						break;
		case CPUINFO_INT_REGISTER + TMS32051_BMAR:		info->i = tms.bmar;						break;
		case CPUINFO_INT_REGISTER + TMS32051_RPTC:		info->i = tms.rptc;						break;
		case CPUINFO_INT_REGISTER + TMS32051_ARP:		info->i = tms.st0.arp;					break;
		case CPUINFO_INT_REGISTER + TMS32051_ARB:		info->i = tms.st1.arb;					break;
		case CPUINFO_INT_REGISTER + TMS32051_AR0:		info->i = tms.ar[0];					break;
		case CPUINFO_INT_REGISTER + TMS32051_AR1:		info->i = tms.ar[1];					break;
		case CPUINFO_INT_REGISTER + TMS32051_AR2:		info->i = tms.ar[2];					break;
		case CPUINFO_INT_REGISTER + TMS32051_AR3:		info->i = tms.ar[3];					break;
		case CPUINFO_INT_REGISTER + TMS32051_AR4:		info->i = tms.ar[4];					break;
		case CPUINFO_INT_REGISTER + TMS32051_AR5:		info->i = tms.ar[5];					break;
		case CPUINFO_INT_REGISTER + TMS32051_AR6:		info->i = tms.ar[6];					break;
		case CPUINFO_INT_REGISTER + TMS32051_AR7:		info->i = tms.ar[7];					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = tms_get_context;		break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = tms_set_context;		break;
		case CPUINFO_PTR_INIT:							info->init = tms_init;					break;
		case CPUINFO_PTR_RESET:							info->reset = tms_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = tms_exit;					break;
		case CPUINFO_PTR_EXECUTE:						info->execute = tms_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = tms_dasm;			break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &tms_icount;				break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = tms_reg_layout;				break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = tms_win_layout;				break;
		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_PROGRAM: info->internal_map = construct_map_internal_pgm; break;
		case CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACE_DATA: info->internal_map = construct_map_internal_data; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "TMS3205x"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (C) 2005-2006 Ville Linde"); break;

		case CPUINFO_STR_FLAGS:							strcpy(info->s = cpuintrf_temp_str(), " "); break;

		case CPUINFO_STR_REGISTER + TMS32051_PC:		sprintf(info->s = cpuintrf_temp_str(), "PC: %04X", tms.pc); break;
		case CPUINFO_STR_REGISTER + TMS32051_ACC:		sprintf(info->s = cpuintrf_temp_str(), "ACC: %08X", tms.acc); break;
		case CPUINFO_STR_REGISTER + TMS32051_ACCB:		sprintf(info->s = cpuintrf_temp_str(), "ACCB: %08X", tms.accb); break;
		case CPUINFO_STR_REGISTER + TMS32051_PREG:		sprintf(info->s = cpuintrf_temp_str(), "PREG: %08X", tms.preg); break;
		case CPUINFO_STR_REGISTER + TMS32051_BMAR:		sprintf(info->s = cpuintrf_temp_str(), "BMAR: %08X", tms.bmar); break;
		case CPUINFO_STR_REGISTER + TMS32051_RPTC:		sprintf(info->s = cpuintrf_temp_str(), "RPTC: %08X", tms.rptc); break;
		case CPUINFO_STR_REGISTER + TMS32051_ARP:		sprintf(info->s = cpuintrf_temp_str(), "ARP: %04X", tms.st0.arp); break;
		case CPUINFO_STR_REGISTER + TMS32051_ARB:		sprintf(info->s = cpuintrf_temp_str(), "ARB: %04X", tms.st1.arb); break;
		case CPUINFO_STR_REGISTER + TMS32051_AR0:		sprintf(info->s = cpuintrf_temp_str(), "AR0: %04X", tms.ar[0]); break;
		case CPUINFO_STR_REGISTER + TMS32051_AR1:		sprintf(info->s = cpuintrf_temp_str(), "AR1: %04X", tms.ar[1]); break;
		case CPUINFO_STR_REGISTER + TMS32051_AR2:		sprintf(info->s = cpuintrf_temp_str(), "AR2: %04X", tms.ar[2]); break;
		case CPUINFO_STR_REGISTER + TMS32051_AR3:		sprintf(info->s = cpuintrf_temp_str(), "AR3: %04X", tms.ar[3]); break;
		case CPUINFO_STR_REGISTER + TMS32051_AR4:		sprintf(info->s = cpuintrf_temp_str(), "AR4: %04X", tms.ar[4]); break;
		case CPUINFO_STR_REGISTER + TMS32051_AR5:		sprintf(info->s = cpuintrf_temp_str(), "AR5: %04X", tms.ar[5]); break;
		case CPUINFO_STR_REGISTER + TMS32051_AR6:		sprintf(info->s = cpuintrf_temp_str(), "AR6: %04X", tms.ar[6]); break;
		case CPUINFO_STR_REGISTER + TMS32051_AR7:		sprintf(info->s = cpuintrf_temp_str(), "AR7: %04X", tms.ar[7]); break;
	}
}

#if (HAS_TMS32051)
void tms32051_set_info(UINT32 state, union cpuinfo *info)
{
	if (state >= CPUINFO_INT_INPUT_STATE && state <= CPUINFO_INT_INPUT_STATE + 5)
	{
		return;
	}
	switch(state)
	{
		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + TMS32051_PC:		tms.pc = info->i; 						break;
		case CPUINFO_INT_REGISTER + TMS32051_ACC:		tms.acc = info->i; 						break;
		case CPUINFO_INT_REGISTER + TMS32051_ACCB:		tms.accb = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS32051_PREG:		tms.preg = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS32051_BMAR:		tms.bmar = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS32051_ARP:		tms.st0.arp = info->i;					break;
		case CPUINFO_INT_REGISTER + TMS32051_ARB:		tms.st1.arb = info->i;					break;
		case CPUINFO_INT_REGISTER + TMS32051_AR0:		tms.ar[0] = info->i; 					break;
		case CPUINFO_INT_REGISTER + TMS32051_AR1:		tms.ar[1] = info->i; 					break;
		case CPUINFO_INT_REGISTER + TMS32051_AR2:		tms.ar[2] = info->i; 					break;
		case CPUINFO_INT_REGISTER + TMS32051_AR3:		tms.ar[3] = info->i; 					break;
		case CPUINFO_INT_REGISTER + TMS32051_AR4:		tms.ar[4] = info->i; 					break;
		case CPUINFO_INT_REGISTER + TMS32051_AR5:		tms.ar[5] = info->i; 					break;
		case CPUINFO_INT_REGISTER + TMS32051_AR6:		tms.ar[6] = info->i; 					break;
		case CPUINFO_INT_REGISTER + TMS32051_AR7:		tms.ar[7] = info->i; 					break;

		default:	tms_set_info(state, info);		break;
	}
}

void tms32051_get_info(UINT32 state, union cpuinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = tms32051_set_info;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "TMS32051"); break;

		default:	tms_get_info(state, info);		break;
	}
}
#endif

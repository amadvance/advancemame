// license:BSD-3-Clause
// copyright-holders:Ville Linde, Angelo Salese, hap
/*
   Motorola MC68HC11 emulator

   Written by Ville Linde
 */

#include "debugger.h"
#include "mc68hc11.h"

enum {
	HC11_PC = 1,
	HC11_SP,
	HC11_A,
	HC11_B,
	HC11_IX,
	HC11_IY
};

#define CC_S	0x80
#define CC_X	0x40
#define CC_H	0x20
#define CC_I	0x10
#define CC_N	0x08
#define CC_Z	0x04
#define CC_V	0x02
#define CC_C	0x01

typedef struct
{
	union {
		struct {
#ifdef LSB_FIRST
			UINT8 b;
			UINT8 a;
#else
			UINT8 a;
			UINT8 b;
#endif
		} d8;
		UINT16 d16;
	} d;

	UINT16 ix;
	UINT16 iy;
	UINT16 sp;
	UINT16 pc;
	UINT16 ppc;
	UINT8 ccr;

	int (*irq_callback)(int irqline);
	int icount;
	int ram_position;
	int reg_position;
	UINT8 adctl;
} HC11_REGS;

static HC11_REGS hc11;
static UINT8 *internal_ram;
static int internal_ram_size;

#define HC11OP(XX)		hc11_##XX

/*****************************************************************************/
/* Internal registers */

static UINT8 spi_data2;

static UINT8 hc11_regs_r(UINT32 address)
{
	int reg = address & 0xff;

	switch(reg)
	{
		case 0x00:
			return io_read_byte(MC68HC11_IO_PORTA);
		case 0x01:
			return 0;
		case 0x09:
			return 0;
		case 0x28:
			return 0;
		case 0x30:
			return 0x80;
		case 0x31:
		{
			if (hc11.adctl & 0x10)
			{
				return io_read_byte((hc11.adctl & 0x4) + MC68HC11_IO_AD0);
			}
			else
			{
				return io_read_byte((hc11.adctl & 0x7) + MC68HC11_IO_AD0);
			}
			break;
		}
		case 0x32:
		{
			if (hc11.adctl & 0x10)
			{
				return io_read_byte((hc11.adctl & 0x4) + MC68HC11_IO_AD1);
			}
			else
			{
				return io_read_byte((hc11.adctl & 0x7) + MC68HC11_IO_AD0);
			}
			break;
		}
		case 0x33:
		{
			if (hc11.adctl & 0x10)
			{
				return io_read_byte((hc11.adctl & 0x4) + MC68HC11_IO_AD2);
			}
			else
			{
				return io_read_byte((hc11.adctl & 0x7) + MC68HC11_IO_AD0);
			}
			break;
		}
		case 0x34:
		{
			if (hc11.adctl & 0x10)
			{
				return io_read_byte((hc11.adctl & 0x4) + MC68HC11_IO_AD3);
			}
			else
			{
				return io_read_byte((hc11.adctl & 0x7) + MC68HC11_IO_AD0);
			}
			break;
		}
		case 0x38:
			return 0;
		case 0x70:
			return 0;
		case 0x71:
			return 0;
		case 0x72:
			return 0;
		case 0x73:
			return 0;
		case 0x74:
			return 0x40;
		case 0x7c:
			return io_read_byte(MC68HC11_IO_PORTH);
		case 0x7e:
			return io_read_byte(MC68HC11_IO_PORTG);
		case 0x7f:
			return 0;

		case 0x88:
			return 0;
		case 0x89:
			return 0x80;
		case 0x8a:
			return io_read_byte(MC68HC11_IO_SPI2_DATA);

		case 0x8b:
			return 0;
	}

	fatalerror("HC11: regs_r %02X", reg);
	return 0;
}

static void hc11_regs_w(UINT32 address, UINT8 value)
{
	int reg = address & 0xff;

	switch(reg)
	{
		case 0x00:
			io_write_byte(MC68HC11_IO_PORTA, value);
			return;
		case 0x01:

			return;
		case 0x08:
			io_write_byte(MC68HC11_IO_PORTD, value);
			return;
		case 0x09:

			return;
		case 0x22:
			return;
		case 0x24:
			return;
		case 0x28:
			return;
		case 0x30:
			hc11.adctl = value;
			return;
		case 0x38:
			return;
		case 0x39:
			return;

		case 0x3d:
		{
			int reg_page = value & 0xf;
			int ram_page = (value >> 4) & 0xf;

			if (reg_page == ram_page) {
				hc11.reg_position = reg_page << 12;
				hc11.ram_position = (ram_page << 12) + 0x100;
			} else {
				hc11.reg_position = reg_page << 12;
				hc11.ram_position = ram_page << 12;
			}
			return;
		}

		case 0x3f:
			return;

		case 0x70:
			return;
		case 0x71:
			return;
		case 0x72:
			return;
		case 0x73:
			return;
		case 0x77:
			return;
		case 0x7c:
			io_write_byte(MC68HC11_IO_PORTH, value);
			return;
		case 0x7d:

			return;
		case 0x7e:
			io_write_byte(MC68HC11_IO_PORTG, value);
			return;
		case 0x7f:

			return;

		case 0x88:
			return;
		case 0x89:
			return;
		case 0x8a:
			io_write_byte(MC68HC11_IO_SPI2_DATA, value);
			return;

		case 0x8b:
			return;

	}
	fatalerror("HC11: regs_w %02X, %02X", reg, value);
}

INLINE UINT8 FETCH(void)
{
	return cpu_readop(hc11.pc++);
}

INLINE UINT16 FETCH16(void)
{
	UINT16 w;
	w = (cpu_readop(hc11.pc) << 8) | (cpu_readop(hc11.pc+1));
	hc11.pc += 2;
	return w;
}

INLINE UINT8 READ8(UINT32 address)
{
	if(address >= hc11.reg_position && address < hc11.reg_position+0x100)
	{
		return hc11_regs_r(address);
	}
	else if(address >= hc11.ram_position && address < hc11.ram_position+internal_ram_size)
	{
		return internal_ram[address-hc11.ram_position];
	}
	return program_read_byte(address);
}

INLINE void WRITE8(UINT32 address, UINT8 value)
{
	if(address >= hc11.reg_position && address < hc11.reg_position+0x100)
	{
		hc11_regs_w(address, value);
		return;
	}
	else if(address >= hc11.ram_position && address < hc11.ram_position+internal_ram_size)
	{
		internal_ram[address-hc11.ram_position] = value;
		return;
	}
	program_write_byte(address, value);
}

INLINE UINT16 READ16(UINT32 address)
{
	return (READ8(address) << 8) | (READ8(address+1));
}

INLINE void WRITE16(UINT32 address, UINT16 value)
{
	WRITE8(address+0, (value >> 8) & 0xff);
	WRITE8(address+1, (value >> 0) & 0xff);
}

/*****************************************************************************/

static void (*hc11_optable[256])(void);
static void (*hc11_optable_page2[256])(void);
static void (*hc11_optable_page3[256])(void);
static void (*hc11_optable_page4[256])(void);

#include "hc11ops.c"
#include "hc11ops.h"

static void hc11_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	int i;

	/* clear the opcode tables */
	for(i=0; i < 256; i++) {
		hc11_optable[i] = HC11OP(invalid);
		hc11_optable_page2[i] = HC11OP(invalid);
		hc11_optable_page3[i] = HC11OP(invalid);
		hc11_optable_page4[i] = HC11OP(invalid);
	}
	/* fill the opcode tables */
	for(i=0; i < sizeof(hc11_opcode_list)/sizeof(HC11_OPCODE_LIST); i++)
	{
		switch(hc11_opcode_list[i].page)
		{
			case 0x00:
				hc11_optable[hc11_opcode_list[i].opcode] = hc11_opcode_list[i].handler;
				break;
			case 0x18:
				hc11_optable_page2[hc11_opcode_list[i].opcode] = hc11_opcode_list[i].handler;
				break;
			case 0x1A:
				hc11_optable_page3[hc11_opcode_list[i].opcode] = hc11_opcode_list[i].handler;
				break;
			case 0xCD:
				hc11_optable_page4[hc11_opcode_list[i].opcode] = hc11_opcode_list[i].handler;
				break;
		}
	}

	internal_ram_size = 1280;		/* FIXME: this is for MC68HC11M0 */
	internal_ram = auto_malloc(internal_ram_size);

	hc11.reg_position = 0;
	hc11.ram_position = 0x100;
	hc11.irq_callback = irqcallback;
}

static void hc11_reset(void)
{
	hc11.pc = READ16(0xfffe);
}

static void hc11_exit(void)
{

}

static void hc11_get_context(void *dst)
{
	if (dst) {
		*(HC11_REGS*)dst = hc11;
	}
}

static void hc11_set_context(void *src)
{
	if (src) {
		hc11 = *(HC11_REGS*)src;
	}
	change_pc(hc11.pc);
}

static int hc11_execute(int cycles)
{
	hc11.icount = cycles;

	while(hc11.icount > 0)
	{
		UINT8 op;

		hc11.ppc = hc11.pc;
		CALL_MAME_DEBUG;

		op = FETCH();
		hc11_optable[op]();
	}

	return cycles-hc11.icount;
}

/*****************************************************************************/

static UINT8 hc11_reg_layout[] =
{
	HC11_PC,		HC11_SP,
	HC11_A,			HC11_B,
	HC11_IX,		HC11_IY,
	0
};

static UINT8 hc11_win_layout[] =
{
	 0, 0,80, 2,	/* register window (top rows) */
	 0, 3,35,19,	/* disassembler window (left colums) */
	36, 3,44, 8,	/* memory #1 window (right, upper middle) */
	36,12,44,10,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

offs_t hc11_disasm(char *buffer, UINT32 pc)
{
#ifdef MAME_DEBUG
	return mc68hc11_dasm_one(buffer, pc);
#else
	sprintf(buffer, "$%02X", FETCH());
	return 1;
#endif
}

/*****************************************************************************/

static void mc68hc11_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_PC:							hc11.pc = info->i; break;
		case CPUINFO_INT_REGISTER + HC11_PC:			hc11.pc = info->i; change_pc(hc11.pc); break;
		case CPUINFO_INT_REGISTER + HC11_SP:			hc11.sp = info->i; break;
		case CPUINFO_INT_REGISTER + HC11_A:				hc11.d.d8.a = info->i; break;
		case CPUINFO_INT_REGISTER + HC11_B:				hc11.d.d8.b = info->i; break;
		case CPUINFO_INT_REGISTER + HC11_IX:			hc11.ix = info->i; break;
		case CPUINFO_INT_REGISTER + HC11_IY:			hc11.iy = info->i; break;
	}
}

void mc68hc11_get_info(UINT32 state, union cpuinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(hc11);					break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 1;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 1;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 5;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 41;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 8;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE:			info->i = CLEAR_LINE;	break;

		case CPUINFO_INT_PREVIOUSPC:					/* not implemented */					break;

		case CPUINFO_INT_PC:	/* intentional fallthrough */
		case CPUINFO_INT_REGISTER + HC11_PC:			info->i = hc11.pc;						break;
		case CPUINFO_INT_REGISTER + HC11_SP:			info->i = hc11.sp;						break;
		case CPUINFO_INT_REGISTER + HC11_A:				info->i = hc11.d.d8.a;						break;
		case CPUINFO_INT_REGISTER + HC11_B:				info->i = hc11.d.d8.b;						break;
		case CPUINFO_INT_REGISTER + HC11_IX:			info->i = hc11.ix;						break;
		case CPUINFO_INT_REGISTER + HC11_IY:			info->i = hc11.iy;						break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = mc68hc11_set_info;		break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = hc11_get_context;		break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = hc11_set_context;		break;
		case CPUINFO_PTR_INIT:							info->init = hc11_init;					break;
		case CPUINFO_PTR_RESET:							info->reset = hc11_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = hc11_exit;					break;
		case CPUINFO_PTR_EXECUTE:						info->execute = hc11_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = hc11_disasm;		break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &hc11.icount;			break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = hc11_reg_layout;				break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = hc11_win_layout;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "MC68HC11"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Motorola MC68HC11"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (C) 2004 Ville Linde"); break;

		case CPUINFO_STR_FLAGS:
			sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c",
				(hc11.ccr & CC_S) ? 'S' : '.',
				(hc11.ccr & CC_X) ? 'X' : '.',
				(hc11.ccr & CC_H) ? 'H' : '.',
				(hc11.ccr & CC_I) ? 'I' : '.',
				(hc11.ccr & CC_N) ? 'N' : '.',
				(hc11.ccr & CC_Z) ? 'Z' : '.',
				(hc11.ccr & CC_V) ? 'V' : '.',
				(hc11.ccr & CC_C) ? 'C' : '.');
			break;

		case CPUINFO_STR_REGISTER + HC11_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC: %04X", hc11.pc); break;
		case CPUINFO_STR_REGISTER + HC11_SP:			sprintf(info->s = cpuintrf_temp_str(), "SP: %04X", hc11.sp); break;
		case CPUINFO_STR_REGISTER + HC11_A:				sprintf(info->s = cpuintrf_temp_str(), "A: %02X", hc11.d.d8.a); break;
		case CPUINFO_STR_REGISTER + HC11_B:				sprintf(info->s = cpuintrf_temp_str(), "B: %02X", hc11.d.d8.b); break;
		case CPUINFO_STR_REGISTER + HC11_IX:			sprintf(info->s = cpuintrf_temp_str(), "IX: %04X", hc11.ix); break;
		case CPUINFO_STR_REGISTER + HC11_IY:			sprintf(info->s = cpuintrf_temp_str(), "IY: %04X", hc11.iy); break;
	}
}

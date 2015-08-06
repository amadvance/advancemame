#ifndef z80gb_H
#define z80gb_H
#include "cpuintrf.h"
#include "driver.h"
#include "includes/gb.h"

extern int z80gb_ICount;

enum {
	Z80GB_PC=1, Z80GB_SP, Z80GB_AF, Z80GB_BC, Z80GB_DE, Z80GB_HL,
	Z80GB_IRQ_STATE
};

/****************************************************************************/
/* Return register contents 												*/
/****************************************************************************/
extern unsigned z80gb_get_reg (int regnum);

extern void z80gb_get_info(UINT32 state, union cpuinfo *info);

/****************************************************************************/
/* Memory functions                                                         */
/****************************************************************************/

#define mem_ReadByte(A)    ((UINT8)program_read_byte_8(A))
#define mem_WriteByte(A,V) (program_write_byte_8(A,V))

INLINE UINT16 mem_ReadWord (UINT32 address)
{
	UINT16 value = (UINT16) mem_ReadByte ((address + 1) & 0xffff) << 8;
	value |= mem_ReadByte (address);
	return value;
}

INLINE void mem_WriteWord (UINT32 address, UINT16 value)
{
  mem_WriteByte (address, value & 0xFF);
  mem_WriteByte ((address + 1) & 0xffff, value >> 8);
}

#ifdef MAME_DEBUG
extern unsigned z80gb_dasm( char *buffer, offs_t pc, UINT8 *oprom, UINT8 *opram, int bytes );
#endif

#endif

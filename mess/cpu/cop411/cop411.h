/**************************************************************************
 *               National Semiconductor COP411  Emulator                  *
 *                                                                        *
 *                   Copyright (C) 2005 by Dan Boris                      *
 **************************************************************************/

#ifndef _COP411_H
#define _COP411_H

#ifndef INLINE
#define INLINE static inline
#endif

#define COP411_CLOCK_DIVIDER		(4)

#define  COP411_L	0x100
#define  COP411_G	0x101
#define  COP411_D	0x102

enum { COP411_PC=1, COP411_A, COP411_B, COP411_C, COP411_EN, COP411_Q,
       COP411_SA, COP411_SB
};

extern void cop411_get_info(UINT32 state, union cpuinfo *info);


#include "memory.h"

/*
 *   Input a UINT8 from given I/O port
 */
#define COP411_In(Port) ((UINT8)io_read_byte_8(Port))


/*
 *   Output a UINT8 to given I/O port
 */
#define COP411_Out(Port,Value) (io_write_byte_8(Port,Value))


/*
 *   Read a UINT8 from given memory location
 */
#define COP411_RDMEM(A) ((unsigned)program_read_byte_8(A))


/*
 *   Write a UINT8 to given memory location
 */
#define COP411_WRMEM(A,V) (program_write_byte_8(A,V))


/*
 *   COP411_RDOP() is identical to COP411_RDMEM() except it is used for reading
 *   opcodes. In case of system with memory mapped I/O, this function can be
 *   used to greatly speed up emulation
 */
#define COP411_RDOP(A) ((unsigned)cpu_readop(A))


/*
 *   COP411_RDOP_ARG() is identical to COP411_RDOP() except it is used for reading
 *   opcode arguments. This difference can be used to support systems that
 *   use different encoding mechanisms for opcodes and opcode arguments
 */
#define COP411_RDOP_ARG(A) ((unsigned)cpu_readop_arg(A))

#ifdef  MAME_DEBUG
int 	DasmCOP411(char *dst, unsigned pc);
#endif

#endif  /* _COP411_H */

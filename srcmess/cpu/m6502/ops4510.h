// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller
/*****************************************************************************
 *
 *   ops4510.h
 *   Addressing mode and opcode macros for 4510 CPU
 *
 *   Copyright (c) 2000 Peter Trauner, all rights reserved.
 *   documentation preliminary databook
 *   documentation by michael steil mist@c64.org
 *   available at ftp://ftp.funet.fi/pub/cbm/c65
 *
 *****************************************************************************/

#define m65ce02 m4510
#define m65ce02_ICount m4510_ICount

#define M4510_MEM(addr) (m4510.mem[(addr)>>13]+(addr))

#undef CHANGE_PC
#define CHANGE_PC change_pc(M4510_MEM(PCD))

/***************************************************************
 *  RDOP    read an opcode
 ***************************************************************/
#undef RDOP
#define RDOP() m4510_cpu_readop()

/***************************************************************
 *  RDOPARG read an opcode argument
 ***************************************************************/
#undef RDOPARG
#define RDOPARG() m4510_cpu_readop_arg()

/***************************************************************
 *  RDMEM   read memory
 ***************************************************************/
#undef RDMEM
#undef RDMEM_ID
#define RDMEM(addr) program_read_byte_8(M4510_MEM(addr))
#define RDMEM_ID(addr) RDMEM(addr)

/***************************************************************
 *  WRMEM   write memory
 ***************************************************************/
#undef WRMEM
#undef WRMEM_ID
#define WRMEM(addr,data) program_write_byte_8(M4510_MEM(addr),data)
#define WRMEM_ID(addr,data) WRMEM(addr,data)

/* c65 docu says transfer of axyz to the mapper register
   so no readback!? */
#define MAP 													\
		if (PEEK_OP() == 0xea /*NOP, in this case end of map*/ )\
 { \
  m4510.mem[0]=0; \
  m4510.mem[1]=0; \
  m4510.mem[2]=0; \
  m4510.mem[3]=0; \
  m4510.mem[4]=0; \
  m4510.mem[5]=0; \
  m4510.mem[6]=0; \
  m4510.mem[7]=0; \
  CHANGE_PC; \
} else { \
  /*UINT16 low, high;*/ \
  /*low=m4510.low;*/ \
  /*high=m4510.high;*/ \
  m4510.low=m4510.a|(m4510.x<<8); \
  m4510.high=m4510.y|(m4510.z<<8); \
  /*m4510.a=low&0xff;*/ \
  /*m4510.x=low>>8;*/ \
  /*m4510.y=high&0xff;*/ \
  /*m4510.z=high>>8;*/ \
  m4510.mem[0]=(m4510.low&0x1000) ? (m4510.low&0xfff)<<8:0; \
  m4510.mem[1]=(m4510.low&0x2000) ? (m4510.low&0xfff)<<8:0; \
  m4510.mem[2]=(m4510.low&0x4000) ? (m4510.low&0xfff)<<8:0; \
  m4510.mem[3]=(m4510.low&0x8000) ? (m4510.low&0xfff)<<8:0; \
  m4510.mem[4]=(m4510.high&0x1000) ? (m4510.high&0xfff)<<8:0; \
  m4510.mem[5]=(m4510.high&0x2000) ? (m4510.high&0xfff)<<8:0; \
  m4510.mem[6]=(m4510.high&0x4000) ? (m4510.high&0xfff)<<8:0; \
  m4510.mem[7]=(m4510.high&0x8000) ? (m4510.high&0xfff)<<8:0; \
  CHANGE_PC; \
 } \
 { \
				UINT8 op = RDOP();								\
				(*m65ce02.insn[op])();							\
 } \


// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria, Curt Coder
/* National Semiconductor COP400 */

#ifndef __COP400__
#define __COP400__

#define COP400_PORT_L	0x100
#define COP400_PORT_G	0x101
#define COP400_PORT_D	0x102
#define	COP400_PORT_IN	0x103
#define	COP400_PORT_SK	0x104
#define	COP400_PORT_SIO	0x105

enum {
	COP400_PC=1,
	COP400_A,
	COP400_B,
	COP400_C,
	COP400_G,
	COP400_EN,
	COP400_Q,
    COP400_SA,
    COP400_SB,
    COP400_SC,
    COP400_SIO,
    COP400_SKL
};

extern void cop420_get_info(UINT32 state, union cpuinfo *info);

#ifdef MAME_DEBUG
offs_t cop420_dasm(char *buffer, offs_t pc, UINT8 *oprom, UINT8 *opram, int bytes_available);
#endif

#endif

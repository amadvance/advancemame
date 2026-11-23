// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/***************************************************************************

    am29dasm.h
    Disassembler for the portable Am29000 emulator.
    Written by Phil Bennett

***************************************************************************/

#ifndef __AM29DASM_H__
#define __AM29DASM_H__

#include "cpuintrf.h"

offs_t am29000_disassemble(char *buffer, offs_t pc, UINT32 op);

#endif /* __AM29DASM_H__ */

// license:BSD-3-Clause
// copyright-holders:Steve Ellenoff,R. Belmont,Ryan Holtz
/* Converted from MAME 0.287 to the AdvanceMAME C CPU interface. */
#ifndef IGS036_H
#define IGS036_H
#include "cpuintrf.h"
enum { IGS036_PC = 0, IGS036_R0, IGS036_R1, IGS036_R2, IGS036_R3,
 IGS036_R4, IGS036_R5, IGS036_R6, IGS036_R7, IGS036_R8, IGS036_R9,
 IGS036_R10, IGS036_R11, IGS036_R12, IGS036_R13, IGS036_R14, IGS036_R15, IGS036_CPSR };
enum { ARM7_IRQ_LINE = 0, ARM7_FIRQ_LINE = 1 };
void igs036_get_info(UINT32 state, union cpuinfo *info);
#endif

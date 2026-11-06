// license:BSD-3-Clause
// copyright-holders:Ryan Holtz

#ifndef ARM920T_H
#define ARM920T_H
#include "cpuintrf.h"
enum { ARM920T_PC = 0, ARM920T_R0, ARM920T_R1, ARM920T_R2, ARM920T_R3,
 ARM920T_R4, ARM920T_R5, ARM920T_R6, ARM920T_R7, ARM920T_R8, ARM920T_R9,
 ARM920T_R10, ARM920T_R11, ARM920T_R12, ARM920T_R13, ARM920T_R14, ARM920T_R15, ARM920T_CPSR };
enum { ARM7_IRQ_LINE = 0, ARM7_FIRQ_LINE = 1 };
void arm920t_get_info(UINT32 state, union cpuinfo *info);
#endif

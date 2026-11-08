// license:BSD-3-Clause
// copyright-holders:Karl Stenerud
#ifndef HEADER__SPC700DS
#define HEADER__SPC700DS
/* ======================================================================== */
/* =============================== COPYRIGHT ============================== */
/* ======================================================================== */
/*

Sony SPC700 CPU Emulator V1.0

Copyright (c) 2000 Karl Stenerud
All rights reserved.


*/

int spc700_disassemble(char* buff, unsigned int pc);

unsigned int spc700_read_8_disassembler(unsigned int address);

#include "cpuintrf.h"

#define spc700_read_8_disassembler(addr)				program_read_byte_8(addr)


#endif /* HEADER__SPC700DS */

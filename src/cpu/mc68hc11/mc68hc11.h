#ifndef _MC68HC11_H
#define _MC68HC11_H

#include "cpuintrf.h"

#ifdef MAME_DEBUG
extern int mc68hc11_dasm_one(char *buffer, offs_t pc);
#endif

void mc68hc11_get_info(UINT32 state, union cpuinfo *info);

#endif

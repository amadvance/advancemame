// license:BSD-3-Clause
// copyright-holders:Ville Linde, Angelo Salese, hap
#ifndef _MC68HC11_H
#define _MC68HC11_H

#include "cpuintrf.h"

#ifdef MAME_DEBUG
extern int mc68hc11_dasm_one(char *buffer, offs_t pc);
#endif

void mc68hc11_get_info(UINT32 state, union cpuinfo *info);

#define MC68HC11_IO_PORTA 0
#define MC68HC11_IO_PORTB 1
#define MC68HC11_IO_PORTC 2
#define MC68HC11_IO_PORTD 3
#define MC68HC11_IO_PORTE 4
#define MC68HC11_IO_PORTF 5
#define MC68HC11_IO_PORTG 6
#define MC68HC11_IO_PORTH 7
#define MC68HC11_IO_SPI1_DATA 8
#define MC68HC11_IO_SPI2_DATA 9
#define MC68HC11_IO_AD0 16
#define MC68HC11_IO_AD1 17
#define MC68HC11_IO_AD2 18
#define MC68HC11_IO_AD3 19
#define MC68HC11_IO_AD4 20
#define MC68HC11_IO_AD5 21
#define MC68HC11_IO_AD6 22
#define MC68HC11_IO_AD7 23

#endif

// license:BSD-3-Clause
// copyright-holders:Peter Trauner,Antoine Mine
/*****************************************************************************
 *
 *	 saturn.h
 *	 portable saturn emulator interface
 *   (hp calculators)
 *
 *	 Copyright (c) 2000 Peter Trauner, all rights reserved.
 *
 *****************************************************************************/
/*
Calculator        Release Date          Chip Version     Analog/Digital IC
HP71B (early)     02/01/84              1LF2              -
HP71B (later)     ??/??/??              1LK7              -
HP18C             06/01/86              1LK7              -
HP28C             01/05/87              1LK7              -
HP17B             01/04/88              1LT8             Lewis
HP19B             01/04/88              1LT8             Lewis
HP27S             01/04/88              1LT8             Lewis
HP28S             01/04/88              1LT8             Lewis
HP48SX            03/16/91              1LT8             Clarke
HP48S             04/02/91              1LT8             Clarke
HP48GX            06/01/93              1LT8             Yorke
HP48G             06/01/93              1LT8             Yorke
HP38G             09/??/95              1LT8             Yorke
*/
/* 4 bit processor
   20 address lines */

#ifndef _SATURN_H
#define _SATURN_H

#include "cpuintrf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SATURN_INT_NONE	0
#define SATURN_INT_IRQ	1
#define SATURN_INT_NMI	2

typedef struct {
	void (*out)(int);
	int (*in)(void);
	void (*reset)(void);
	void (*config)(int v);
	void (*unconfig)(int v);
	int (*id)(void);
	void (*crc)(int addr, int data);
} SATURN_CONFIG;

void saturn_get_info(UINT32 state, union cpuinfo *info);

#ifdef __cplusplus
	}
#endif

#endif


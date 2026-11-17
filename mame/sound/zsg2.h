// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, R. Belmont, hap, superctr

/* ZOOM ZSG-2 wavetable interface. */

#ifndef ZSG2_H
#define ZSG2_H

#include "osd_cpu.h"


void zsg2_init(const UINT8 *rom, UINT32 rom_bytes);
void zsg2_reset(void);

UINT16 zsg2_read(offs_t offset);
void zsg2_write(offs_t offset, UINT16 data);

void zsg2_update(INT32 **outputs, int samples);
void zsg2_state_register(void);

#endif

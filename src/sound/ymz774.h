// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, R. Belmont, MetalliC
/* Yamaha YMZ774, converted from MAME 0.287 to C. */
#ifndef YMZ774_H
#define YMZ774_H
#include "sound/custom.h"
extern const struct CustomSound_interface ymz774_interface;
UINT8 ymz774_read(offs_t offset);
void ymz774_write(offs_t offset, UINT8 data);
void ymz774_reset(void);
#endif

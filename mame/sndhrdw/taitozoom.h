// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, hap, superctr, cam900, zanac

/* Taito Zoom sound board interface. */

#ifndef TAITOZOOM_H
#define TAITOZOOM_H

WRITE32_HANDLER( taitozoom_reg_w );
WRITE32_HANDLER( taitozoom_sound_irq_w );
READ32_HANDLER( taitozoom_sound_irq_r );
READ32_HANDLER( taitozoom_shared_r );
WRITE32_HANDLER( taitozoom_shared_w );

ADDRESS_MAP_EXTERN( taitozoom_mn_map );

extern struct CustomSound_interface taitozoom_custom_interface;

void taitozoom_init( void );
void taitozoom_reset( void );

#endif

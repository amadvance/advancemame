// license:BSD-3-Clause
// copyright-holders:Philip Bennett
#ifndef CUBEQCPU_H
#define CUBEQCPU_H
void cquestsnd_get_info(UINT32 state, union cpuinfo *info);
void cquestrot_get_info(UINT32 state, union cpuinfo *info);
void cquestlin_get_info(UINT32 state, union cpuinfo *info);
READ16_HANDLER( cubeqcpu_rotram_r );
WRITE16_HANDLER( cubeqcpu_rotram_w );
READ16_HANDLER( cubeqcpu_sndram_r );
WRITE16_HANDLER( cubeqcpu_sndram_w );
void cubeqcpu_swap_banks(void);
UINT8 cubeqcpu_ptr(int y);
UINT32 *cubeqcpu_stack(void);
void cubeqcpu_dac_w(UINT16 data);
#endif

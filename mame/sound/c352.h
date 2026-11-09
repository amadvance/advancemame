// license:BSD-3-Clause
// copyright-holders:R. Belmont
#ifndef _C352_H_
#define _C352_H_

struct C352interface
{
	int region;				/* memory region of sample ROMs */
};

READ16_HANDLER( c352_0_r );
WRITE16_HANDLER( c352_0_w );

/* Optional cabinet routing, fixed-point gains (0x100 = unity). */
void c352_set_routing(const int route[4][2]);
void c352_set_output_gain(int output, int gain);

#endif


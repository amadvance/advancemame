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

#endif


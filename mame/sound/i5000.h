// license:BSD-3-Clause
// copyright-holders:hap
/* Imagetek I5000 sound emulator */

#ifndef __I5000_H__
#define __I5000_H__

struct I5000interface {
	int region;						/* memory region of the sample ROM */
};

READ16_HANDLER( I5000_0_r );
WRITE16_HANDLER( I5000_0_w );

#endif

// license:BSD-3-Clause
// copyright-holders:hap
#ifndef ALPHA8303_H
#define ALPHA8303_H

void alpha8303_init(int region);
void alpha8303_reset(void);
void alpha8303_start(int state);
void alpha8303_bus_dir(int state);
READ16_HANDLER( alpha8303_ram_r );
WRITE16_HANDLER( alpha8303_ram_w );

#endif

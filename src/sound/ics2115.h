// license:BSD-3-Clause
// copyright-holders:Alex Marshall,nimitz,austere
#ifndef __ICS2115_H__
#define __ICS2115_H__

struct ics2115_interface {
	int region;
	void (*irq_cb)(int);
};

READ8_HANDLER( ics2115_r );
WRITE8_HANDLER( ics2115_w );

#endif

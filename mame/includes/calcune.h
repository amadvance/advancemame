// license:BSD-3-Clause
// copyright-holders:David Haywood
#ifndef CALCUNE_H
#define CALCUNE_H

VIDEO_START(calcune);
VIDEO_UPDATE(calcune);
READ16_HANDLER(calcune_vdp_r);
WRITE16_HANDLER(calcune_vdp_w);
void calcune_vdp_select(int chip);
void calcune_vdp_interrupt(int line);

#endif

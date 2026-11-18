// license:BSD-3-Clause
// copyright-holders:David Haywood, Ville Linde
#ifndef _K001005_H
#define _K001005_H

#include "driver.h"

#define K001006_MAX_CHIPS 2

/* K001006 texel unit */
int k001006_start(void);
READ32_HANDLER( k001006_0_r );
WRITE32_HANDLER( k001006_0_w );
READ32_HANDLER( k001006_1_r );
WRITE32_HANDLER( k001006_1_w );

/* K001005 polygon renderer, dsp_cpunum is the SHARC driving the FIFO */
int k001005_start(int dsp_cpunum, int width, int height);
READ32_HANDLER( k001005_r );
WRITE32_HANDLER( k001005_w );
void k001005_draw(mame_bitmap *bitmap, const rectangle *cliprect);

#endif

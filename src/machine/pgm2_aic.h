// license:BSD-3-Clause
// copyright-holders:David Haywood, MetalliC
#ifndef PGM2_AIC_H
#define PGM2_AIC_H
void pgm2_aic_reset(void);
void pgm2_aic_init(void);
void pgm2_aic_irq(int line, int state);
READ32_HANDLER(pgm2_aic_r);
WRITE32_HANDLER(pgm2_aic_w);
#endif

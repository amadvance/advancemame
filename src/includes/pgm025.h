#ifndef PGM025_H
#define PGM025_H

void pgm_025_configure(unsigned region, int with_dma);
void pgm_025_reset(void);
void pgm_022_reset(UINT16 *ram);
READ16_HANDLER(pgm_025_r);
WRITE16_HANDLER(pgm_025_w);

#endif

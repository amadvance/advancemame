// license:BSD-3-Clause
// copyright-holders:Aaron Giles
#ifndef LDVP931_H
#define LDVP931_H

int vp931_init(void);
void vp931_reset(void);
void vp931_field(void);
void vp931_write(UINT8 data);
UINT8 vp931_read(void);
int vp931_ready(void);
int vp931_pending(void);
void vp931_audio(int mask);
READ8_HANDLER( vp931_io_r );
WRITE8_HANDLER( vp931_io_w );

#endif

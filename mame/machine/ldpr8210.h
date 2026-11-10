// license:BSD-3-Clause
// copyright-holders:Aaron Giles
#ifndef LDPR8210_H
#define LDPR8210_H
int simutrek_init(void);
void simutrek_reset(void);
void simutrek_field(void);
void simutrek_write(UINT8 data);
UINT8 simutrek_status(void);
void simutrek_audio(int enabled);
void simutrek_video(int enabled);
READ8_HANDLER( pr8210_io_r );
WRITE8_HANDLER( pr8210_io_w );
READ8_HANDLER( simutrek_io_r );
WRITE8_HANDLER( simutrek_io_w );
#endif

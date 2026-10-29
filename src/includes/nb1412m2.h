// license:LGPL-2.1+
// copyright-holders:Angelo Salese
#ifndef NB1412M2_H
#define NB1412M2_H

void nb1412m2_init(void);
void nb1412m2_reset(void);
READ8_HANDLER( nb1412m2_data_r );
WRITE8_HANDLER( nb1412m2_data_w );
WRITE8_HANDLER( nb1412m2_command_w );

#endif

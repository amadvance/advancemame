// license:BSD-3-Clause
// copyright-holders:Jonathan Gevaryahu, Kevin Horton, R. Belmont, Zsolt Vasvari
#ifndef S14001A_H
#define S14001A_H

#include "sound/custom.h"

extern struct CustomSound_interface s14001a_speech_interface;
int s14001a_speech_busy(void);
void s14001a_set_clock(int clock);
WRITE8_HANDLER( s14001a_word_w );
WRITE8_HANDLER( s14001a_start_w );

#endif

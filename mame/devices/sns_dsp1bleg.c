// license:BSD-3-Clause
// copyright-holders:Fabio Priuli

#include "driver.h"

/* Standalone ROM inventory for archive dependencies and XML consumers. */
ROM_START( sns_dsp1bleg )
	ROM_REGION( 0x2800, REGION_CPU1, 0 )
	ROM_LOAD( "dsp1b.bin", 0, 0x2800, CRC(453557e0) SHA1(3a218b0e4572a8eba6d0121b17fdac9529609220) )
ROM_END

DEVICE( 1990, sns_dsp1bleg, "Nintendo", "SNES DSP1B Firmware" )

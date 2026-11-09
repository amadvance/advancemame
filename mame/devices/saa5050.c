// license:BSD-3-Clause
// copyright-holders:Curt Coder, Nigel Barnes

#include "driver.h"

/* Standalone ROM inventory for archive dependencies and XML consumers. */
ROM_START( saa5050 )
	ROM_REGION( 0x500, REGION_GFX1, 0 )
	ROM_LOAD( "saa5050", 0x0140, 0x03c0, CRC(201490f3) SHA1(6c8daba70374e5aa3a6402f24cdc5f8677d58a0f) )
ROM_END

DEVICE( 197?, saa5050, "Mullard", "SAA5050 Teletext Character Generator" )

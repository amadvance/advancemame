// license:BSD-3-Clause
// copyright-holders:K.Wilkins

#include "driver.h"

/* Standalone ROM inventory for archive dependencies and XML consumers. */
ROM_START( namcoc68 )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "c68.bin", 0, 0x8000, CRC(ca64550a) SHA1(38d1ad1b1287cadef0c999aff9357927315f8e6b) )
ROM_END

DEVICE( 198?, namcoc68, "Namco", "Namco C68 I/O" )

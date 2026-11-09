// license:BSD-3-Clause
// copyright-holders:Alex W. Jackson

#include "driver.h"

/* Standalone ROM inventory for archive dependencies and XML consumers. */
ROM_START( namcoc75 )
	ROM_REGION( 0x4000, REGION_CPU1, 0 )
	ROM_LOAD( "c75.bin", 0, 0x4000, CRC(42f539a5) SHA1(3103e5a0a2867620309fd4fe478a2be0effbeff8) )
ROM_END

DEVICE( 199?, namcoc75, "Namco", "Namco C75 (M37702)" )

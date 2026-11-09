// license:BSD-3-Clause
// copyright-holders:Aaron Giles

#include "driver.h"

/* Standalone ROM inventory for archive dependencies and XML consumers. */
ROM_START( midssio )
	ROM_REGION( 0x20, REGION_CPU1, 0 )
	ROM_LOAD( "82s123.12d", 0, 0x20, CRC(e1281ee9) SHA1(9ac9b01d24affc0ee9227a4364c4fd8f8290343a) )
ROM_END

DEVICE( 198?, midssio, "Midway", "Midway SSIO Sound Board" )

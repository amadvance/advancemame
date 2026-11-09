// license:BSD-3-Clause
// copyright-holders:smf, R. Belmont

#include "driver.h"

/* Standalone ROM inventory for archive dependencies and XML consumers. */
ROM_START( coh3002t )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD( "m534002c-60.ic353", 0x00000, 0x80000, CRC(03967fa7) SHA1(0e17fec2286e4e25deb23d40e41ce0986f373d49) )
ROM_END

DEVICE( 1997, coh3002t, "Sony/Taito", "Taito GNET BIOS" )

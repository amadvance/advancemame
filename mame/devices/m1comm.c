// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann

#include "driver.h"

/* Standalone ROM inventory for archive dependencies and XML consumers. */
ROM_START( m1comm )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD( "epr-15112.17", 0x00000, 0x20000, CRC(4950e771) SHA1(99014124e0324dd114cb22f55159d18b597a155a) )
	ROM_LOAD( "epr-15624.17", 0x20000, 0x20000, CRC(9b3ba315) SHA1(0cd0983cc8b2f2d6b41617d0d0a24cc6c188e62a) )
ROM_END

DEVICE( 1992, m1comm, "Sega", "Model 1 Communication Board" )

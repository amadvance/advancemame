// license:BSD-3-Clause
// copyright-holders:K.Wilkins

#include "driver.h"

/* Standalone ROM inventory for archive dependencies and XML consumers. */
ROM_START( namcoc65 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "sys2mcpu.bin", 0, 0x2000, CRC(a342a97e) SHA1(2c420d34dba21e409bf78ddca710fc7de65a6642) )
ROM_END

DEVICE( 198?, namcoc65, "Namco", "Namco C65 I/O" )

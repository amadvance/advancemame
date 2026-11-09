// license:BSD-3-Clause
// copyright-holders:Phil Stroffolino, hap, R. Belmont

#include "driver.h"

/* Standalone ROM inventory for archive dependencies and XML consumers. */
ROM_START( namcoc74 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD( "c74.bin", 0, 0x4000, CRC(a3dce360) SHA1(8f3248b1890abb2e649927240ae46f73bb171e3b) )
ROM_END

DEVICE( 199?, namcoc74, "Namco", "Namco C74 (M37702)" )

// license:BSD-3-Clause
// copyright-holders:Phil Stroffolino

#include "driver.h"

/* Standalone ROM inventory for archive dependencies and XML consumers. */
ROM_START( namcoc69 )
	ROM_REGION16_LE( 0x4000, REGION_CPU1, 0 )
	ROM_LOAD( "c69.bin", 0, 0x4000, CRC(349134d9) SHA1(61a4981fc2716c228b6121fedcbf1ed6f34dc2de) )
ROM_END

DEVICE( 199?, namcoc69, "Namco", "Namco C69 (M37702)" )

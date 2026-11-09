// license:BSD-3-Clause
// copyright-holders:Phil Stroffolino, hap, R. Belmont

#include "driver.h"

/* Standalone ROM inventory for archive dependencies and XML consumers. */
ROM_START( namcoc71 )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD( "c71.bin", 0, 0x2000, CRC(47c623ab) SHA1(e363ac50f5556f83308d4cc191b455e9b62bcfc8) )
ROM_END

DEVICE( 199?, namcoc71, "Namco", "Namco C71 (TMS320C25)" )

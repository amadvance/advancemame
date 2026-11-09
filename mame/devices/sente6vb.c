// license:BSD-3-Clause
// copyright-holders:Aaron Giles

#include "driver.h"

/* Standalone ROM inventory for archive dependencies and XML consumers. */
ROM_START( sente6vb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "8002-10 9-25-84.5", 0, 0x2000, CRC(4dd0a525) SHA1(f0c447adc5b67917851a9df978df851247e75c43) )
ROM_END

DEVICE( 1984, sente6vb, "Bally/Sente", "Bally Sente 6VB Audio Board" )

// license:BSD-3-Clause
// copyright-holders:David Haywood, Nicola Salmoria, Tomasz Slanina

#include "driver.h"

/* Standalone ROM inventory for archive dependencies and XML consumers. */
ROM_START( t5182 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "t5182.rom", 0, 0x2000, CRC(d354c8fc) SHA1(a1c9e1ac293f107f69cc5788cf6abc3db1646e33) )
ROM_END

DEVICE( 198?, t5182, "Toshiba", "T5182 MCU" )

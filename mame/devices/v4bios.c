// license:BSD-3-Clause
// copyright-holders:David Haywood

#include "driver.h"

/* Standalone ROM inventory for archive dependencies and XML consumers. */
ROM_START( v4bios )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "vid.p1", 0x0000, 0x10000, CRC(e996bc18) SHA1(49798165640627eb31024319353da04380787b10) )
ROM_END

DEVICE( 199?, v4bios, "Barcrest", "MPU4 Video Firmware" )

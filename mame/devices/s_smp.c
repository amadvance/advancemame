// license:LGPL-2.1+
// copyright-holders:Tomasz Slanina, Peter Ferrie, Stephane Humbert

#include "driver.h"

/* Standalone ROM inventory for archive dependencies and XML consumers. */
ROM_START( s_smp )
	ROM_REGION( 0x100, REGION_CPU1, 0 )
	ROM_LOAD( "spc700.rom", 0, 0x40, CRC(44bb3a40) SHA1(97e352553e94242ae823547cd853eecda55c20f0) )
ROM_END

DEVICE( 1990, s_smp, "Nintendo/Sony", "Nintendo/Sony S-SMP" )

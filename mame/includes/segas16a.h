// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

	Sega System 16A hardware

***************************************************************************/

#include "vidhrdw/segaic16.h"

/*----------- defined in vidhrdw/segas16a.c -----------*/

VIDEO_START( system16a );
VIDEO_UPDATE( system16a );

/*----------- defined in machine/system16.c -----------*/

extern MACHINE_RESET( sys16_onetime );

// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*************************************************************************

    Driver for Midway Zeus games

**************************************************************************/

/*
    Differences from MAME:

    C globals and handlers instead of the midzeus_state class. The Zeus 2
    video hardware (midzeus2.c) is not ported.
*/

#define MIDZEUS_VIDEO_CLOCK		66666700


/*----------- defined in vidhrdw/midzeus.c -----------*/

extern UINT32 *zeusbase;

VIDEO_START( midzeus );
VIDEO_UPDATE( midzeus );

READ32_HANDLER( zeus_r );
WRITE32_HANDLER( zeus_w );

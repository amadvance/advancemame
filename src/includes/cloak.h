// license:BSD-3-Clause
// copyright-holders:Dan Boris, Mirko Buffoni
/*************************************************************************

    Atari Cloak & Dagger hardware

*************************************************************************/

/*----------- defined in vidhrdw/cloak.c -----------*/

WRITE8_HANDLER( cloak_paletteram_w );
READ8_HANDLER( graph_processor_r );
WRITE8_HANDLER( graph_processor_w );
WRITE8_HANDLER( cloak_clearbmp_w );

VIDEO_START( cloak );
VIDEO_UPDATE( cloak );

// license:BSD-3-Clause
// copyright-holders:David Graves, R. Belmont
/*----------- defined in vidhrdw/gcpinbal.c -----------*/

VIDEO_START( gcpinbal );
VIDEO_UPDATE( gcpinbal );

READ16_HANDLER ( gcpinbal_tilemaps_word_r );
WRITE16_HANDLER( gcpinbal_tilemaps_word_w );

extern UINT16 *gcpinbal_tilemapram;
extern UINT16 *gcpinbal_ioc_ram;


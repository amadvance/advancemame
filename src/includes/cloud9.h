// license:BSD-3-Clause
// copyright-holders:Mike Balfour, Aaron Giles

WRITE8_HANDLER( cloud9_paletteram_w );
WRITE8_HANDLER( cloud9_video_control_w );
WRITE8_HANDLER( cloud9_videoram_w );
READ8_HANDLER( cloud9_bitmode_r );
WRITE8_HANDLER( cloud9_bitmode_w );
WRITE8_HANDLER( cloud9_bitmode_addr_w );
VIDEO_START( cloud9 );
VIDEO_UPDATE( cloud9 );

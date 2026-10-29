// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*************************************************************************

    Model Racing Dribbling hardware

*************************************************************************/

/*----------- defined in drivers/dribling.c -----------*/

extern UINT8 dribling_abca;


/*----------- defined in vidhrdw/dribling.c -----------*/

PALETTE_INIT( dribling );
WRITE8_HANDLER( dribling_colorram_w );
VIDEO_UPDATE( dribling );

struct CustomSound_interface;
extern const struct CustomSound_interface dribling_sound_interface;
void dribling_sound_data_w(UINT8 data);
void dribling_sound_pitch_w(UINT8 data);
void dribling_sound_control_w(UINT8 data);

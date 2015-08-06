/*************************************************************************

    Sega Z80-3D system

*************************************************************************/

#include "sound/discrete.h"

/*----------- defined in machine/turbo.c -----------*/

extern UINT8 turbo_opa, turbo_opb, turbo_opc;
extern UINT8 turbo_ipa, turbo_ipb, turbo_ipc;
extern UINT8 turbo_fbpla, turbo_fbcol;
extern UINT8 turbo_speed;

extern UINT8 subroc3d_col, subroc3d_ply, subroc3d_chofs;

extern UINT8 buckrog_fchg, buckrog_mov, buckrog_obch;

MACHINE_RESET( turbo );
MACHINE_RESET( subroc3d );
MACHINE_RESET( buckrog );

READ8_HANDLER( turbo_8279_r );
WRITE8_HANDLER( turbo_8279_w );

READ8_HANDLER( turbo_collision_r );
WRITE8_HANDLER( turbo_collision_clear_w );
WRITE8_HANDLER( turbo_coin_and_lamp_w );

void turbo_rom_decode(void);

void turbo_update_tachometer(void);
void turbo_update_segments(void);

READ8_HANDLER( buckrog_cpu2_command_r );
READ8_HANDLER( buckrog_port_2_r );
READ8_HANDLER( buckrog_port_3_r );


/*----------- defined in sndhrdw/turbo.c -----------*/

extern struct discrete_sound_block turbo_sound_interface[];

WRITE8_HANDLER( turbo_sound_A_w );
WRITE8_HANDLER( turbo_sound_B_w );
WRITE8_HANDLER( turbo_sound_C_w );

WRITE8_HANDLER( subroc3d_sound_A_w );
WRITE8_HANDLER( subroc3d_sound_B_w );
WRITE8_HANDLER( subroc3d_sound_C_w );

WRITE8_HANDLER( buckrog_sound_A_w );
WRITE8_HANDLER( buckrog_sound_B_w );


/*----------- defined in vidhrdw/turbo.c -----------*/

extern UINT8 *sega_sprite_position;
extern UINT8 turbo_collision;

PALETTE_INIT( turbo );
VIDEO_START( turbo );
VIDEO_EOF( turbo );
VIDEO_UPDATE( turbo );

PALETTE_INIT( subroc3d );
VIDEO_START( subroc3d );
VIDEO_UPDATE( subroc3d );

PALETTE_INIT( buckrog );
VIDEO_START( buckrog );
VIDEO_UPDATE( buckrog );

WRITE8_HANDLER( buckrog_bitmap_w );

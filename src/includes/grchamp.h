/*************************************************************************

    Taito Grand Champ hardware

*************************************************************************/

#include "sound/discrete.h"

/* Discrete Sound Input Nodes */
#define GRCHAMP_ENGINE_CS_EN				NODE_01
#define GRCHAMP_SIFT_DATA					NODE_02
#define GRCHAMP_ATTACK_UP_DATA				NODE_03
#define GRCHAMP_IDLING_EN					NODE_04
#define GRCHAMP_FOG_EN						NODE_05
#define GRCHAMP_PLAYER_SPEED_DATA			NODE_06
#define GRCHAMP_ATTACK_SPEED_DATA			NODE_07
#define GRCHAMP_A_DATA						NODE_08
#define GRCHAMP_B_DATA						NODE_09

/*----------- defined in sndhrdw/grchamp.c -----------*/

extern struct discrete_sound_block grchamp_discrete_interface[];

/*----------- defined in vidhrdw/grchamp.c -----------*/

extern UINT8 grchamp_videoreg0;
extern UINT8 grchamp_player_ypos;
extern int grchamp_collision;
extern UINT8 grchamp_vreg1[0x10];
extern UINT8 *grchamp_videoram;
extern UINT8 *grchamp_radar;

PALETTE_INIT( grchamp );
VIDEO_START( grchamp );
VIDEO_UPDATE( grchamp );
WRITE8_HANDLER( grchamp_videoram_w );

WRITE8_HANDLER( grchamp_player_xpos_w );
WRITE8_HANDLER( grchamp_player_ypos_w );
WRITE8_HANDLER( grchamp_tile_select_w );
WRITE8_HANDLER( grchamp_rain_xpos_w );
WRITE8_HANDLER( grchamp_rain_ypos_w );

/*----------- defined in machine/grchamp.c -----------*/

extern int grchamp_cpu_irq_enable[2];

DRIVER_INIT( grchamp );
READ8_HANDLER( grchamp_port_0_r );
READ8_HANDLER( grchamp_port_1_r );
WRITE8_HANDLER( grchamp_port_1_w );

WRITE8_HANDLER( grchamp_control0_w );
WRITE8_HANDLER( grchamp_coinled_w );
WRITE8_HANDLER( grchamp_sound_w );
WRITE8_HANDLER( grchamp_comm_w );

WRITE8_HANDLER( grchamp_portA_0_w );
WRITE8_HANDLER( grchamp_portB_0_w );
WRITE8_HANDLER( grchamp_portA_1_w );
WRITE8_HANDLER( grchamp_portB_1_w );
WRITE8_HANDLER( grchamp_portA_2_w );
WRITE8_HANDLER( grchamp_portB_2_w );

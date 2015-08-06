/*************************************************************************

    8080bw.h

*************************************************************************/

#include "sound/discrete.h"

/*----------- defined in machine/8080bw.c -----------*/

WRITE8_HANDLER( c8080bw_shift_amount_w );
WRITE8_HANDLER( c8080bw_shift_data_w );
READ8_HANDLER( c8080bw_shift_data_r );
READ8_HANDLER( c8080bw_shift_data_rev_r );
READ8_HANDLER( c8080bw_shift_data_comp_r );
INTERRUPT_GEN( c8080bw_interrupt );

READ8_HANDLER( boothill_shift_data_r );

READ8_HANDLER( spcenctr_port_0_r );
READ8_HANDLER( spcenctr_port_1_r );

READ8_HANDLER( boothill_port_0_r );
READ8_HANDLER( boothill_port_1_r );

READ8_HANDLER( gunfight_port_0_r );
READ8_HANDLER( gunfight_port_1_r );

READ8_HANDLER( seawolf_port_1_r );

WRITE8_HANDLER( desertgu_controller_select_w );
READ8_HANDLER( desertgu_port_1_r );

/*----------- defined in sndhrdw/8080bw.c -----------*/

MACHINE_RESET( invaders );
MACHINE_RESET( sstrangr );
MACHINE_RESET( invad2ct );
MACHINE_RESET( gunfight );
MACHINE_RESET( boothill );
MACHINE_RESET( phantom2 );
MACHINE_RESET( bowler );
MACHINE_RESET( ballbomb );
MACHINE_RESET( seawolf );
MACHINE_RESET( desertgu );
MACHINE_RESET( schaser );
MACHINE_RESET( polaris );
MACHINE_RESET( clowns );

WRITE8_HANDLER( indianbt_sh_port7_w );

extern struct Samplesinterface boothill_samples_interface;
extern struct discrete_sound_block clowns_discrete_interface[];
extern struct SN76477interface invaders_sn76477_interface;
extern struct Samplesinterface invaders_samples_interface;
extern struct SN76477interface invad2ct_sn76477_interface_1;
extern struct SN76477interface invad2ct_sn76477_interface_2;
extern struct Samplesinterface invad2ct_samples_interface;
extern struct discrete_sound_block indianbt_discrete_interface[];
extern struct discrete_sound_block polaris_discrete_interface[];
extern struct discrete_sound_block schaser_discrete_interface[];
void schaser_effect_555_cb(int effect);
extern mame_timer *schaser_effect_555_timer;
extern struct SN76477interface schaser_sn76477_interface;
extern struct Samplesinterface seawolf_samples_interface;


/*----------- defined in vidhrdw/8080bw.c -----------*/

DRIVER_INIT( 8080bw );
DRIVER_INIT( invaders );
DRIVER_INIT( invadpt2 );
DRIVER_INIT( cosmo );
DRIVER_INIT( sstrngr2 );
DRIVER_INIT( invaddlx );
DRIVER_INIT( invrvnge );
DRIVER_INIT( invad2ct );
DRIVER_INIT( schaser );
DRIVER_INIT( rollingc );
DRIVER_INIT( polaris );
DRIVER_INIT( lupin3 );
DRIVER_INIT( seawolf );
DRIVER_INIT( blueshrk );
DRIVER_INIT( desertgu );
DRIVER_INIT( phantom2 );
DRIVER_INIT( bowler );
DRIVER_INIT( gunfight );
DRIVER_INIT( indianbt );
DRIVER_INIT( shuttlei );

void c8080bw_flip_screen_w(int data);
void c8080bw_screen_red_w(int data);

INTERRUPT_GEN( polaris_interrupt );
INTERRUPT_GEN( phantom2_interrupt );

WRITE8_HANDLER( c8080bw_videoram_w );
WRITE8_HANDLER( schaser_colorram_w );
READ8_HANDLER( schaser_colorram_r );
WRITE8_HANDLER( cosmo_colorram_w );

VIDEO_UPDATE( 8080bw );

PALETTE_INIT( invadpt2 );
PALETTE_INIT( sflush );
PALETTE_INIT( cosmo );
PALETTE_INIT( indianbt );

WRITE8_HANDLER( bowler_bonus_display_w );

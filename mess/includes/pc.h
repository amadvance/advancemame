#include "driver.h"

void init_pccga(void);
void init_pcmda(void);
void init_europc(void);
void init_bondwell(void);
void init_pc200(void);
void init_pc1512(void);
void init_pc1640(void);
void init_pc_vga(void);
void init_t1000hx(void);

MACHINE_RESET( pc_mda );
MACHINE_RESET( pc_cga );
MACHINE_RESET( pc_t1t );
MACHINE_RESET( pc_aga );
MACHINE_RESET( pc_vga );

void pc_cga_frame_interrupt(void);
void pc_mda_frame_interrupt(void);
void tandy1000_frame_interrupt (void);
void pc_aga_frame_interrupt(void);
void pc_vga_frame_interrupt(void);


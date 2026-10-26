// license:BSD-3-Clause
// copyright-holders:Kevin Thacker, Curt Coder
MACHINE_RESET( enterprise );
DEVICE_LOAD( enterprise_floppy );

int enterprise_rom_load(int);
int enterprise_rom_id(int);

void Enterprise_Initialise(void);

VIDEO_START( enterprise );
VIDEO_UPDATE( enterprise );



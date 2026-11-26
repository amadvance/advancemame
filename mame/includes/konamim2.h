// license:BSD-3-Clause
// copyright-holders:Philip Bennett
#ifndef KONAMIM2_H
#define KONAMIM2_H

#define M2_CLOCK 66666700

void m2_irq(UINT32 bit, int state);
UINT8 m2_ram_byte(UINT32 address);
UINT16 m2_ram_word(UINT32 address);
UINT32 m2_ram_dword(UINT32 address);
void m2_ram_write_byte(UINT32 address, UINT8 data);
void m2_ram_write_word(UINT32 address, UINT16 data);
void m2_ram_write_dword(UINT32 address, UINT32 data);

void m2_te_init(void);
void m2_te_reset(void);
UINT32 m2_te_read(offs_t offset);
void m2_te_write(offs_t offset, UINT32 data);
READ64_HANDLER( m2_te_tram_r );
WRITE64_HANDLER( m2_te_tram_w );

void m2_dspp_init(void);
void m2_dspp_reset(void);
UINT32 m2_dspp_read(offs_t offset);
void m2_dspp_write(offs_t offset, UINT32 data);

#endif
